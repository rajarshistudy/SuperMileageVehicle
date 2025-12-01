import asyncio
import websockets
import random
import time

async def handler(websocket, path):
    print(f"Client connected from {websocket.remote_address}")
    
    try:
        # Simulate speed data
        speed = 0
        while True:
            # Simulate acceleration/deceleration
            speed += random.uniform(-5, 8)
            speed = max(0, min(160, speed))  # Keep between 0-160 mph
            
            # Send speed to client
            await websocket.send(f"speed:{speed:.1f}")
            print(f"Sent speed: {speed:.1f} mph")
            
            # Wait before sending next update
            await asyncio.sleep(0.5)  # Update twice per second
            
    except websockets.exceptions.ConnectionClosed:
        print("Client disconnected")

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8765):
        print("WebSocket server started on ws://0.0.0.0:8765")
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())