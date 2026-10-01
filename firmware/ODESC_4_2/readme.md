# ODESC 4.2 — ввод в работу нового ODrive (ELLIC)

Ожидаемо: прошивка **0.5.6**, железо **3.6**, протокол **CAN Simple**, 250000 бит/с, мотор на **axis0**, энкодер — Hall (mode 1, cpr 42).

Файлы в этой папке:

| Файл | Что это |
|---|---|
| `configODRIVE.txt` | **Шаблон** (бэкап рабочего ODrive, node_id=1). Содержит калибровку старого мотора — как есть на новый не заливать |
| `make_config.py` | Делает из шаблона безопасный конфиг для пусконаладки с нужным node_id |
| `backups/` | Итоговые бэкапы каждого откалиброванного ODrive (по одному файлу на плату) |

> В командах ниже объект называется `dev0` (как в прежних заметках). Если в твоём шелле он `odrv0` — подставь.

---

## Почему так, а не «залить как есть»

Конфиг стоит переносить (лимиты, усиления, CAN-рейты, Hall-режим одинаковы для всей партии). Но в нём есть **данные конкретного мотора**: `phase_resistance`, `phase_inductance`, `phase_offset`, `hall_polarity`, флаги `pre_calibrated`. Их надо измерить заново на каждом новом моторе, иначе это риск плохого момента и рывков.

Две ловушки, которые закрывает `make_config.py`:

1. Если флаги `pre_calibrated` остались `True`, `FULL_CALIBRATION_SEQUENCE` **пропустит** калибровку.
2. В шаблоне `startup_closed_loop_control = true` — после рестора плата сразу включит замкнутый контур на чужой калибровке, и мотор может дёрнуться. В конфиге для пусконаладки это выключено, включаем в самом конце.

---

## Перед началом (1 минута)

- Колесо/мотор **без нагрузки**: колесо приподнято, ничего не касается. Калибровка вращает мотор.
- Питание силовое (аккумулятор/блок), не только USB.
- К ПК по USB подключён **один** ODrive. CAN к ESP32 пока **не подключён** (чтобы не было двух плат с одинаковым node_id на шине).
- Версия `odrivetool` та же, что использовалась для `backup-config` на старой плате.

---

## Шаг 1. Прошивка (только для чистой платы)

Через программатор залить `firmware.elf`. 


Затем в `odrivetool`:

```python
print(f"{dev0.fw_version_major}.{dev0.fw_version_minor}.{dev0.fw_version_revision}")   # ждём 0.5.6
print(f"{dev0.hw_version_major}.{dev0.hw_version_minor}")                              # ждём 3.6
print(hex(dev0.serial_number))                                                         # запиши — пригодится для имени бэкапа
```

## Шаг 2. Сделать конфиг под нужный node_id

В терминале (не в odrivetool), в папке с `configODRIVE.txt`:

```
python make_config.py 1     # RIGHT  -> config_node1_commissioning.json
python make_config.py 2     # LEFT   -> config_node2_commissioning.json
```

RIGHT = node_id 1, LEFT = node_id 2 (как в спецификации).

## Шаг 3. Залить конфиг

```
odrivetool restore-config config_node2_commissioning.json
```

Плата перезагрузится и сохранит конфиг. Затем открыть `odrivetool` заново и проверить:
Предупреждения Could not restore ... anticogging.calib_anticogging / cogging_ratio / index безвредны: это служебные поля антикоггинга, которые odrivetool 0.5.4 не умеет записывать через exchange. Антикоггинг ты не используешь (pre_calibrated: false), так что на работу они не влияют. Итоговые строки «Some of the configuration could not be restored» и «Configuration restored» ожидаемы. Второй запуск был не нужен, но ничему не повредил.

Последняя команда restore-config config_node1_commissioning.json внутри IPython (In [2]:) не сработает. Это команда терминала, а не Python. Тебе она там уже не нужна, она выполнена. Строку v в In [1] можно проигнорировать.

Оболочка уже открыта и подключена как dev0, так что команды из readme.md подходят без замены. Дальше, шаг 3 (проверки после рестора), вставь в оболочку:

```python
print(dev0.axis0.config.can.node_id)                       # нужный node_id
print(dev0.can.config.baud_rate)                           # 250000
print(dev0.axis0.config.startup_closed_loop_control)       # False (пока)
print(dev0.axis0.controller.config.vel_limit, dev0.axis0.motor.config.current_lim)   # 2.0 10.0
```

Если `restore-config` ругается на отдельные поля — это обычно read-only параметры, на работу не влияет. Критично, чтобы совпали проверки выше.

## Шаг 4. Калибровка (мотор свободен! минимум 37 вольт и 2 ампера на блоке питания. При малых токах не калибруется!!)

