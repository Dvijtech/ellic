from controller import Robot

robot = Robot()
timestep = int(robot.getBasicTimeStep())

motor = robot.getDevice("crank_motor")
sensor = robot.getDevice("crank_sensor")

sensor.enable(timestep)

motor.setPosition(float('inf'))
motor.setVelocity(3.0)

slider = robot.getDevice("slider_motor")
slider.setPosition(float('inf'))
slider.setVelocity(0.2)

while robot.step(timestep) != 1:
    print("angle:", sensor.getValue())