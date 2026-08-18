import asyncio

from bleak import BleakClient, BleakScanner


DEVICE_NAME = "ESP32 Steering Wheel"
DEVICE_ADDRESS = "C0:CD:D6:D0:1E:EE"


async def main():

    print()
    print("==============================================")
    print("ELLIC BLE GATT TEST")
    print("==============================================")
    print()

    print("Scanning...")

    devices = await BleakScanner.discover(timeout=10)

    device = None

    for d in devices:

        print(
            f"  {d.name!r} [{d.address}]"
        )

        if d.address.upper() == DEVICE_ADDRESS:
            device = d

    if device is None:

        print()
        print("ESP32 NOT FOUND")
        return

    print()
    print("FOUND:")
    print(f"  Name:    {device.name}")
    print(f"  Address: {device.address}")
    print()

    print("Connecting...")
    print()

    try:

        client = BleakClient(
            device,
            timeout=30.0
        )

        await client.connect()

        print("CONNECTED")
        print()

        print("Connected:", client.is_connected)
        print()

        print("GATT SERVICES:")
        print("----------------------------------------------")

        for service in client.services:

            print()
            print(f"SERVICE: {service.uuid}")
            print(f"  Description: {service.description}")

            for characteristic in service.characteristics:

                print(
                    f"    CHARACTERISTIC: {characteristic.uuid}"
                )

                print(
                    f"      Properties: "
                    f"{', '.join(characteristic.properties)}"
                )

        print()
        print("----------------------------------------------")
        print("GATT DISCOVERY COMPLETE")
        print()

        await client.disconnect()

    except Exception as e:

        print()
        print("==============================================")
        print("ERROR")
        print("==============================================")
        print()

        print(type(e).__name__)
        print(str(e))

        print()

        import traceback
        traceback.print_exc()


if __name__ == "__main__":

    asyncio.run(main())