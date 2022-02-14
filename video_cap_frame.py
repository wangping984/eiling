import argparse
from operator import truediv
import os
import cv2

IMAGE_EXT = [".jpg", ".jpeg", ".bmp", ".png"]
VIDEO_EXT = [".mp4", ".avi"]

def make_parser():
    parser = argparse.ArgumentParser(description="Capture frames from video!")
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        help="input video file/files or folder path including videos.",
        required=False,
        default=os.getcwd(),
        nargs="+"
    )

    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="folder path to save output capture file/files.",
        required=False,
        default=os.getcwd(),
        nargs=1
    )

    parser.add_argument(
        "-p",
        "--period",
        type=int,
        help="every number of frames to be captured.",
        required=False,
        default=30,
        nargs=1
    )

    # parser.add_argument(
    #     "-r",
    #     "--resize",
    #     type=int,
    #     help="every number of frames to be captured.",
    #     required=False,
    #     default=30,
    #     nargs=1
    # )

    return parser

def args_check(args):
    if os.path.isdir(args.output[0]) is False:
        try:
            os.makedirs(args.output[0], exist_ok = True)
            print("Directory '%s' created successfully" % args.output[0])
        except OSError as error:
            print("Directory '%s' can not be created" % args.output[0])
            
            return 'invalid'

    return 'valid'




if __name__ == "__main__":
    args = make_parser().parse_args()
    if args_check(args) == 'invalid':
        quit()

    for x in args.input:
        if os.path.isdir(x):
            file = []
            for ext in VIDEO_EXT:
                file = file + [f for f in os.listdir(x) if f.lower().endswith(ext)]
            print(file)
            for f in file:
                cap = cv2.VideoCapture(os.path.join(x, f))
                count = 0
            
                while cap.isOpened():
                    ret, frame = cap.read()

                    if ret:
                        cv2.imwrite(os.path.splitext(os.path.join(args.output[0],f))[0]+'_{:d}'.format(count)+'.jpg', frame)
                        count += args.period[0] # i.e. at 30 fps, this advances one second
                        cap.set(cv2.CAP_PROP_POS_FRAMES, count)
                    else:
                        cap.release()
                        break
