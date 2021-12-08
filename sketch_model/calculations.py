import math

rise_speed = 10


def angle_to_motor_speed(angle, angle_domain=(-math.pi/2 * rise_speed, math.pi/2 * rise_speed), min_speed=0, max_speed=255):
    domain_min = angle_domain[0]
    domain_max = angle_domain[1]

    domain_size = (domain_max - domain_min) / 2
    speed_range = max_speed - min_speed

    ratio = angle / domain_size
    # use ratio to scale between 0 and 255
    return int(min_speed + ratio * speed_range)


def dist_arctan(avg_x, vel, rise_speed, forward_shift=0, flattening=.15, sensitivity=1):
    if vel < 0:
        forward_shift *= -1
    distance = -abs(avg_x + forward_shift)
    distance = distance / flattening + sensitivity
    distance = math.atan(distance)
    distance += (math.pi / 8)
    distance = distance / (3 * math.pi / 8) * math.pi / 2
    return distance * rise_speed
