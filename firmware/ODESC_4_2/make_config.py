#!/usr/bin/env python3
"""
make_config.py - готовит конфиг для ПУСКОНАЛАДКИ нового ODrive из шаблона.

Использование:
    python make_config.py <node_id> [шаблон.json]

    python make_config.py 1                       # RIGHT, шаблон configODRIVE.txt
    python make_config.py 2                       # LEFT
    python make_config.py 2 mytemplate.json

Результат: config_node<N>_commissioning.json рядом со скриптом.
Дальше:  odrivetool restore-config config_node<N>_commissioning.json

Что меняется относительно шаблона (всё остальное - как в рабочем конфиге):
  axis0.config.can.node_id                      -> <node_id>
  axis1.config.can.node_id                      -> 10 + <node_id>  (чтобы оси axis1 двух плат не совпадали)
  axis0.config.startup_closed_loop_control      -> False  (после рестора мотор НЕ должен сам поехать
                                                           с чужой калибровкой; включается в конце вручную)
  axis0.motor.config.pre_calibrated             -> False  (калибровка старого мотора не наследуется)
  axis0.encoder.config.pre_calibrated           -> False
  axis0.encoder.config.hall_polarity_calibrated -> False
"""
import json
import os
import sys


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    node_id = int(sys.argv[1])
    if not (0 <= node_id <= 62):
        sys.exit("node_id должен быть 0..62")

    here = os.path.dirname(os.path.abspath(__file__))
    template = sys.argv[2] if len(sys.argv) > 2 else os.path.join(here, "configODRIVE.txt")

    # json в Python понимает Infinity, как и odrivetool backup-config
    with open(template, "r", encoding="utf-8") as f:
        cfg = json.load(f)

    a0 = cfg["axis0"]
    a0["config"]["can"]["node_id"] = node_id
    cfg["axis1"]["config"]["can"]["node_id"] = 10 + node_id

    a0["config"]["startup_closed_loop_control"] = False
    a0["motor"]["config"]["pre_calibrated"] = False
    a0["encoder"]["config"]["pre_calibrated"] = False
    a0["encoder"]["config"]["hall_polarity_calibrated"] = False

    out = os.path.join(here, f"config_node{node_id}_commissioning.json")
    with open(out, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)

    print(f"OK: {out}")
    print(f"  axis0 node_id = {node_id}, axis1 node_id = {10 + node_id}")
    print("  startup_closed_loop_control = False, pre_calibrated = False (motor/encoder)")
    print(f"Дальше: odrivetool restore-config \"{out}\"")


if __name__ == "__main__":
    main()
