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


/// Returns a set of masks that indicate thresholded temporal gradients
std::vector<cv::Mat> TemporalGradientSimple(std::vector<cv::Mat> images, uint8_t thresh){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> temporal_gradient_mask;
    for (int i = 0; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::absdiff(images[i], images[i+1], temp_grad);

        // Thresholding
        cv::threshold(temp_grad, temp_grad, thresh, 255, cv::THRESH_BINARY);
        temporal_gradient_mask.push_back(temp_grad);
    }

    return temporal_gradient_mask;
}


/// Returns a set of masks that indicate thresholded temporal gradients using a 1x3 filter
std::vector<cv::Mat> TemporalGradientFilter(std::vector<cv::Mat> images, uint8_t thresh){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> mask;
    for (int i = 1; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::addWeighted(images[i-1], -0.5, images[i+1], 0.5, 0, temp_grad);

        // Thresholding
        cv::threshold(temp_grad, temp_grad, thresh, 255, cv::THRESH_BINARY);
        mask.push_back(temp_grad);
    }

    return mask;
}


/// Returns a set of masks that indicate thresholded temporal gradients using a 1x3 filter
std::vector<cv::Mat> TemporalGradientDoG(std::vector<cv::Mat> images, uint8_t thresh, float tsigma){
    // TODO
    std::vector<cv::Mat> mask;
    return mask;
}


/// main
int main(int argc, char** argv)
{
    // Read in images
    std::vector<cv::Mat> images = ReadImages("Office");

    // Get simple temporal derivative mask
    std::vector<cv::Mat> simple_gradient_mask = TemporalGradientSimple(images, 10);

    // Get filter temporal derivative mask
    std::vector<cv::Mat> filter_gradient_mask = TemporalGradientFilter(images, 10);

    // Show the images like a video and save it
    cv::VideoWriter video("output.avi", 
                          cv::VideoWriter::fourcc('M','J','P','G'), 
                          10, filter_gradient_mask[0].size());
    for(int i = 0; i < filter_gradient_mask.size(); i++){
        cv::Mat simple_grad_img, filter_grad_img, frame;

        // Apply masks
        cv::bitwise_and(images[i], simple_gradient_mask[i], simple_grad_img);
        cv::bitwise_and(images[i+1], filter_gradient_mask[i], filter_grad_img);

        // Concatenate for viewing
        cv::hconcat(images[i], simple_grad_img, frame);
        cv::hconcat(frame, filter_grad_img, frame);

        cv::imshow("Video", frame);
        if(cv::waitKey(10) == 'q')
            break;

        video.write(frame);
    }    
}
