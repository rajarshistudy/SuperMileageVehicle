import pandas as pd
import numpy as np
from scipy.spatial import distance
import matplotlib as mlt
from pymavlink import mavutil

# Load track limits
track = pd.read_csv('sem-us-2022-track_coordinates.csv')

# Load vehicle telemetry (after converting from MAVLink)
# mavlogdump.py --types=GPS --format=csv log.bin > vehicle_gps.csv
vehicle = pd.read_csv('vehicle_gps.csv')

# Function to check if vehicle is on track
def is_within_track(vehicle_lat, vehicle_lon, track_coords, threshold_meters=5.0):
    """
    Check if vehicle position is within threshold distance of track boundary
    threshold_meters: acceptable distance from track boundary
    """
    # Convert lat/lon to approximate meters (rough approximation)
    # At Indianapolis latitude (39.79°), 1° lat ≈ 111km, 1° lon ≈ 85km
    
    vehicle_point = np.array([vehicle_lat, vehicle_lon])
    track_points = track[['Latitude', 'Longitude']].values
    
    # Find minimum distance to any track boundary point
    distances = distance.cdist([vehicle_point], track_points, metric='euclidean')[0]
    
    # Convert to meters (rough approximation)
    min_distance_deg = np.min(distances)
    min_distance_m = min_distance_deg * 111000  # Very rough approximation
    
    return min_distance_m < threshold_meters, min_distance_m

# Function to extract GPS data of the car from MAVLINK
def extract_gps_from_mavlink(log_file):
    """
    Extract GPS data from MAVLink log file (.tlog or .bin)
    Returns a pandas DataFrame with GPS coordinates
    """
    # Open the log file
    mlog = mavutil.mavlink_connection(log_file)
    
    # Lists to store data
    gps_data = []
    
    print(f"Reading {log_file}...")
    
    # Read all messages
    while True:
        msg = mlog.recv_match(type='GPS', blocking=False)
        
        if msg is None:
            break
        
        # Extract GPS data
        gps_data.append({
            'timestamp': msg._timestamp,
            'latitude': msg.Lat,
            'longitude': msg.Lng,
            'altitude': msg.Alt,
            'ground_speed': msg.Spd,
            'ground_course': msg.GCrs,
            'num_satellites': msg.NSats,
            'hdop': msg.HDop,
            'vertical_speed': msg.VZ
        })
    
    # Convert to DataFrame
    df = pd.DataFrame(gps_data)
    
    # Convert timestamp to datetime
    if len(df) > 0:
        df['datetime'] = pd.to_datetime(df['timestamp'], unit='s')
    
    print(f"Extracted {len(df)} GPS data points")
    
    return df


# Example: Check each vehicle position
for idx, row in vehicle.iterrows():
    on_track, dist = is_within_track(
        row['GPS.Lat'], 
        row['GPS.Lng'], 
        track
    )
    print(f"Time {row['timestamp']}: On track: {on_track}, Distance: {dist:.2f}m")
