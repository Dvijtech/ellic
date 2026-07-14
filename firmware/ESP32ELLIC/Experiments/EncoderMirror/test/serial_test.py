import usb.core
import usb.util

dev = usb.core.find(idVendor=0x1209, idProduct=0x0D32)

print(dev)

if dev is None:
    print("Не найдено")
    exit()

try:
    dev.set_configuration()
    print("set_configuration OK")
except Exception as e:
    print("set_configuration:", e)

try:
    cfg = dev.get_active_configuration()
    print(cfg)
except Exception as e:
    print("get_active_configuration:", e)