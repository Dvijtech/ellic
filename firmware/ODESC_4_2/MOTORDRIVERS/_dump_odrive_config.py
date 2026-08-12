import odrive
import time

print("Connecting...")

dev0 = odrive.find_any()

print("Connected:", dev0.serial_number)

print("\n===== MOTOR =====")
print("motor_type =", dev0.axis0.motor.config.motor_type)
print("pole_pairs =", dev0.axis0.motor.config.pole_pairs)
print("current_lim =", dev0.axis0.motor.config.current_lim)
print("pre_calibrated =", dev0.axis0.motor.config.pre_calibrated)


print("\n===== ENCODER =====")
print("mode =", dev0.axis0.encoder.config.mode)
print("cpr =", dev0.axis0.encoder.config.cpr)
print("direction =", dev0.axis0.encoder.config.direction)
print("hall_polarity =", dev0.axis0.encoder.config.hall_polarity)
print("phase_offset =", dev0.axis0.encoder.config.phase_offset)
print("phase_offset_float =", dev0.axis0.encoder.config.phase_offset_float)
print("pre_calibrated =", dev0.axis0.encoder.config.pre_calibrated)


print("\n===== CONTROLLER =====")
print("control_mode =", dev0.axis0.controller.config.control_mode)
print("input_mode =", dev0.axis0.controller.config.input_mode)
print("vel_limit =", dev0.axis0.controller.config.vel_limit)


print("\n===== STARTUP =====")
print("startup_closed_loop_control =", dev0.axis0.config.startup_closed_loop_control)
print("startup_motor_calibration =", dev0.axis0.config.startup_motor_calibration)
print("startup_encoder_offset_calibration =", dev0.axis0.config.startup_encoder_offset_calibration)


print("\n===== DC BUS =====")
print("brake_resistance =", dev0.config.brake_resistance)
print("dc_max_negative_current =", dev0.config.dc_max_negative_current)
print("dc_max_positive_current =", dev0.config.dc_max_positive_current)


print("\n===== STATUS =====")
print("motor_calibrated =", dev0.axis0.motor.is_calibrated)
print("encoder_ready =", dev0.axis0.encoder.is_ready)
