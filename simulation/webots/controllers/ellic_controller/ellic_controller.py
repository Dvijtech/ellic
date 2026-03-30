import sys
sys.path.append('/usr/lib/python3/dist-packages')  # Adjust this path based on your Webots installation
from controller import Robot

robot = Robot()
timestep = int(robot.getBasicTimeStep())

motor = robot.getDevice("crank_motor")
sensor = robot.getDevice("crank_sensor")

sensor.enable(timestep)

motor.setPosition(float('inf'))
motor.setVelocity(4.0)

while robot.step(timestep) != -1:
    print("angle:", sensor.getValue())