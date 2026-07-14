# Power Architecture

## Design Principle

**Panel current never flows through the ESP32.** The dev board's 5V pin and traces are not a power bus for 512 LEDs — routing panel current through it risks brownout resets and long-term board damage.

**The fix:** two independent current paths from the same source.

## Supply

A **USB power bank with two USB-A ports, rated 5V @ 3A**.

> Both ports share the bank's internal ground plane, so the ESP32 and panels have a **common ground by construction**, which is what makes the WS2812B data line work.

## Topology

```
                    ┌─────────────────────┐
                    │   USB Power Bank    │
                    │     5V @ 3A         │
                    │  Port 1     Port 2  │
                    └────┬───────────┬────┘
                         │           │
        USB-A pigtail ───┘           └─── USB-A to USB-C cable
        (2-pin bare wire)
                 │                              │
      ┌──────────┴──────────┐                   ▼
      │  Solder joint       │            ┌─────────────┐
      │  + heat shrink      │            │   ESP32     │
      └──┬───────────────┬──┘            │  dev board  │
         │               │               └──────┬──────┘
      ┌──┴──────┐   ┌────┴────┐                 │
      │ Panel 0 │   │ Panel 1 │                 │ GPIO 32
      │ +5V/GND │   │ +5V/GND │                 │ (data)
      └────┬────┘   └─────────┘                 │
           │                                    │
           │  DIN ◄─────────────────────────────┘
           │
           └──── DOUT ────► Panel 1 DIN  (data chain)

           ESP32 GND ────────► common ground rail
```

[Wiring setup](images/wiring_setup.png)

**Port 1 → Panels.** A USB-A male to 2-pin bare wire pigtail (5V/3A). The red and black wires are each **split into two branches** via solder joints, insulated with heat shrink, and run to each panel's power injection wires:

```
Pigtail red   ──┬── Panel 0 (+5V)
                └── Panel 1 (+5V)

Pigtail black ──┬── Panel 0 (GND)
                └── Panel 1 (GND)
```

[Custom cable](images/custom_cable_.png)

**Port 2 → ESP32.** A standard USB-A to USB-C cable into the dev board's USB port. Do *not* also feed the 5V pin — driving both paths risks backfeed.

## Current Budget

At full brightness (bright white), each of a WS2812B's red, green, and blue segments draws roughly **20mA**, so a single LED can pull about **60mA**. The theoretical worst case for the full display:

```text
512 LEDs × 60 mA = 30,720 mA ≈ 30.7 A  @ 5 V  (~154 W)
```

In practice, an audio visualizer rarely lights every LED white at once. With mixed colours and moving patterns, average current is typically around 20 mA per LED (about one-third of the theoretical maximum). To ensure safe operation under all conditions, the firmware enforces a hard current limit rather than relying on this average.

## Firmware Power Governor

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
```

This caps draw at 2000mA (inside the bank's 3A rating) and auto-dims any frame that would exceed it, making brownout structurally impossible even if a frame would otherwise approach the 30A worst case. Update the value if the supply changes.

> Source: [Last Minute Engineers — Controlling WS2812B Addressable LEDs with Arduino](https://lastminuteengineers.com/ws2812b-arduino-tutorial/).

## Bill of Materials

| Component | Spec | Qty | Link |
|---|---|---|---|
| Power bank | USB-A ×2, 5V @ 3A | 1 | [Amazon.ca](https://www.amazon.ca/dp/B09176JCKZ) |
| USB pigtail cable | USB-A male → 2-pin bare wire, 11in, 5V/3A | 1 | [Amazon.ca](https://www.amazon.ca/dp/B0BS6WHP3R) |
| LED panel | WS2812B flexible matrix, 8×32 | 2 | [Amazon.ca](https://www.amazon.ca/dp/B07P5TSKX8) |
| ESP32 dev board | Freenove ESP32-WROOM (ESP32-D0WD-V3) | 1 | [Amazon.ca](amazon.ca/dp/B0C9THDPXP?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1) |
| USB-A to USB-C cable | Standard | 1 | (Comes with ESP32 dev board) |
| Heat shrink tubing | For pigtail solder joints | 4 | [Amazon.ca](https://www.amazon.ca/dp/B0BTKSQ8NS/ref=sspa_dk_detail_4?pd_rd_i=B0BTKSQ8NS&pd_rd_w=dU9PE&content-id=amzn1.sym.453395a9-a1f7-4b76-acdc-9e8eaa87e9be&pf_rd_p=453395a9-a1f7-4b76-acdc-9e8eaa87e9be&pf_rd_r=MZS1YWTVQK4XCPVM1PES&pd_rd_wg=DyZbZ&pd_rd_r=7a129894-9fb1-4109-b265-de4a41b0e972&s=industrial&sp_csd=d2lkZ2V0TmFtZT1zcF9kZXRhaWxfdGhlbWF0aWM&th=1) |
| Jumper wire | Male-to-female (ESP32 header pin → panel connector) | 2 | [Amazon.ca](https://www.amazon.ca/dp/B07KLM9KR1?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_3) |
