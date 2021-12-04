import os
import argparse

import cv2
import numpy as np


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "-o", "--output",
        required=True,
        help="path to output dir containing ArUCo tag"
    )
    ap.add_argument(
        "-n", "--num",
        type=int, required=True,
        help="Number of ArUCo tags to generate"
    )
    args = ap.parse_args()

    tag_size = 300;
    arucoDict = cv2.aruco.Dictionary_get(cv2.aruco.DICT_5X5_50)
    for i in range(1, args.num + 1):
        print(f"[INFO] generating ArUCo tag with ID {i}")
        tag = np.zeros((tag_size, tag_size, 1), dtype="uint8")
        cv2.aruco.drawMarker(arucoDict, i, tag_size, tag, 1)

        file_name = os.path.join(args.output, f"tag_{i}.png")
        cv2.imwrite(file_name, tag)
        cv2.imshow("ArUCo Tag", tag)
        cv2.waitKey(10000)


if __name__ == "__main__":
    main()
