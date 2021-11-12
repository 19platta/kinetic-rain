import math


def angle_to_motor_speed(angle, angle_domain=(-math.pi/2, math.pi/2), min_speed=0, max_speed=255):
    domain_min = angle_domain[0]
    domain_max = angle_domain[1]

    domain_size = domain_max - domain_min
    speed_range = max_speed - min_speed

    ratio = angle / domain_size
    # use ratio to scale between 0 and 255
    return int(min_speed + ratio * max_speed)


def dist_arctan(avg_x, vel, rise_speed, forward_shift=0, flattening=150, sensitivity=0.75):
    if vel < 0:
        forward_shift *= -1
    distance = -abs(avg_x + forward_shift)
    distance = distance / flattening + sensitivity
    return math.atan(distance) * rise_speed


