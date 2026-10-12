# ODESC 4.2 — ввод в работу нового ODrive (ELLIC)
# далее инструкция по настройке лимитов тока и интегральных коэффициентов

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



# далее инструкция по настройке лимитов тока и интегральных коэффициентов

# Инструкция: настройка ODrive по шагам

Каждый этап заканчивается проверкой. Если проверка не пройдена, дальше не идём и присылаешь вывод. Изменения живут в RAM, пока нет `save_configuration()`.

## Этап 0. Условия
- По USB подключён **один** ODrive. ESP32 и CAN отключены.
- Питание **второй** платы физически снято (левая держит позицию после включения).
- Колесо поднято. Силовое питание под рукой.
- Блоки вставляй целиком. После вставки нажми Enter ещё раз, первая строка-комментарий может потеряться.

## Этап 1. Бэкап и снимок

В обычном терминале:
```
odrivetool backup-config backups/ellic_before_<дата>.json
```

В `odrivetool`:
```python
# снимок состояния платы
a = dev0.axis0; c = a.controller.config
print("node_id", a.config.can.node_id, "(1=RIGHT, 2=LEFT) | serial", hex(dev0.serial_number), "| fw", dev0.fw_version_major, dev0.fw_version_minor, dev0.fw_version_revision)
print("state", a.current_state, "| errors", a.error, a.motor.error, a.encoder.error, a.controller.error, "| startup_closed_loop", a.config.startup_closed_loop_control)
print("calibrated motor/enc", a.motor.config.pre_calibrated, a.encoder.config.pre_calibrated, "| bandwidth", a.encoder.config.bandwidth)
print("input_mode", c.input_mode, "| control_mode", c.control_mode, "| gain_sched", c.enable_gain_scheduling)
print("vel_gain", c.vel_gain, "| vel_int_gain", c.vel_integrator_gain, "| vel_int_limit", c.vel_integrator_limit, "| pos_gain", c.pos_gain)
print("vel_limit", c.vel_limit, "| current_lim", a.motor.config.current_lim, "| torque_lim", a.motor.config.torque_lim)
print("dc_max_positive", dev0.config.dc_max_positive_current, "| dc_max_negative", dev0.config.dc_max_negative_current, "| Vbus", dev0.vbus_voltage)

```
**Проверка:** fw 0.5.6, `pre_calibrated` у мотора и энкодера True, Vbus около 40. `node_id` показывает, какая это плата (1 = правая, 2 = левая). Если `pre_calibrated` False, сначала калибровка по шагу 4 из readme.

## Этап 2. Значения и проверка в воздухе

```python
# накатываем рабочие значения (в RAM)
a = dev0.axis0; c = a.controller.config
a.encoder.config.bandwidth = 100
c.input_mode = 1
c.control_mode = 3
c.enable_gain_scheduling = False
c.enable_torque_mode_vel_limit = True
c.vel_limit = 2
c.pos_gain = 12
c.vel_gain = 0.08
c.vel_integrator_gain = 0.08
c.vel_integrator_limit = 0.25
a.motor.config.current_lim = 10
dev0.config.dc_max_positive_current = 15
print("OK:", a.encoder.config.bandwidth, c.input_mode, c.pos_gain, c.vel_gain, c.vel_integrator_gain, c.vel_integrator_limit, a.motor.config.current_lim, dev0.config.dc_max_positive_current)

```
**Ожидаемо:** `OK: 100.0 1 12.0 0.08 0.08 0.25 10.0 15.0`.

```python
# тест в воздухе: шаг 0.2 оборота за 300 мс в обе стороны
import time
a = dev0.axis0
if a.current_state != 8:
    dev0.clear_errors()
    a.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(0.5)
for STEP in [0.2, -0.2]:
    a.controller.input_pos = a.encoder.pos_estimate
    time.sleep(0.5)
    p0 = a.encoder.pos_estimate; iq = []
    for i in range(15):
        a.controller.input_pos = a.encoder.pos_estimate + STEP
        t1 = time.time()
        while time.time() - t1 < 0.3:
            iq.append(a.motor.current_control.Iq_measured)
            time.sleep(0.03)
    done = a.encoder.pos_estimate - p0
    flips = sum(1 for x, y in zip(iq, iq[1:]) if x*y < 0)
    print("STEP", STEP, "| прошло", round(done, 3), "из", round(STEP*15, 3), "| Iq", round(min(iq), 2), round(max(iq), 2), "| смен знака", flips, "| state", a.current_state, "| ошибки", a.error, a.controller.error)
    a.controller.input_pos = a.encoder.pos_estimate

```
**Проверка:** «прошло» 85–110% от цели, смен знака ≤2, `state` 8, ошибок 0 0, треска нет. При треске поставь `dev0.axis0.encoder.config.bandwidth = 50` и повтори.

## Этап 3. Сохранение

```python
# сохранение: сначала IDLE (плата перезагрузится, USB пропадёт)
dev0.axis0.requested_state = AXIS_STATE_IDLE
dev0.save_configuration()

```
Переподключись и повтори снимок из этапа 1. Значения должны совпасть с этапом 2. Затем в терминале:
```
odrivetool backup-config backups/ellic_node<N>_<serial>_v1.json
```

