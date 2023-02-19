#!/usr/bin/env python3

import os
import argparse


# Function to rename multiple files to numerical ordering
def main(args):
    files = sorted(os.listdir(args.folder))
    for count, filename in enumerate(files):
        src = f"{args.folder}/{filename}"  # foldername/filename, if .py file is outside folder
        dst = f"{args.folder}/{str(count)}.jpg"
        print(filename)
        os.rename(src, dst)


# Driver Code
if __name__ == '__main__':

    args = argparse.ArgumentParser()
    args.add_argument("-f", "--folder",
                      type=str,
                      default="/home/aryaman/Documents/CV_Project/Office/")
    args = args.parse_args()

    main(args)
