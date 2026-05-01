import serial
import serial.tools.list_ports
import time
import os
import argparse
from PIL import Image, ImageOps
import io

# --- FUNCTIONS ---

# Auto-detect M5Stick port (macOS)
def find_m5stick_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "usbserial" in p.device or "usbmodem" in p.device:
            return p.device
    return None

# Process and send image to M5Stick
def send_image(ser, image_path):
    try:
        # 1. Load & Convert
        img = Image.open(image_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        # 2. Resize/Crop to 240x135
        target_size = (240, 135)
        img = ImageOps.fit(img, target_size, method=Image.Resampling.LANCZOS)
        
        # 3. Compress to JPEG
        img_byte_arr = io.BytesIO()
        img.save(img_byte_arr, format='JPEG', quality=85)
        img_bytes = img_byte_arr.getvalue()
        image_size = len(img_bytes)
        
        print(f"Sending {os.path.basename(image_path)} ({image_size} bytes)...")

        # 4. Send Header (Size)
        header = f"{image_size}\n".encode()
        ser.write(header)
        
        # 5. Wait for Ack ('A')
        start_wait = time.time()
        ack_received = False
        while time.time() - start_wait < 3.0:
            if ser.in_waiting:
                if ser.read() == b'A':
                    ack_received = True
                    break
        
        # Timeout check
        if not ack_received:
            print("  -> Timed out waiting for Ack. Skipping.")
            ser.reset_input_buffer()
            return

        # 6. Send Image Data
        ser.write(img_bytes)
        
        # 7. Wait for Done ('D')
        while True:
            if ser.in_waiting:
                if ser.read() == b'D':
                    break
            
        print("  -> Success.")

    except Exception as e:
        print(f"Error sending image: {e}")

# --- MAIN ---

def main():
    parser = argparse.ArgumentParser(description='M5Stick Photo Frame Sender')
    parser.add_argument('--port', help='Serial port (Optional)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--folder', required=True, help='Image folder path')
    parser.add_argument('--interval', type=int, default=5, help='Seconds between images')
    args = parser.parse_args()

    # Auto-detect port if needed
    port_name = args.port
    if not port_name:
        print("Scanning for M5Stick...")
        port_name = find_m5stick_port()
        if port_name:
            print(f"Auto-detected: {port_name}")
        else:
            print("Error: Device not found. Specify --port manually.")
            return

    # Validate folder
    abs_folder = os.path.abspath(args.folder)
    if not os.path.exists(abs_folder):
        print(f"Error: Folder not found: {abs_folder}")
        return

    # Find valid images
    valid_extensions = ('.jpg', '.jpeg', '.png', '.bmp')
    images = [os.path.join(abs_folder, f) for f in os.listdir(abs_folder) if f.lower().endswith(valid_extensions)]
    
    if not images:
        print(f"No images found in {abs_folder}!")
        return

    print(f"Found {len(images)} images. Connecting to {port_name}...")

    try:
        ser = serial.Serial(port_name, args.baud, timeout=1)
        print("Waiting 3s for reset...")
        time.sleep(3) 
        
        while True:
            for img_path in images:
                send_image(ser, img_path)
                
                # Display duration
                print(f"  -> Displaying for {args.interval} seconds...")
                time.sleep(args.interval)
                
    except serial.SerialException:
        print("Serial Error: Check connection or close Arduino IDE.")
    except KeyboardInterrupt:
        print("\nStopping.")
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()