```python
# 4.1 Параметры мотора
dev0.axis0.requested_state = AXIS_STATE_MOTOR_CALIBRATION
# подождать ~5 с, пока мотор пискнет/затихнет
print(dev0.axis0.error, dev0.axis0.motor.error)            # оба 0
print(dev0.axis0.motor.config.phase_resistance, dev0.axis0.motor.config.phase_inductance)
#   ориентир со старой платы: R ≈ 0.176, L ≈ 0.000204 (±~20% нормально; в разы — стоп, проверять фазы)
dev0.axis0.motor.config.pre_calibrated = True

# 4.2 Hall: полярность, затем смещение (мотор повернётся на несколько оборотов)
dev0.axis0.requested_state = AXIS_STATE_ENCODER_HALL_POLARITY_CALIBRATION
dev0.axis0.requested_state = AXIS_STATE_ENCODER_OFFSET_CALIBRATION
# подождать ~10 с
print(dev0.axis0.error, dev0.axis0.encoder.error)          # оба 0
print(dev0.axis0.encoder.config.hall_polarity_calibrated)  # True
print(dev0.axis0.encoder.is_ready)                         # True
print(dev0.axis0.encoder.config.phase_offset_float)        # со старой платы было ≈ 0.50 (ориентир, не критерий)
dev0.axis0.encoder.config.pre_calibrated = True
```

Если после любого шага ошибка не 0: `dump_errors(dev0)`, `dev0.clear_errors()`, проверить подключение фаз и Hall-кабеля, повторить шаг.


## Шаг 5. Проба замкнутого контура (мотор всё ещё свободен)

```python
dev0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
p = dev0.axis0.encoder.pos_estimate
dev0.axis0.controller.input_pos = p + 0.5      # полоборота вперёд
# …посмотреть, что вращение плавное, без шума и рывков…
dev0.axis0.controller.input_pos = p            # назад
print(dev0.axis0.error, dev0.axis0.motor.error, dev0.axis0.controller.error, dev0.axis0.encoder.error)  # все 0
dev0.axis0.requested_state = AXIS_STATE_IDLE
```

## Шаг 6. Включить автозапуск и сохранить

```python
dev0.axis0.config.startup_closed_loop_control = True
dev0.save_configuration()          # плата перезагрузится, USB-связь пропадёт — это нормально
```

После перезапуска: `print(dev0.axis0.current_state)` → `8` (замкнутый контур), ошибки 0.

## Шаг 7. Бэкап готовой платы

В терминале:

```
odrivetool backup-config backups/ellic_node2_<serial>.json
```

Один файл на каждую плату. `configODRIVE.txt` остаётся шаблоном и не перезаписывается.

## Шаг 8. Подключение к шине и проверка с ESP32

Перед включением: на CAN-шине ровно **две** терминирующие 120 Ω на концах (при выключенном питании между CAN_H и CAN_L ≈ **60 Ω**), общая земля, у каждой платы свой node_id.

Включать **по одной плате**: сначала RIGHT, проверить, потом добавить LEFT. В телеметрии ESP32 для каждого ODrive должно быть:

- `online=1`, `state=8`
- `axis_err=0 motor_err=0 enc_err=0 ctrl_err=0`
- `Vbus` соответствует питанию, `rx` растёт, `rx_fail` не растёт
- при вращении вала колёса идут в зеркально противоположные стороны (знаки `LEFT_WHEEL_SIGN` / `RIGHT_WHEEL_SIGN` из `Config.h`)

Ранее при `node_id=1` наблюдалось странное поведение (`axis_err=0x40`), которого не было при `node_id=2`. Поэтому RIGHT проверяй отдельно и первой; `make_config.py` дополнительно разводит `axis1` node_id (11 / 12), чтобы оси двух плат не пересекались.

---

## Быстрая шпаргалка (нормальный случай)

```
python make_config.py <N>
odrivetool restore-config config_node<N>_commissioning.json
# в odrivetool: проверки шага 3 → калибровка (шаг 4) → проба (шаг 5)
# dev0.axis0.config.startup_closed_loop_control = True ; dev0.save_configuration()
odrivetool backup-config backups/ellic_node<N>_<serial>.json
```

## Чек-лист перед первым запуском ESP32

- [ ] fw 0.5.6, hw 3.6
- [ ] node_id правильный, baud 250000
- [ ] калибровка пройдена, ошибки 0, `pre_calibrated = True` (мотор и энкодер)
- [ ] `startup_closed_loop_control = True`, сохранено
- [ ] бэкап сделан
- [ ] CAN: терминаторы, земля, уникальные node_id
- [ ] `Config.h` проверен (см. ниже)

> Внимание: значения в `Config.h` расходятся со спецификацией — `MOTOR_GEAR_RATIO = 6.4` (в спеке 4.4), `TURN_STEP = 1.0` (в спеке 0.03), `TELEMETRY_PRINT_PERIOD_MS = 1000` (в спеке 100). Если это не осознанные настройки под новую сборку — верни спецификационные значения перед первым запуском колёс. TURN_STEP = 1.0 это целый оборот мотора за один цикл.
