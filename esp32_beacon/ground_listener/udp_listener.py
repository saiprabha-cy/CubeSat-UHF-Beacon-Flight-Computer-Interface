"""
udp_listener.py

Host-side UDP listener for the ESP32 beacon. Decodes frames using the
EXACT same protocol as Stage 1's host-verified C implementation
(stm32_firmware) and the ESP32 firmware below -- same SYNC/TYPE/LENGTH/
PAYLOAD/CRC16 format, so "same protocol everywhere" is provably true.

This decode logic was verified independently (round-trip build+decode
test, see Stage 6 chat history) before being used here to listen for
real hardware -- isolates "is my decode correct" from "did the ESP32
actually send something," so if this listener ever fails, you know the
problem is on the ESP32/network side, not here.

Run: python3 udp_listener.py
(listens on UDP port 5005 by default -- must match the ESP32 sketch's
UDP_PORT setting)
"""

import socket
import struct
import time

UDP_PORT = 5005
PROTOCOL_SYNC1 = 0xAA
PROTOCOL_SYNC2 = 0x55
FRAME_TYPE_TELEMETRY = 0x01


def crc16_update(crc, byte):
    crc ^= (byte << 8)
    for _ in range(8):
        if crc & 0x8000:
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF
        else:
            crc = (crc << 1) & 0xFFFF
    return crc


def crc16_buffer(data):
    crc = 0xFFFF
    for b in data:
        crc = crc16_update(crc, b)
    return crc


def decode_frame(frame):
    """Returns a dict on success, or None (with a printed reason) on
    any validation failure -- never raises, since a real listener must
    survive a single corrupted/truncated UDP packet and keep listening."""
    if len(frame) < 6:
        print(f"  [reject] frame too short ({len(frame)} bytes)")
        return None
    if frame[0] != PROTOCOL_SYNC1 or frame[1] != PROTOCOL_SYNC2:
        print(f"  [reject] bad sync bytes: {frame[0]:02x} {frame[1]:02x}")
        return None

    ftype = frame[2]
    flen = frame[3]
    expected_total = 4 + flen + 2
    if len(frame) != expected_total:
        print(f"  [reject] length mismatch: frame={len(frame)}B, expected={expected_total}B")
        return None

    payload = frame[4:4+flen]
    body = frame[2:4+flen]
    crc_received = frame[4+flen] | (frame[5+flen] << 8)
    crc_computed = crc16_buffer(body)
    if crc_received != crc_computed:
        print(f"  [reject] CRC mismatch: received={crc_received:04x}, computed={crc_computed:04x}")
        return None

    if ftype == FRAME_TYPE_TELEMETRY:
        if flen != 34:
            print(f"  [reject] telemetry payload wrong length: {flen} (expected 34)")
            return None
        ts, att_err, wx, wy, wz, hx, hy, hz, seq = struct.unpack('<IfffffffH', payload)
        return {
            'type': 'TELEMETRY',
            'timestamp_ms': ts,
            'attitude_error_deg': att_err,
            'omega': (wx, wy, wz),
            'h_wheel': (hx, hy, hz),
            'seq': seq,
        }
    else:
        print(f"  [info] non-telemetry frame type: 0x{ftype:02x}")
        return {'type': f'0x{ftype:02x}', 'raw_payload': payload.hex()}


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', UDP_PORT))
    print(f"Listening for beacon telemetry on UDP port {UDP_PORT}...")
    print("(Ctrl+C to stop)\n")

    frame_count = 0
    last_seq = None

    try:
        while True:
            data, addr = sock.recvfrom(1024)
            decoded = decode_frame(data)
            ts_now = time.strftime('%H:%M:%S')
            if decoded is None:
                print(f"[{ts_now}] from {addr[0]}: FAILED TO DECODE ({len(data)} bytes raw)")
                continue

            frame_count += 1
            if decoded['type'] == 'TELEMETRY':
                seq = decoded['seq']
                drop_note = ''
                if last_seq is not None and seq != (last_seq + 1) & 0xFFFF:
                    drop_note = f'  [[GAP: expected seq {(last_seq+1)&0xFFFF}, got {seq}]]'
                last_seq = seq
                print(f"[{ts_now}] seq={seq:5d} err={decoded['attitude_error_deg']:.4f}deg "
                      f"omega={decoded['omega']} h_wheel={decoded['h_wheel']}{drop_note}")
            else:
                print(f"[{ts_now}] {decoded}")
    except KeyboardInterrupt:
        print(f"\nStopped. Received {frame_count} valid frames.")


if __name__ == '__main__':
    main()
