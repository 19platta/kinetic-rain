class Motor:
    def __init__(self):
        self.x_position = 0
        self.speed = 0
        self.angle = 0

    def set_position(self, pos):
        self.x_position = pos

    def set_speed(self, speed):
        self.speed = speed

    def set_angle(self, angle):
        self.angle = angle
