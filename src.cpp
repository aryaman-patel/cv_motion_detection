#include <iostream>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <algorithm>
// g++ -std=c++17 src.cpp -o src $(pkg-config --cflags --libs opencv4)


bool sorted_func(std::filesystem::path a, std::filesystem::path b)
{
    // Sort the files by numerical order
    return std::stoi(a.filename().string()) < std::stoi(b.filename().string());
}


int main(int argc, char** argv)
{
    // Read images form the folder
    std::vector<cv::Mat> images;
    std::string folderpath = "Office";
    std::vector<std::filesystem::path> entries;
    // Sort the files by numerical order.
    for(const auto &entry : std::filesystem::directory_iterator(folderpath))
    {
        entries.push_back(entry.path());
        sort(entries.begin(), entries.end(), sorted_func);
    }

    // Read in an order :
    for(const auto &entry : entries)
    {
        // Print the file name
        cv::Mat image = cv::imread(entry.string());
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        images.push_back(image);
    }
    // Convert to find temporal gradient
    std::vector<cv::Mat> temporal_gradient;

    for (int i = 0; i < images.size() - 1; i++)
    {
        cv::Mat temp_grad;
        cv::absdiff(images[i], images[i+1], temp_grad);
        // Thresholding
        cv::threshold(temp_grad, temp_grad, 50, 255, cv::THRESH_BINARY);
        temporal_gradient.push_back(temp_grad);
    }

    // Show the images like a video
    int i = 0;
    while (i < temporal_gradient.size())
    {
        cv::imshow("Video", temporal_gradient[i]);
        cv::waitKey(25);
        i++;
    }
    
    

}
