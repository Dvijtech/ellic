при установке odesc 4.2. сначала надо через программатор залить на них firmware.elf

потом командой odrivetool входим в редактор 

# проверка версии 
print(f"{dev0.fw_version_major}.{dev0.fw_version_minor}.{dev0.fw_version_revision}")
ожидаемая версия 0.5.6

# Версия оборудования: Если нужно узнать версию платы ODrive, используйте команды 
dev0.hw_version_major и dev0.hw_version_minor
ожидаемая версия 3.6

