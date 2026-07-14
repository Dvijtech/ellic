import usb.core
import usb.util
from usb.backend import libusb1

dll = r"C:\Users\Dim\YandexDisk\ELLIC\ellic\.venv\Lib\site-packages\libusb_package\libusb-1.0.dll"

backend = libusb1.get_backend(find_library=lambda x: dll)

dev = usb.core.find(idVendor=0x1209, idProduct=0x0D32, backend=backend)

print(dev)

try:
    dev.set_configuration()
    usb.util.claim_interface(dev, 2)
    print("SUCCESS")
except Exception as e:
    print(type(e).__name__)
    print(e)