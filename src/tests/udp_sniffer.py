"""
udp_sniffer.py - Inspect spectrum's PC -> ESP32 UDP frames.

Wire Format:
    payload[0]     mode        0 = spectrum bars, 1 = oscilloscope trace
    payload[1..3]  R, G, B     theme colour (0-255)
    payload[4..35] h0..h31     32 column heights (0-255)

Usage:
    python udp_sniffer.py                   # listen on 0.0.0.0:4210 (default)
    python udp_sniffer.py --host 127.0.0.1  # bind one interface (e.g. loopback)
    python udp_sniffer.py --port 4210
    python udp_sniffer.py --raw             # also dump raw hex per frame

NOTE: Run this INSTEAD of the ESP32 receiving, or point ESP32_IP in
udp_sender.hpp at this PC's address temporarily. Two receivers can't
share the same unicast datagram stream.
"""

import argparse
import socket
import time

# Frame layout
HEADER_BYTES = 4                              # mode byte + R,G,B
N_LED_COLS   = 32                             # one height byte per column
PAYLOAD_SIZE = HEADER_BYTES + N_LED_COLS      # expected datagram size, in bytes (36)

# Human-readable labels for output
MODE_NAMES = {0: "spectrum", 1: "oscilloscope"}

def main() -> int:
    # Command-line options
    ap = argparse.ArgumentParser(description="Decode spectrum UDP frames")
    ap.add_argument("--host", default="0.0.0.0", help="bind address (default 0.0.0.0)")
    ap.add_argument("--port", type=int, default=4210, help="UDP port (default 4210, matches ESP32_PORT)")
    ap.add_argument("--raw", action="store_true", help="print raw hex of each frame")
    args = ap.parse_args()

    # Open a UDP socket and claim the port (use IPv4 addressing)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((args.host, args.port))
    except OSError as e:
        # Binding fails if the ESP32 firmware or another sniffer already holds it
        print(f"bind failed on {args.host}:{args.port} - {e}")
        return 1

    print(f"Listening on {args.host}:{args.port} - expecting {PAYLOAD_SIZE}-byte frames.\n")

    # Running totals plus the state for the 1-second FPS window.
    frames = 0                          # every datagram received
    bad = 0                             # subset that failed the size check
    window_start = time.monotonic()     # start of the current FPS window
    window_frames = 0                   # frames counted in the current window
    fps = 0.0                           # last computed rate, shown per frame

    try:
        while True:
            # Block until a datagram arrives (2048 is an ample recv buffer).
            data, addr = sock.recvfrom(2048)
            frames += 1
            window_frames += 1

            # Rolling frames-per-second estimate over 1 s windows: once a full
            # second has elapsed, divide frames by elapsed time and reset.
            now = time.monotonic()
            if now - window_start >= 1.0:
                fps = window_frames / (now - window_start)
                window_start = now
                window_frames = 0

            # Reject anything that isn't exactly one frame
            if len(data) != PAYLOAD_SIZE:
                bad += 1
                print(f"[!] frame {frames}: BAD SIZE {len(data)} bytes "
                      f"(expected {PAYLOAD_SIZE}) from {addr[0]}:{addr[1]}")
                if args.raw:
                    print("hex:", data.hex(" "))
                continue

            # Datagram frame fields
            mode    = data[0]                       # 0 = spectrum, 1 = oscilloscope
            r, g, b = data[1], data[2], data[3]     # theme colour
            heights = list(data[HEADER_BYTES:])     # the 32 column heights

            # One summary line per frame: rate, mode, colour, and height stats
            mode_name = MODE_NAMES.get(mode, f"unknown({mode})")
            print(f"frame {frames:6d} | {fps:5.1f} fps | mode={mode_name:<12} "
                  f"| rgb=({r:3d},{g:3d},{b:3d}) "
                  f"| h: min={min(heights):3d} max={max(heights):3d} "
                  f"avg={sum(heights)/len(heights):6.1f}")

            # Optional full hex dump of the raw datagram for byte-level debugging
            if args.raw:
                print("hex:", data.hex(" "))

    except KeyboardInterrupt:
        # Print a summary tally and exit
        print(f"\nStopped. {frames} frames total, {bad} malformed.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())