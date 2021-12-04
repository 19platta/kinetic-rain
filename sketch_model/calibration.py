# import the necessary packages
from imutils.video import VideoStream
from imutils.video import FPS
import time
import cv2
import joblib

NUM_MOTORS = 8
PROCESSING_FRAME_WIDTH = 300
VIEWING_FRAME_WIDTH = 1280


# initialize the video stream, allow the cammera sensor to warmup,
# and initialize the FPS counter
print("[INFO] starting video stream...")
vs = VideoStream(src=0).start()
time.sleep(2.0)
fps = FPS().start()

motor_coords = []

event = None


def on_mouse(event,x,y,flags,params):
    # get mouse click
    if event == cv2.EVENT_LBUTTONDOWN:
        motor_coords.append((x, y))


# loop over the frames from the video stream
while len(motor_coords) < NUM_MOTORS:
    # grab the frame from the threaded video stream and resize it
    # to have a maximum width of 400 pixels
    frame = vs.read()
    cv2.namedWindow('frame')
    frame = cv2.resize(frame, (VIEWING_FRAME_WIDTH, 960))  # output frame
    cv2.setMouseCallback('frame', on_mouse)

    # visually indicate where the motors are
    for motor in motor_coords:
        cv2.circle(frame, motor, radius=5, color=(255, 0, 0), thickness=1)

    cv2.imshow("frame", frame)
    key = cv2.waitKey(1) & 0xFF

    # if the `q` key was pressed, break from the loop
    if key == ord("q"):
        break
    # update the FPS counter
    fps.update()

scaled_x_coords = [int(coords[0] / VIEWING_FRAME_WIDTH * PROCESSING_FRAME_WIDTH) for coords in motor_coords]
joblib.dump(scaled_x_coords, 'motor_locations.jl')

# stop the timer and display FPS information
fps.stop()
print("[INFO] elapsed time: {:.2f}".format(fps.elapsed()))
print("[INFO] approx. FPS: {:.2f}".format(fps.fps()))
# do a bit of cleanup
cv2.destroyAllWindows()
vs.stop()
