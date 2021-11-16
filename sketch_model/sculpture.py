from motor import Motor


class Sculpture:
    def __init__(self, num_motors, rise_speed):
        self.rise_speed = rise_speed
        self.num_motors = num_motors
        self.motors = [Motor() for _ in range(self.num_motors)]

    def get_motor_speeds(self):
        return[motor.speed for motor in self.motors]
