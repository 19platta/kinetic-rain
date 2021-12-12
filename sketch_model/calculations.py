import math

rise_speed = 10


def angle_to_motor_speed(
    angle,
    angle_domain=(-math.pi/2 * rise_speed, math.pi/2 * rise_speed),
    pos_min_speed=80, neg_min_speed=-20,
    pos_max_speed=220, neg_max_speed=-50
):
    min_speed = pos_min_speed if angle > 0 else neg_min_speed
    max_speed = pos_max_speed if angle > 0 else neg_max_speed

    domain_min = angle_domain[0]
    domain_max = angle_domain[1]

    domain_size = (domain_max - domain_min) / 2
    speed_range = abs(max_speed - min_speed)

    ratio = angle / domain_size
    # print(f"RATIO: {ratio}")
    # print(f"MIN SPEED: {min_speed}")
    # print(f"MAX SPEED: {max_speed}")
    # use ratio to scale between 0 and 255
    return int(min_speed + ratio * speed_range)


def dist_arctan(avg_x, vel, rise_speed, forward_shift=0, flattening=.15, sensitivity=1):
    if vel < 0:
        forward_shift *= -1
    distance = -abs(avg_x + forward_shift)
    distance = distance / flattening + sensitivity
    distance = math.atan(distance)
    distance += (math.pi / 8)
    distance = distance / (3 * math.pi / 8) * math.pi / 2  # -pi / 2 - pi / 2
    return distance * rise_speed
