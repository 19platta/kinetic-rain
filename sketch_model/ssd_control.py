import argparse
from time import sleep

import cv2
import serial
import joblib
import numpy as np

import imutils
from imutils.video import VideoStream
from imutils.video import FPS

from sculpture import Sculpture
import calculations


FRAME_WIDTH = 300
FRAME_HEIGHT = 300
OFFSET = 0

MOTOR_STALL_SPEED = 20
MOTOR_MAX_SPEED = 200

# initialize the list of class labels MobileNet SSD was trained to
# detect, then generate a set of bounding box colors for each class
CLASSES = ["background", "aeroplane", "bicycle", "bird", "boat",
           "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
           "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
           "sofa", "train", "tvmonitor"]
PERSON_IDX = CLASSES.index("person")
COLOR = (0, 128, 128)


def parse_args():
    ap = argparse.ArgumentParser()
    ap.add_argument("-p", "--prototxt", required=True,
                    help="path to Caffe 'deploy' prototxt file")
    ap.add_argument("-m", "--model", required=True,
                    help="path to Caffe pre-trained model")
    ap.add_argument("-c", "--confidence", type=float, default=0.2,
                    help="minimum probability to filter weak detections")
    ap.add_argument("-l", "--motor-locations", type=str, default="motor_locations.jl",
                    help="motor locations to load")
    ap.add_argument("-v", "--video-source", type=str, default="/dev/video2",
                    help="motor locations to load")
    ap.add_argument("--no-draw", default=False, action="store_true",
                    help="do not draw to screen")
    args = vars(ap.parse_args())
    return args


def init_sculpture(args):
    sculpture = Sculpture(8, 10)
    motor_positions = joblib.load(args["motor_locations"])
    for i, motor in enumerate(sculpture.motors):
        motor.set_position(motor_positions[i])
    return sculpture


def init_serial(com_port="dev/ttyACM0", baud_rate=9600):
    serial_port = serial.Serial(com_port, baud_rate, timeout=1)
    return serial_port


def init_net(args):
    print("[INFO] loading model...")
    net = cv2.dnn.readNetFromCaffe(args["prototxt"], args["model"])
    return net


def init_video(args):
    # initialize the video stream, allow the cammera sensor to warmup,
    # and initialize the FPS counter
    print("[INFO] starting video stream...")
    vs = VideoStream(args["video_source"]).start()
    sleep(2.0)
    fps = FPS().start()
    return vs, fps


def detection_generator(net, vs):
    # loop over the frames from the video stream
    while True:
        # grab the frame from the threaded video stream and resize it
        # to have a maximum width of 400 pixels
        frame = vs.read()
        frame = imutils.resize(frame, width=400)
        # grab the frame dimensions and convert it to a blob
        blob = cv2.dnn.blobFromImage(
            cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT)),
            0.007843, (FRAME_WIDTH, FRAME_HEIGHT), 127.5
        )
        # pass the blob through the network and obtain the detections and predictions
        net.setInput(blob)
        detections = net.forward()
        yield frame, detections


def get_bounding_box(args, frame, detections):
    (h, w) = frame.shape[:2]
    for i in np.arange(0, detections.shape[2]):
        # extract the confidence associated with the prediction
        confidence = detections[0, 0, i, 2]
        idx = int(detections[0, 0, i, 1])
        if confidence > args["confidence"] and idx == PERSON_IDX:
            # Compute the bounding box for the object
            box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
            coords = box.astype("int")
            # Only processing the first detection
            return coords, confidence
    return None, None


def draw_box_to_frame(frame, coords, confidence):
    if coords is not None:
        startX, startY, endX, endY = coords
        label = "{:.2f}%".format(confidence * 100)
        y = startY - 15 if startY - 15 > 15 else startY + 15
        cv2.rectangle(
            frame, (startX, startY),
            (endX, endY), COLOR, 2
        )
        cv2.putText(
            frame, label, (startX, y),
            cv2.FONT_HERSHEY_SIMPLEX, 0.5, COLOR, 2
        )
    frame = cv2.resize(frame, (1000, 800))   # show the output frame
    cv2.imshow("Frame", frame)


def set_speeds(sculpture, serial_port, coords, prev_x):
    # Calculate where the person is
    startX, _, endX, _ = coords
    avg_x = (startX + endX) / 2  # 0 - 300
    norm_x = avg_x / FRAME_WIDTH  # 0 - 1
    diff_x = norm_x - prev_x  # approximate velocity

    # Update motor speeds
    for motor in sculpture.motors:
        norm_dist_from_motor = (avg_x - motor.x_position) / 300
        angle = calculations.dist_arctan(
            norm_dist_from_motor, diff_x, sculpture.rise_speed,
            flattening=0.15, sensitivity=1
        )
        motor.set_angle(angle)
        motor.set_speed(calculations.angle_to_motor_speed(
            angle,
            min_speed=MOTOR_STALL_SPEED,
            max_speed=MOTOR_MAX_SPEED
        ))

    # print(sculpture.get_motor_speeds_as_str())
    # print([motor.angle for motor in sculpture.motors])

    # speed_string = str(sculpture.get_speeds_and_angles())
    # serial_port.write(bytes(speed_string, 'utf-8'))

    return norm_x


def main():
    args = parse_args()
    sculpture = init_sculpture(args)
    net = init_net(args)
    vs, fps = init_video(args)
    serial_port = None
    # serial_port = init_serial()

    prev_x = 0
    frame = None
    try:
        for (frame, detections) in detection_generator(net, vs):
            coords, confidence = get_bounding_box(args, frame, detections)
            if not args["no_draw"]:
                draw_box_to_frame(frame, coords, confidence)
                key = cv2.waitKey(1) & 0xFF
                if key == ord("q"):
                    raise KeyboardInterrupt  # This is a little jank...
            if coords is not None:
                prev_x = set_speeds(sculpture, serial_port, coords, prev_x)
            fps.update()
    except KeyboardInterrupt:
        fps.stop()
        print("[INFO] elapsed time: {:.2f}".format(fps.elapsed()))
        print("[INFO] approx. FPS: {:.2f}".format(fps.fps()))
        cv2.destroyAllWindows()
        vs.stop()


if __name__ == "__main__":
    main()
