import argparse
import os

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

if __name__ == "__main__":
    args = make_parser().parse_args()