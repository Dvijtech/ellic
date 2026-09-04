# Перейти в корень репозитория
cd C:\Users\Dim\YandexDisk\ELLIC\ellic

# Получить последние изменения с GitHub
git pull origin main --rebase

# После перемещения проверить статус Git
git status
# Будут показаны удалённые файлы (из старых мест) и новые (в media)

# Добавить все изменения в индекс
git add -A

# Сделать коммит
git commit -m "Commit"

# Отправить на GitHub
git push origin main


# выводит и сохраняет в кэш все файлы репозитория с путями
git ls-files --cached --others --exclude-standard | ForEach-Object { Split-Path $_ -Parent } | Where-Object { $_ } | Sort-Object -Unique 

# Сохраняет в кэш все тексты .md
git ls-files -- '*.md' | ForEach-Object { Get-Content $_ } | Set-Clipboard

скопироать в кэш
| Set-Clipboard

посмотреть что в гитигноре
Get-Content .gitignore

# выводит даты изменений по файлу
git log --pretty=format:"%h %ar %s" --date=relative cad/fusion/PARTS/

642651a 1773822342 Change name wheels compons
bb9062f 1773822208 start to razbirat components of model fusion

# Сохранение версии файла по пути и новому имени
PS C:\Users\Dim\YandexDisk\ELLIC\ellic> git show 642651a:"cad/fusion/PARTS/WHEELS/Wheel_d24_nonMotored.f3d" >cad/fusion/PARTS/WHEELS/Wheel_d24_nonMotored_642651a.f3d

# Сохранить тексты файлов:
    Get-ChildItem -Recurse -Include *.cpp,*.h | ForEach-Object { "=== $($_.Name) ==="; Get-Content $_.FullName; "`n" } | Out-File output.txt


# .venv
cd C:\Users\Dim\YandexDisk\ELLIC\ellic
.venv\Scripts\activate

# bluetooth
cd C:\Users\Dim\YandexDisk\ELLIC\ellic\firmware\telemetry
python ble_logger.py


# odrivetool

dev0.axis0.error
dev0.axis0.motor.error
dev0.axis0.encoder.error
dev0.axis0.controller.error

dump_errors(dev0)

dev0.clear_errors()

dev0.axis0.requested_state = AXIS_STATE_IDLE

dev0.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE

dev0.axis0.current_state
dev0.axis0.motor.is_calibrated
dev0.axis0.encoder.is_ready

\\\ Запуск мотора в обход чегото. дергаетя потом в одну сторону потом в другуг сторону работает
dev0.axis0.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION

dev0.save_configuration()

In [113]: dev0.axis0.config.startup_encoder_offset_calibration = False

In [114]: dev0.axis0.encoder.config.pre_calibrated = True


