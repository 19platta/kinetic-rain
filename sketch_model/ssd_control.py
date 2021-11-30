# import the necessary packages
from imutils.video import VideoStream
from imutils.video import FPS
from motor import Motor
from sculpture import Sculpture
import numpy as np
import argparse
import imutils
import time
import cv2
import calculations

import serial
from time import sleep

sculpture = Sculpture(8, 18)

FRAME_WIDTH = 300
FRAME_HEIGHT = 300
OFFSET = 0

MOTOR_STALL_SPEED = 20
MOTOR_MAX_SPEED = 200

motor_array = sculpture.motors
motor_speeds = []

prev_x = 0

arduinoComPort = "/dev/ttyACM0"
baudRate = 9600
#serialPort = serial.Serial(arduinoComPort, baudRate, timeout=1)

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-p", "--prototxt", required=True,
                help="path to Caffe 'deploy' prototxt file")
ap.add_argument("-m", "--model", required=True,
                help="path to Caffe pre-trained model")
ap.add_argument("-c", "--confidence", type=float, default=0.2,
                help="minimum probability to filter weak detections")
args = vars(ap.parse_args())# construct the argument parse and parse the arguments

# initialize the list of class labels MobileNet SSD was trained to
# detect, then generate a set of bounding box colors for each class
CLASSES = ["background", "aeroplane", "bicycle", "bird", "boat",
           "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
           "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
           "sofa", "train", "tvmonitor"]
COLORS = np.random.uniform(0, 255, size=(len(CLASSES), 3))

# load our serialized model from disk
print("[INFO] loading model...")
net = cv2.dnn.readNetFromCaffe(args["prototxt"], args["model"])
# initialize the video stream, allow the cammera sensor to warmup,
# and initialize the FPS counter
print("[INFO] starting video stream...")
vs = VideoStream(src=0).start()
time.sleep(2.0)
fps = FPS().start()

startX = 0
endX = 0

# loop over the frames from the video stream
while True:
    # grab the frame from the threaded video stream and resize it
    # to have a maximum width of 400 pixels
    frame = vs.read()
    frame = imutils.resize(frame, width=400)
    # grab the frame dimensions and convert it to a blob
    (h, w) = frame.shape[:2]
    blob = cv2.dnn.blobFromImage(cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT)),
                                 0.007843, (FRAME_WIDTH, FRAME_HEIGHT), 127.5)
    # pass the blob through the network and obtain the detections and predictions
    net.setInput(blob)
    detections = net.forward()

    # loop over the detections
    for i in np.arange(0, detections.shape[2]):
        # extract the confidence (i.e., probability) associated with
        # the prediction
        confidence = detections[0, 0, i, 2]
        # filter out weak detections by ensuring the `confidence` is
        # greater than the minimum confidence
        if confidence > args["confidence"]:
            # extract the index of the class label from the
            # `detections`, then compute the (x, y)-coordinates of
            # the bounding box for the object
            idx = int(detections[0, 0, i, 1])
            box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
            (startX, startY, endX, endY) = box.astype("int")
            # draw the prediction on the frame
            label = "{}: {:.2f}%".format(CLASSES[idx],
                                         confidence * 100)
            cv2.rectangle(frame, (startX, startY), (endX, endY),
                          COLORS[idx], 2)
            y = startY - 15 if startY - 15 > 15 else startY + 15
            cv2.putText(frame, label, (startX, y),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, COLORS[idx], 2)

    frame = cv2.resize(frame, (1280, 960))   # show the output frame
    cv2.imshow("Frame", frame)
    key = cv2.waitKey(1) & 0xFF
    # if the `q` key was pressed, break from the loop
    if key == ord("q"):
        break
    # update the FPS counter
    fps.update()

    # calculate where the person is
    avg_x = (startX + endX) / 2  # 0 - 300
    norm_x = avg_x / FRAME_WIDTH  # 0 - 1

    diff_x = norm_x - prev_x  # approximate velocity

    for motor in motor_array:
        norm_dist_from_motor = (avg_x - motor.x_position) / 300
        angle = calculations.dist_arctan(norm_dist_from_motor, diff_x, sculpture.rise_speed)
        motor.set_angle(angle)
        motor.set_speed(calculations.angle_to_motor_speed(
            angle,
            min_speed=MOTOR_STALL_SPEED,
            max_speed=MOTOR_MAX_SPEED
            )
        )

    speed_string = ",".join(sculpture.get_speeds_and_angles_as_str())
    # serialPort.write(bytes(speed_string, 'utf-8'))
    # serialPort.write(bytes(str(100), 'utf-8'))

    prev_x = norm_x

# stop the timer and display FPS information
fps.stop()
print("[INFO] elapsed time: {:.2f}".format(fps.elapsed()))
print("[INFO] approx. FPS: {:.2f}".format(fps.fps()))
# do a bit of cleanup
cv2.destroyAllWindows()
vs.stop()
