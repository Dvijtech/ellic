import odrive
import time

def connect():
    print("Connecting...")
    odrv = odrive.find_any()
    print("Connected:", odrv.serial_number)
    return odrv