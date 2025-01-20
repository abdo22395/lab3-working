import asyncio
import mariadb
from bleak import BleakClient

# BLE device information
address = "50:36:2C:B2:FF:83"
MODEL_NBR_UUID = "00002a24-0000-1000-8000-00805f9b34fb"
HR_NOTIFY_UUID = "00002a37-0000-1000-8000-00805f9b34fb"

# MariaDB connection details
db_config = {
    'host': 'localhost',  # or the IP of the Raspberry Pi
    'port': 3306,
    'user': 'Daytoo',
    'password': 'Magnfacnt123',
    'database': 'sensor_data'
}

# Connect to the MariaDB database
def connect_to_db():
    try:
        conn = mariadb.connect(**db_config)
        return conn
    except mariadb.Error as e:
        print(f"Error connecting to MariaDB: {e}")
        return None

# Store heart rate data in the database
def store_heart_rate_data(heart_rate):
    conn = connect_to_db()
    if conn:
        cursor = conn.cursor()
        try:
            query = "INSERT INTO heart_rate_data (heart_rate) VALUES (?)"
            cursor.execute(query, (heart_rate,))
            conn.commit()
            print(f"Stored heart rate: {heart_rate}")
        except mariadb.Error as e:
            print(f"Error storing data: {e}")
        finally:
            cursor.close()
            conn.close()

# Notification handler for heart rate data
async def notification_handler(characteristic, data):
    heart_rate = int.from_bytes(data[1:], "big", signed=True)
    print(f"Received heart rate data: {heart_rate}")
    store_heart_rate_data(heart_rate)

# Main function to read data from the BLE device and notify the database
async def main(address):
    async with BleakClient(address) as client:
        model_number = await client.read_gatt_char(MODEL_NBR_UUID)
        print("Model Number: {0}".format("".join(map(chr, model_number))))
        
        await client.start_notify(HR_NOTIFY_UUID, notification_handler)
        
        await asyncio.sleep(30.0)  # Collect data for 30 seconds
        
        await client.stop_notify(HR_NOTIFY_UUID)

# Run the main function
asyncio.run(main(address))
