import asyncio
from bleak import BleakClient
import mariadb

# Database connection parameters
DB_HOST = "localhost"
DB_USER = "Daytoo"
DB_PASSWORD = "Magnfacnt123"
DB_NAME = "lab3_table"

address = "5A:0C:0E:A8:54:A2"
MODEL_NBR_UUID = "00002a24-0000-1000-8000-00805f9b34fb"
HR_NOTIFY_UUID = "00002a37-0000-1000-8000-00805f9b34fb"

async def write_to_db(heart_rate):
    try:
        conn = mariadb.connect(
            user=DB_USER,
            password=DB_PASSWORD,
            host=DB_HOST,
            database=DB_NAME
        )
        cursor = conn.cursor()
        cursor.execute("INSERT INTO heart_rate_data (heart_rate) VALUES (?)", (heart_rate,))
        conn.commit()
    except mariadb.Error as e:
        print(f"Error: {e}")
    finally:
        cursor.close()
        conn.close()

async def main(address):
    async with BleakClient(address) as client:
        model_number = await client.read_gatt_char(MODEL_NBR_UUID)
        print("Model Number: {0}".format("".join(map(chr, model_number))))
        
        async def notification_handler(characteristic, data):
            heart_rate = int.from_bytes(data[1:], "big", signed="True")
            print(f"{characteristic.description}: {data} | Heart Rate: {heart_rate}")
            await write_to_db(heart_rate)  # Write heart rate to database

        await client.start_notify(HR_NOTIFY_UUID, notification_handler)
        await asyncio.sleep(30.0)
        await client.stop_notify(HR_NOTIFY_UUID)

asyncio.run(main(address))
