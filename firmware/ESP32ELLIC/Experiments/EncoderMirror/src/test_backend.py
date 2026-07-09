from usb.backend import libusb1

dll = r"C:\Users\Dim\YandexDisk\ELLIC\ellic\.venv\Lib\site-packages\libusb_package\libusb-1.0.dll"

backend = libusb1.get_backend(find_library=lambda x: dll)
print("Backend:", backend)