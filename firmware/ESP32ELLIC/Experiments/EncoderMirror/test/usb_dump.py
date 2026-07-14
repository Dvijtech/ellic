import usb.core
import usb.backend.libusb1
import libusb_package

backend = usb.backend.libusb1.get_backend(
    find_library=lambda x: libusb_package.get_library_path()
)

dev = usb.core.find(
    idVendor=0x1209,
    idProduct=0x0D32,
    backend=backend
)

print(dev)

if dev:
    print("Configuration:", dev.bNumConfigurations)

    for cfg in dev:
        print("CFG", cfg.bConfigurationValue)

        for intf in cfg:
            print(
                "Interface",
                intf.bInterfaceNumber,
                "Alt",
                intf.bAlternateSetting
            )

            for ep in intf:
                print(
                    "Endpoint",
                    hex(ep.bEndpointAddress),
                    ep.bmAttributes
                )