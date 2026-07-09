import usb.core
from usb.backend import libusb1
import libusb_package

dll = libusb_package.get_library_path()
print("DLL:", dll)

backend = libusb1.get_backend(find_library=lambda x: dll)
print("Backend:", backend)

devs = list(usb.core.find(find_all=True, backend=backend))
print("Devices:", devs)