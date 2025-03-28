import serial
import threading

# Define the COM ports and their settings
COM_PORT_1 = 'COM9'  # Replace with your first COM port
COM_PORT_2 = 'COM4'  # Replace with your second COM port
BAUD_RATE = 9600     # Baud rate (adjust as needed)
TIMEOUT = 1          # Timeout for reading (in seconds)

# Function to read from one port and write to another
def bridge_ports(port_in, port_out):
    try:
        while True:
            # Read data from the input port
            data = port_in.read(port_in.in_waiting or 1)
            if data:
                # Write the data to the output port
                port_out.write(data)
                print(f"Bridged data: {data}")  # Optional: Print the bridged data
    except KeyboardInterrupt:
        print("Bridging stopped.")

# Open the COM ports
try:
    com1 = serial.Serial(COM_PORT_1, BAUD_RATE, timeout=TIMEOUT)
    com2 = serial.Serial(COM_PORT_2, BAUD_RATE, timeout=TIMEOUT)
    print(f"Opened {COM_PORT_1} and {COM_PORT_2} for bridging.")

    # Create threads to bridge data in both directions
    thread1 = threading.Thread(target=bridge_ports, args=(com1, com2))
    thread2 = threading.Thread(target=bridge_ports, args=(com2, com1))

    # Start the threads
    thread1.start()
    thread2.start()

    # Wait for the threads to finish (they won't unless interrupted)
    thread1.join()
    thread2.join()

except serial.SerialException as e:
    print(f"Error opening COM ports: {e}")
finally:
    # Close the COM ports when done
    if 'com1' in locals():
        com1.close()
    if 'com2' in locals():
        com2.close()
    print("COM ports closed.")