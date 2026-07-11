/*
  ESP32 firmware for the spectrum LED visualizer. Receives UDP frames from the
  PC and renders them on 2 vertically stacked 8x32 WS2812B panels.

  PHYSICAL LAYOUT:
    - Panels are stacked to form a 32 wide x 16 tall grid (512 LEDs)
    - Panel 0 (first in the data chain) is the BOTTOM half  -> y = 8..15
    - Panel 1 (second in the data chain) is the TOP half    -> y = 0..7
    - Each panel is column-major serpentine, origin top-left
    - Panel 1 (top) is rotated 180 degrees relative to panel 0 (bottom)
    - Data on GPIO 32 -> Panel 0 DIN, Panel 0 DOUT -> Panel 1 DIN

    Logical grid coordinates:
      x: 0-31  left -> right
      y: 0-15  top  -> bottom

  COMMUNICATION PROTOCOL:
    byte 0     : mode      0 = filled bars, 1 = oscilloscope trace
    byte 1-3  : R, G, B   theme colour
    byte 4-35 : heights   one byte per column (0-255)
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

// ---------- CONFIG -----------

#define WIFI_SSID   "YourNetworkName"       // SSID of your Wi-Fi network
#define WIFI_PASS   "YourNetworkPassword"   // Password of your Wi-Fi network

#define UDP_PORT    4210 

#define DATA_PIN      32       
#define PANEL_WIDTH   32
#define PANEL_HEIGHT  8
#define NUM_PANELS    2

#define MATRIX_WIDTH  PANEL_WIDTH                               // 32
#define MATRIX_HEIGHT (PANEL_HEIGHT * NUM_PANELS)               // 16
#define NUM_LEDS      (PANEL_WIDTH * PANEL_HEIGHT * NUM_PANELS) // 512

// UDP payload layout — MUST match udp_sender.cpp on the PC.
#define MODE_BYTES    1
#define COLOUR_BYTES  3
#define HEADER_BYTES  (MODE_BYTES + COLOUR_BYTES)     // 4
#define PAYLOAD_SIZE  (HEADER_BYTES + MATRIX_WIDTH)   // 36

#define MODE_BARS     0
#define MODE_TRACE    1

// Keep low while powered over USB (brownout risk above ~50)
#define BRIGHTNESS    25

// FastLED power governor: caps total draw regardless of what we ask for.
// 1500 mA is a safe ceiling for USB testing. Raise (e.g. to 6000) with a PSU.
#define MAX_MILLIAMPS 1500

// ---------- GLOBALS -----------

CRGB leds[NUM_LEDS];
WiFiUDP udp;

// Frame state
uint8_t barHeights[MATRIX_WIDTH] = {};
uint8_t displayMode = MODE_BARS;
CRGB    themeColour = CRGB(0, 255, 160);   // fallback until first packet

// Diagnostics
bool          firstFrameSeen  = false;
unsigned long lastSizeWarnMs  = 0;

// ---------- LED INDEX MAPPING -----------

// Map logical (x, y) -> LED index for 2 vertically stacked panels
int XY(int x, int y) {
  int panel;   
  int localY;  

  if (y >= PANEL_HEIGHT) {
    panel  = 0;
    localY = y - PANEL_HEIGHT;  
  } else {
    panel  = 1;
    localY = y;             
  }

  int localX = x;

  // The top (panel 1) is rotated 180 degrees relative to the bottom (panel 0)
  if (panel == 1) {
    localX = (PANEL_WIDTH  - 1) - localX;
    localY = (PANEL_HEIGHT - 1) - localY;
  }

  int base = panel * PANEL_WIDTH * PANEL_HEIGHT;

  // Even columns run top→bottom, odd columns run bottom→top
  if (localX % 2 == 0) {
    return base + localX * PANEL_HEIGHT + localY;
  } else {
    return base + localX * PANEL_HEIGHT + (PANEL_HEIGHT - 1 - localY);
  }
}


// ---------- RENDERERS -----------

// Paint the idle background (very dim grey keeps the grid visible)
inline void paintBackground() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(4, 4, 4);
}

// Gradient theme sentinel: the terminal stores it as pure white and computes
// its green→red gradient per-row, so a white theme colour means "colour by
// height" here too. (A custom white JSON theme also triggers this.)

// Gradient theme:
//   The theme's stored RGB is pure white (1,1,1) because the terminal
//   computes its green→red gradient per-row at draw time.
//   We use that as a sentinel: when the incoming theme colour is exactly white, 
//   render with the height gradient below instead of a solid colour, mirroring the terminal.
// (Caveat: a custom JSON theme of pure white will also get the gradient)

inline bool gradientActive() {
  return themeColour.r == 255 && themeColour.g == 255 && themeColour.b == 255;
}

// Green→yellow→red gradient across the height: 0.0 = green (bottom), 1.0 = red (top)
CRGB barColour(float fraction) {
  if (fraction < 0.0f) fraction = 0.0f;
  if (fraction > 1.0f) fraction = 1.0f;

  uint8_t r, g;
  if (fraction < 0.5f) {
    r = (uint8_t)(fraction * 2.0f * 255);   // Green → Yellow
    g = 255;
  } else {
    r = 255;                                 // Yellow → Red
    g = (uint8_t)((1.0f - (fraction - 0.5f) * 2.0f) * 255);
  }
  return CRGB(r, g, 0);
}

// Pixel colour: height gradient when the Gradient theme is active, else solid theme colour
inline CRGB pixelColour(int rowFromBottom) {
  if (gradientActive()) {
    return barColour((float)rowFromBottom / (float)(MATRIX_HEIGHT - 1));
  }
  return themeColour;
}

// MODE_BARS — filled columns growing up from the bottom (height 0 = dark, 255 = full)
void drawBars() {
  paintBackground();

  for (int x = 0; x < MATRIX_WIDTH; x++) {
    int litRows = ((int)barHeights[x] * MATRIX_HEIGHT + 127) / 255;  // 0-255 → 0-16, rounded

    for (int r = 0; r < litRows; r++) {
      int y = (MATRIX_HEIGHT - 1) - r;   // r rows up → top-origin y
      leds[XY(x, y)] = pixelColour(r);
    }
  }
  FastLED.show();
}

// MODE_TRACE — oscilloscope line through the 32 heights (0 = bottom, 255 = top)
void drawTrace() {
  paintBackground();

  int prevY = -1;
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    int rowFromBottom = ((int)barHeights[x] * (MATRIX_HEIGHT - 1) + 127) / 255;
    int y = (MATRIX_HEIGHT - 1) - rowFromBottom;

    if (prevY < 0) prevY = y;   // first column: nothing to connect yet

    // Fill this column from the previous column's level to its own, so steep
    // jumps between adjacent columns render as a connected line
    int yStart = (prevY < y) ? prevY : y;
    int yEnd   = (prevY < y) ? y : prevY;
    for (int yy = yStart; yy <= yEnd; yy++) {
      leds[XY(x, yy)] = pixelColour((MATRIX_HEIGHT - 1) - yy);
    }
    prevY = y;
  }
  FastLED.show();
}


// ---------- SETUP -----------

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== spectrum LED display (32x16, 512 LEDs, GPIO 32) ===");
  Serial.printf("[spectrum] Protocol v3: expecting %d-byte packets "
                "([mode][R][G][B][%d heights])\n", PAYLOAD_SIZE, MATRIX_WIDTH);

  // LED initialization and configuration
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);  // power safety cap
  FastLED.clear(true);

  // Quick self-test: sweep one row down the display
  Serial.println("[spectrum] Self-test: row sweep…");
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    FastLED.clear();
    for (int x = 0; x < MATRIX_WIDTH; x++) leds[XY(x, y)] = CRGB::Green;
    FastLED.show();
    delay(60);
  }
  FastLED.clear(true);

  // Wifi
  Serial.print("[spectrum] Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait for connection with a timeout (20 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempts > 40) {
      Serial.println("\n[spectrum] WiFi failed – check SSID/password (2.4 GHz only).");
      fill_solid(leds, NUM_LEDS, CRGB(64, 0, 0));
      FastLED.show();
      while (true) delay(1000);
    }
  }

  Serial.println();
  Serial.print("[spectrum] Connected. IP: ");
  Serial.println(WiFi.localIP());   // ← copy this into udp_sender.hpp on the PC

  udp.begin(UDP_PORT);
  Serial.print("[spectrum] Listening on UDP port ");
  Serial.println(UDP_PORT);
  Serial.println("[spectrum] Waiting for first frame from the PC…");
}


// ---------- LOOP -----------

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  // Read the packet if it's the expected size
  if (packetSize == PAYLOAD_SIZE) {
    uint8_t buf[PAYLOAD_SIZE];
    udp.read(buf, PAYLOAD_SIZE);

    // Parse the packet into the variables used by the renderers
    displayMode = buf[0];
    themeColour = CRGB(buf[1], buf[2], buf[3]);
    memcpy(barHeights, buf + HEADER_BYTES, MATRIX_WIDTH);

    // Print message only on the first frame
    if (!firstFrameSeen) {
      firstFrameSeen = true;
      Serial.printf("[spectrum] First frame received. mode=%d colour=(%d,%d,%d)\n",
                    displayMode, buf[1], buf[2], buf[3]);
    }

    // Display the frame according to the current mode
    if (displayMode == MODE_TRACE) drawTrace();
    else                           drawBars();
  }

  else {
    // Wrong-size packet (the PC and this firmware disagree on the protocol)
    uint8_t scratch[64];
    while (udp.available()) udp.read(scratch, sizeof(scratch));

    unsigned long now = millis();
    if (now - lastSizeWarnMs > 1000) {
      lastSizeWarnMs = now;
      Serial.printf("[spectrum] !! Dropping packets: got %d bytes, expected %d.\n"
                    "           PC exe and firmware are out of sync — rebuild the\n"
                    "           PC app AND reflash this firmware from the same repo.\n",
                    packetSize, PAYLOAD_SIZE);
    }
  }
}
