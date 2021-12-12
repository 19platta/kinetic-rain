from time import sleep
import serial

SLEEP = 1;


def init_serial(com_port="/dev/ttyACM0", baud_rate=9600):
    serial_port = serial.Serial(com_port, baud_rate, timeout=1)
    return serial_port


def _to_speed_string(speed):
    return str([(speed, 0.0) for _ in range(8)])


def main():
    serial_port = init_serial()
    speeds = [100, 90, 80, 70]
    speed_strings = [_to_speed_string(s) for s in speeds]
    stop = _to_speed_string(0)

    for speed_string in speed_strings:
        print(speed_string)
        serial_port.write(bytes(speed_string, 'utf-8'))
        sleep(SLEEP)

    input()
    serial_port.write(bytes(stop, 'utf-8'))


if __name__ == "__main__":
    main()