## Этап 4. Подъём тока (только на земле, с человеком)

**Условия:** второй ODrive обесточен. Рука на питании. Ноги от колёс. Один прогон ≤8 с, между прогонами 3 минуты. После прогона ладонь на корпус мотора, термистор мультиметром, если есть. `T_fet` ≤70. Порядок `LIM`: 10 → 12 → 15 → 17 → 20, выше не идём.

```python
# подъём момента ступенью LIM (меняй LIM: 10, 12, 15, 17, 20)
import time
LIM = 10
KT = 0.04
a = dev0.axis0
c = a.controller
old_lim = a.motor.config.current_lim
p0 = a.encoder.pos_estimate
moved = False
try:
    a.motor.config.current_lim = LIM
    c.config.enable_torque_mode_vel_limit = False
    c.config.control_mode = 1
    c.input_torque = 0
    a.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(0.3)
    t = 0.0
    dt = LIM * KT / 32
    for i in range(32):
        t += dt
        c.input_torque = t
        time.sleep(0.25)
        d = a.encoder.pos_estimate - p0
        Tf = a.motor.fet_thermistor.temperature
        print("torque", round(t, 3), "Iq", round(a.motor.current_control.Iq_measured, 2), "dpos", round(d, 3), "T_fet", round(Tf, 1), "state", a.current_state)
        if a.current_state != 8:
            print("СТОП: ось не в состоянии 8")
            break
        if abs(d) > 0.5:
            moved = True
            break
        if a.error != 0 or c.error != 0 or Tf > 70:
            print("СТОП: ошибка или перегрев")
            break
finally:
    tt = abs(c.input_torque)
    while tt > 0.01:
        tt -= 0.05
        c.input_torque = max(tt, 0)
        time.sleep(0.05)
    c.input_torque = 0
    c.config.enable_torque_mode_vel_limit = True
    c.config.control_mode = 3
    c.input_pos = a.encoder.pos_estimate
    a.motor.config.current_lim = old_lim
print("ТРОНУЛОСЬ" if moved else "НЕ ТРОНУЛОСЬ", "| LIM", LIM, "| state", a.current_state, "err", a.error, c.error)

```
**Как читать:**
- `Iq` на последней строке должен быть примерно равен `LIM` (±10%). Если он ниже, тест недействителен и ступень не засчитывается.
- `ТРОНУЛОСЬ`: запиши `Iq` в момент страгивания и иди на этап 5.
- `НЕ ТРОНУЛОСЬ` на 20 А: ток выше не поднимаем. Проверяем по очереди: питание второй платы, мёртвая точка (обратная сторона и другой угол вала), знак момента, шины, Kt (`8,27/KV`), динамометр на ободе.
- Любая ошибка, перегрев или горячий мотор: остановка, остывание, присылаешь вывод.

## Этап 5. Рабочие значения

Подставь `LIM_WORK` = ток страгивания + 2–3 А, не выше 20:
```python
# рабочий ток и потолок интегратора под него
LIM_WORK = 15
a = dev0.axis0
a.motor.config.current_lim = LIM_WORK
a.controller.config.vel_integrator_limit = round(0.04 * LIM_WORK - 0.16 + 0.01, 3)
print(a.motor.config.current_lim, a.controller.config.vel_integrator_limit)

```
```python
# проверка на земле, как будет делать ESP32 (≤3 с)
import time
a = dev0.axis0
if a.current_state != 8:
    dev0.clear_errors()
    a.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
    time.sleep(0.5)
p0 = a.encoder.pos_estimate
for i in range(10):
    a.controller.input_pos = a.encoder.pos_estimate + 0.7
    time.sleep(0.3)
    print("dpos", round(a.encoder.pos_estimate - p0, 3), "Iq", round(a.motor.current_control.Iq_measured, 2), "T_fet", round(a.motor.fet_thermistor.temperature, 1), "state", a.current_state)
a.controller.input_pos = a.encoder.pos_estimate
print("state", a.current_state, "err", a.error, a.controller.error)

```
**Проверка:** колесо трогается за ≤1 с и идёт. Из-за `vel_limit 2` ожидай около 0,55–0,6 оборота за цикл, итого `dpos` около 6 из 7. Колесо стоит с растущим током означает, что не годится.

Если трогается медленно, повышай по одному параметру и после каждого повторяй тест в воздухе (этап 2): `vel_integrator_gain` 0,08 → 0,15 → 0,3, затем `vel_gain` 0,08 → 0,12 → 0,15 (только без треска).

## Этап 6. Фиксация

Повтори блок сохранения из этапа 3, переподключись, сними снимок (этап 1) и сделай `backup-config ..._v2.json`. В `Config.h` оставь `MOTOR_GEAR_RATIO = 6.33f`, `TURN_STEP = 0.7f`.

## Этап 7. ESP32: позиция и цель в телеметрии

Правки из моего прошлого сообщения (Config.h, ODriveCAN.h, ODriveCAN.cpp, Telemetry.cpp). Применяем после этапа 6, затем проверка поворота от ESP32.

## Этап 8. Вторая плата

Те же этапы 0–6, значения не копировать вслепую.

Начинай с этапов 0 и 1. Пришли вывод снимка (последний print — Vbus).