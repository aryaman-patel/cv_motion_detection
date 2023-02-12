#include <iostream>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <algorithm>

/// A function to compare two numerical filenames so we can sort them
bool CompareFilepaths(std::filesystem::path a, std::filesystem::path b){
    return std::stoi(a.filename().string()) < std::stoi(b.filename().string());
}

/// Reads images in `folder` and returns them in a vector
std::vector<cv::Mat> ReadImages(std::string folder){
    // Read images from the folder
    std::vector<cv::Mat> images;
    std::vector<std::filesystem::path> entries;

    // Sort the files by numerical order.
    for(const auto &entry : std::filesystem::directory_iterator(folder)){
        entries.push_back(entry.path());
        sort(entries.begin(), entries.end(), CompareFilepaths);
    }

    // Read in an order
    for(const auto &entry : entries){
        cv::Mat image = cv::imread(entry.string());
        cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
        images.push_back(image);
    }

    return images;
}

/// main
int main(int argc, char** argv)
{
    // Read in images
    std::vector<cv::Mat> images = ReadImages("Office");

    // Convert to find temporal gradient mask
    std::vector<cv::Mat> temporal_gradient_mask;
    for (int i = 0; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::absdiff(images[i], images[i+1], temp_grad);

        // Thresholding
        cv::threshold(temp_grad, temp_grad, 50, 255, cv::THRESH_BINARY);
        temporal_gradient_mask.push_back(temp_grad);
    }

    // Show the images like a video and save it
    cv::VideoWriter video("output.avi", cv::VideoWriter::fourcc('M','J','P','G'), 10, temporal_gradient_mask[0].size());
    for(int i = 0; i < images.size(); i++){
        cv::Mat masked_img;

        cv::bitwise_and(images[i], temporal_gradient_mask[i], masked_img);

        cv::imshow("Video", masked_img);
        if(cv::waitKey(25) == 'q')
            break;

        video.write(masked_img);
    }    
}
