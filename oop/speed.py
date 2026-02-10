import struct
import csv
import sys
import math


def extract_speed(tlog_file, output_csv):
    
    with open(tlog_file, 'rb') as f:
        data = f.read()
    
    speed_data = []
    i = 0
    
    while i < len(data):
        # Skip tlog timestamp (6 bytes: 2 reserved + 4 timestamp)
        if i + 6 > len(data):
            break
        timestamp_us = struct.unpack('>I', data[i+2:i+6])[0]
        timestamp_s = timestamp_us / 1e6
        i += 6
        
        # Check for MAVLink v2 packet (starts with 0xFD)
        if i >= len(data) or data[i] != 0xFD:
            i += 1
            continue
        
        # Parse MAVLink v2 header
        if i + 10 > len(data):
            break
            
        plen = data[i + 1]
        incomp = data[i + 2]
        sig_len = 13 if (incomp & 0x01) else 0
        msg_id = data[i + 7] | (data[i + 8] << 8) | (data[i + 9] << 16)
        
        total_len = 10 + plen + 2 + sig_len
        if i + total_len > len(data):
            i += 1
            continue
        
        payload = data[i + 10:i + 10 + plen]
        
        # Extract speed from GLOBAL_POSITION_INT (msg_id = 33)
        if msg_id == 33 and len(payload) >= 28:
            # vx, vy at bytes 20-24 (i16, i16) in cm/s
            vx, vy = struct.unpack('<hh', payload[20:24])
            speed = math.sqrt((vx/100.0)**2 + (vy/100.0)**2)  # Convert to m/s
            speed_data.append({'timestamp_s': timestamp_s, 'speed_ms': speed})
        
        # Extract speed from VFR_HUD (msg_id = 74)
        elif msg_id == 74 and len(payload) >= 8:
            # groundspeed at bytes 4-8 (float)
            speed = struct.unpack('<f', payload[4:8])[0]
            speed_data.append({'timestamp_s': timestamp_s, 'speed_ms': speed})
        
        i += total_len
    
    # Write to CSV
    with open(output_csv, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['timestamp_s', 'speed_ms'])
        writer.writeheader()
        writer.writerows(speed_data)
    
    print(f"Extracted {len(speed_data)} speed measurements")
    if speed_data:
        speeds = [r['speed_ms'] for r in speed_data]
        print(f"Min: {min(speeds):.2f} m/s")
        print(f"Max: {max(speeds):.2f} m/s")
        print(f"Avg: {sum(speeds)/len(speeds):.2f} m/s")
    print(f"Saved to {output_csv}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 extract_speed_only.py <tlog_file> [output.csv]")
        sys.exit(1)
    
    tlog_file = sys.argv[1]
    output_csv = sys.argv[2] if len(sys.argv) > 2 else 'speed_only.csv'
    
    extract_speed(tlog_file, output_csv)