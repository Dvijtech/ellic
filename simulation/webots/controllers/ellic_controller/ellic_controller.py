from controller import Robot
import math

robot = Robot()
timestep = int(robot.getBasicTimeStep())

# === устройства ===
crank = robot.getDevice("crank_motor")
slider = robot.getDevice("slider_motor")
wheel = robot.getDevice("wheel_motor")

# === настройки моторов ===
crank.setPosition(float('inf'))
crank.setVelocity(2.0)

slider.setVelocity(2.0)
slider.setPosition(0)

wheel.setPosition(float('inf'))
wheel.setVelocity(0)

# === параметры Ellic ===
R = 0.2              # радиус кривошипа
L = 0.5              # длина шатуна
wheel_radius = 0.15

Z1 = 20              # звезда на шатунe
Z2 = 40              # звезда на колесе

speed = 2.0          # скорость вращения кривошипа

time = 0

while robot.step(timestep) != -1:
    dt = timestep / 1000.0
    time += dt
    
    # === главный параметр ===
    theta = time * speed
    
    # =========================
    # 1. КОЛЕНВАЛ
    # =========================
    crank.setVelocity(speed)
    
    # =========================
    # 2. КАРЕТКА (crank-slider)
    # =========================
    under_sqrt = L**2 - (R * math.sin(theta))**2
    if under_sqrt < 0:
        under_sqrt = 0  # защита от NaN
    
    x = R * math.cos(theta) + math.sqrt(under_sqrt)
    
    slider.setPosition(x)
    
    # =========================
    # 3. КОЛЕСО
    # =========================
    # вращение от цепи
    phi_chain = theta * (Z1 / Z2)
    
    # вращение от движения
    phi_roll = x / wheel_radius
    
    phi_total = phi_chain + phi_roll
    
    wheel.setVelocity(0)
    wheel.setPosition(phi_total)
    
    # =========================
    # 4. ДВИЖЕНИЕ ВСЕГО МЕХА (MVP)
    # =========================
    forward_speed = abs(math.sin(theta)) * 2.0
    
    # robot.getSelf().setVelocity([forward_speed, 0, 0, 0, 0, 0]) 