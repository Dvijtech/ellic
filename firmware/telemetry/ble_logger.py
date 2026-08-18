import asyncio
import csv
from datetime import datetime
from pathlib import Path

from bleak import BleakClient, BleakScanner


# ============================================================
# ELLIC BLE TELEMETRY
# ============================================================

DEVICE_NAME = "ESP32 Steering Wheel"

SERVICE_UUID = "a1b2c3d0-1000-4000-8000-00805f9b34fb"

CHARACTERISTIC_UUID = "a1b2c3d1-1000-4000-8000-00805f9b34fb"


# ============================================================
# DATA DIRECTORY
# ============================================================

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"

DATA_DIR.mkdir(parents=True, exist_ok=True)


# Один файл на запуск программы
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

RAW_FILE = DATA_DIR / f"telemetry_{timestamp}.bin"
LOG_FILE = DATA_DIR / f"telemetry_{timestamp}.csv"


# ============================================================
# STATISTICS
# ============================================================

packet_count = 0
total_bytes = 0

start_time = None


# ============================================================
# BLE NOTIFICATION
# ============================================================

def notification_handler(sender, data: bytearray):
    global packet_count
    global total_bytes

    now = datetime.now()

    packet_count += 1
    total_bytes += len(data)

    # --------------------------------------------------------
    # SAVE RAW BINARY DATA
    # --------------------------------------------------------

    with open(RAW_FILE, "ab") as f:
        f.write(data)

    # --------------------------------------------------------
    # SAVE LOG INFORMATION
    # --------------------------------------------------------

    with open(LOG_FILE, "a", newline="") as f:
        writer = csv.writer(f)

        writer.writerow([
            now.isoformat(timespec="milliseconds"),
            packet_count,
            len(data),
            data.hex(" ")
        ])

    # --------------------------------------------------------
    # TERMINAL
    # --------------------------------------------------------

    print(
        f"[{now.strftime('%H:%M:%S.%f')[:-3]}] "
        f"PACKET #{packet_count:06d} "
        f"{len(data):2d} bytes"
    )


# ============================================================
# SCAN
# ============================================================

async def find_device():

    print()
    print("==============================================")
    print("ELLIC BLE TELEMETRY LOGGER")
    print("==============================================")
    print()

    print("Scanning for BLE devices...")
    print(f"Looking for: {DEVICE_NAME}")
    print()

    devices = await BleakScanner.discover(timeout=10)

    for device in devices:

        print(
            f"  {device.name!r} "
            f"[{device.address}]"
        )

        if device.name == DEVICE_NAME:
            print()
            print("FOUND ELLIC ESP32")
            print(f"Address: {device.address}")
            print()

            return device

    return None


# ============================================================
# MAIN BLE LOOP
# ============================================================

async def main():

    global start_time

    device = await find_device()

    if device is None:

        print()
        print("ERROR: ESP32 Steering Wheel not found.")
        print()
        print("Check:")
        print("  1. ESP32 is powered.")
        print("  2. BLE is running.")
        print("  3. ESP32 is not connected to another device.")
        print("  4. Bluetooth is enabled on the PC.")
        print()

        return

    print("Connecting...")

    async with BleakClient(
        device.address,
        timeout=30.0,
        services={SERVICE_UUID}
    ) as client:

        print()

        if not client.is_connected:
            print("ERROR: BLE connection failed.")
            return

        print("CONNECTED")
        print()

        print("Telemetry characteristic:")
        print(CHARACTERISTIC_UUID)
        print()

        # ----------------------------------------------------
        # START LOG FILE
        # ----------------------------------------------------

        with open(LOG_FILE, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "timestamp",
                "packet_number",
                "packet_size",
                "raw_hex"
            ])

        # ----------------------------------------------------
        # START TELEMETRY
        # ----------------------------------------------------

        print("Subscribing to telemetry notifications...")

        await client.start_notify(
            CHARACTERISTIC_UUID,
            notification_handler
        )

        print()
        print("==============================================")
        print("TELEMETRY ACTIVE")
        print("==============================================")
        print()
        print(f"RAW: {RAW_FILE}")
        print(f"LOG: {LOG_FILE}")
        print()
        print("Waiting for packets...")
        print("Press Ctrl+C to stop.")
        print()

        start_time = datetime.now()

        try:

            while True:

                await asyncio.sleep(1)

                elapsed = (
                    datetime.now() - start_time
                ).total_seconds()

                if elapsed > 0:

                    rate = packet_count / elapsed

                    print(
                        f"  packets={packet_count} "
                        f"bytes={total_bytes} "
                        f"rate={rate:.2f} pkt/s",
                        end="\r"
                    )

        except asyncio.CancelledError:
            pass

        finally:

            try:
                await client.stop_notify(
                    CHARACTERISTIC_UUID
                )
            except Exception:
                pass


# ============================================================
# PROGRAM ENTRY
# ============================================================

if __name__ == "__main__":

    try:
        asyncio.run(main())

    except KeyboardInterrupt:

        print()
        print()
        print("LOGGER STOPPED")

        print()
        print(f"Packets received: {packet_count}")
        print(f"Bytes received:   {total_bytes}")

        if start_time is not None:

            elapsed = (
                datetime.now() - start_time
            ).total_seconds()

            if elapsed > 0:

                print(
                    f"Average rate:      "
                    f"{packet_count / elapsed:.2f} pkt/s"
                )

        print()
        print(f"RAW DATA: {RAW_FILE}")
        print(f"LOG FILE: {LOG_FILE}")
        print()