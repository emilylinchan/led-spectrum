/*
  Individual LED test for 2 vertically stacked 8x32 WS2812B LED panels using ESP32 Dev Board

  PHYSICAL LAYOUT:
    - Panels are stacked to form a 32 wide x 16 tall grid
    - Panel 0 (first in the data chain) is the BOTTOM half  -> y = 8-15
    - Panel 1 (second in the data chain) is the TOP half    -> y = 0-7
    - Each panel is column-major serpentine, origin top-left
    - Panel 1 (top) is rotated 180 degrees relative to panel 0 (bottom)

    Logical grid coordinates:
      x: 0-31  left -> right
      y: 0-15  top  -> bottom  

  Tests (cycles automatically, prints status over Serial @ 115200):
    1. Pixel walk     - lights each LED one at a time, in wire order
    2. Row sweep      - one horizontal line, top -> bottom (verifies stack)
    3. Column sweep   - one vertical line, left -> right
    4. Panel ID       - bottom panel RED, top panel BLUE (verifies chain)
    5. Corner markers - marks the four true corners of the 32x16 grid
    6. Full white     - low-brightness full fill (power sanity check)

  Send 'n' over Serial at any time to skip to the next test
*/

#include <FastLED.h>

// ---------- CONFIG ----------

#define DATA_PIN      32
#define PANEL_WIDTH   32
#define PANEL_HEIGHT  8
#define NUM_PANELS    2

// Vertically stacked: full grid is 1 panel wide, 2 panels tall
#define MATRIX_WIDTH  PANEL_WIDTH                       // 32
#define MATRIX_HEIGHT (PANEL_HEIGHT * NUM_PANELS)       // 16
#define NUM_LEDS      (PANEL_WIDTH * PANEL_HEIGHT * NUM_PANELS)  // 512

// Keep low while powered over USB (brownout risk above ~50)
#define BRIGHTNESS  25

// ---------- LED INDEX MAPPING -----------

CRGB leds[NUM_LEDS];

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

  // Invert since the top panel rotated 180 degrees relative to the bottom panel
  if (panel == 1) {
    localX = (PANEL_WIDTH  - 1) - localX;
    localY = (PANEL_HEIGHT - 1) - localY;
  }

  int base = panel * PANEL_WIDTH * PANEL_HEIGHT;

  // Even columns run top->bottom, odd columns run bottom->top
  if (localX % 2 == 0) {
    return base + localX * PANEL_HEIGHT + localY;
  } else {
    return base + localX * PANEL_HEIGHT + (PANEL_HEIGHT - 1 - localY);
  }
}

bool skipRequested() {
  // Skip a test if 'n' is received by the Serial Monitor
  if (Serial.available() && Serial.read() == 'n') {
    while (Serial.available()) Serial.read(); // Drain remaining buffer chars
    return true;
  }
  return false;
}

void clearAll() {
  // Zero all LEDs (black display)
  FastLED.clear();
  FastLED.show();
}

// ---------- TESTS ----------

// 1. Light every LED individually in raw wire order
void testPixelWalk() {
  Serial.println("[1] Pixel walk (wire order). Follow the single dot's path.");
  for (int i = 0; i < NUM_LEDS; i++) {
    FastLED.clear();
    leds[i] = CRGB::White;
    FastLED.show();
    if (i % 32 == 0) Serial.printf("    LED %d / %d\n", i, NUM_LEDS);
    delay(50);
    if (skipRequested()) return;
  }
}

// 2. Row sweep: one clean horizontal line, top -> bottom
void testRowSweep() {
  Serial.println("[2] Row sweep. Expect one horizontal line moving top->bottom.");
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    FastLED.clear();
    for (int x = 0; x < MATRIX_WIDTH; x++) leds[XY(x, y)] = CRGB::Green;
    FastLED.show();
    Serial.printf("    Row %d / %d\n", y, MATRIX_HEIGHT - 1);
    delay(300);
    if (skipRequested()) return;
  }
}

// 3. Column sweep: one clean vertical line, left -> right
void testColumnSweep() {
  Serial.println("[3] Column sweep. Expect one vertical line moving left->right.");
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    FastLED.clear();
    for (int y = 0; y < MATRIX_HEIGHT; y++) leds[XY(x, y)] = CRGB::Cyan;
    FastLED.show();
    delay(80);
    if (skipRequested()) return;
  }
}

// 4. Panel identification: bottom panel RED, top panel BLUE
void testPanelID() {
  Serial.println("[4] Panel ID: BOTTOM half = RED, TOP half = BLUE.");
  FastLED.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      bool bottom = (y >= PANEL_HEIGHT);
      leds[XY(x, y)] = bottom ? CRGB::Red : CRGB::Blue;
    }
  }
  FastLED.show();
  for (int t = 0; t < 30; t++) { delay(100); if (skipRequested()) return; }
}

// 5. Corner markers: the four true corners of the 32x16 grid
void testCorners() {
  Serial.println("[5] Corners: (0,0)=RED  (31,0)=GREEN  (0,15)=BLUE  (31,15)=WHITE");
  Serial.println("    Red top-left, green top-right, blue bottom-left, white bottom-right.");
  FastLED.clear();
  leds[XY(0, 0)]                                 = CRGB::Red;    // top-left
  leds[XY(MATRIX_WIDTH - 1, 0)]                  = CRGB::Green;  // top-right
  leds[XY(0, MATRIX_HEIGHT - 1)]                 = CRGB::Blue;   // bottom-left
  leds[XY(MATRIX_WIDTH - 1, MATRIX_HEIGHT - 1)]  = CRGB::White;  // bottom-right
  FastLED.show();
  for (int t = 0; t < 40; t++) { delay(100); if (skipRequested()) return; }
}

// 6. Full white at low brightness
void testFullWhite() {
  Serial.println("[6] Full white, low brightness. Watch for resets or flicker.");
  Serial.println("    If it flickers or the board resets, power is insufficient -");
  Serial.println("    do NOT raise brightness on USB power.");
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  for (int t = 0; t < 30; t++) { delay(100); if (skipRequested()) return; }
}

// ---------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== WS2812B Stacked-Panel Test (32x16, 512 LEDs) ===");
  Serial.println("Layout: bottom panel = rows 8-15, top panel = rows 0-7.");
  Serial.println("Send 'n' at any time to skip to the next test.\n");

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500); // 1.5A @ 5V safety cap for USB testing
  clearAll();
}

void loop() {
  testPixelWalk();   clearAll(); delay(500);
  testRowSweep();    clearAll(); delay(500);
  testColumnSweep(); clearAll(); delay(500);
  testPanelID();     clearAll(); delay(500);
  testCorners();     clearAll(); delay(500);
  testFullWhite();   clearAll(); delay(500);
  Serial.println("\n--- Cycle complete, restarting ---\n");
}
