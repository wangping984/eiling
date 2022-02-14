import argparse
from operator import truediv
import os
import cv2
from pathlib import Path

IMAGE_EXT = [".jpg", ".jpeg", ".bmp", ".png"]
VIDEO_EXT = [".mp4", ".avi"]

def make_parser():
    parser = argparse.ArgumentParser(description="Capture frames from video!")
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        help="input video file/files or folder path including videos. Only mp4 and avi is valid.",
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

def capture(file, args):
    cap = cv2.VideoCapture(file)
    count = 0
    fn_no_ext = Path(file).stem
    
    while cap.isOpened():
        ret, frame = cap.read()

        if ret:
            cv2.imwrite(os.path.join(args.output[0],fn_no_ext) +'_{:d}'.format(count)+'.jpg', frame)
            count += args.period[0] # i.e. at 30 fps, this advances one second
            cap.set(cv2.CAP_PROP_POS_FRAMES, count)
        else:
            cap.release()
            break
    return



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
                capture(os.path.join(x, f),args)
        elif os.path.isfile(x):
            _, ext = os.path.splitext(x)
            if ext.lower() not in VIDEO_EXT:
                print("input file is not valid video format. Only mp4 and avi is valid.")
                quit()
            else:
                capture(x,args)
            
        else:
            quit()
        