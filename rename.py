import os
 
# Function to rename multiple files
def main():   
    folder = "/home/aryaman/Documents/CV_Project/Office/"
    files = sorted(os.listdir(folder))
    for count, filename in enumerate(files):
        dst = f"{str(count)}.jpg"
        src =f"{folder}/{filename}"  # foldername/filename, if .py file is outside folder
        dst =f"{folder}/{dst}"
        print(filename)
        # rename() function will
        # rename all the files
        os.rename(src, dst)
 
# Driver Code
if __name__ == '__main__':
     
    # Calling main() function
    main()