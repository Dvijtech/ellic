import odrive

print("Connecting...")
dev0 = odrive.find_any()

print("Connected")

#
# MOTOR
#

dev0.axis0.motor.config.motor_type = 0
dev0.axis0.motor.config.pole_pairs = 7
dev0.axis0.motor.config.current_lim = 5

#
# ENCODER
#

dev0.axis0.encoder.config.mode = 1
dev0.axis0.encoder.config.cpr = 42
# dev0.axis0.encoder.config.direction = 1
dev0.axis0.encoder.config.hall_polarity = 0

#
# CONTROLLER
#

dev0.axis0.controller.config.control_mode = 1
dev0.axis0.controller.config.input_mode = 1
dev0.axis0.controller.config.vel_limit = 20

#
# STARTUP
#

dev0.axis0.config.startup_closed_loop_control = True
dev0.axis0.config.startup_motor_calibration = False
dev0.axis0.config.startup_encoder_offset_calibration = False

#
# POWER
#

dev0.config.brake_resistance = 2

dev0.config.dc_max_negative_current = -5

print("Saving...")

dev0.save_configuration()