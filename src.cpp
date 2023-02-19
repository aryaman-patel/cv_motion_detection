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

// Writes the output to a video file
void VideoFileWriter(std::vector<cv::Mat>& input, std::vector<cv::Mat>& output)
{
    
    std::string filename =  "output.avi";
    cv::VideoWriter writer(filename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 25.0, cv::Size(2*output[0].cols,output[0].rows), false);
    // check if writer object was initialized successfully.
    if (!writer.isOpened()) {
        std::cerr << "Could not open the output video file for write\n";
    }
    std::cout << "Writing videofile: " << filename << std::endl
        << "Press any key to terminate" << std::endl;
    cv::Mat frame;
    for (int i = 0; i < output.size(); i++)
    {
        // Horizontal concatenation
        cv::hconcat(input[i], output[i], output[i]);
        cv::imshow("Output", output[i]);
        writer.write(output[i]);
        // Show live and wait for a key with timeout long enough to show images
        if (cv::waitKey(30) >= 0)
            break;
    }
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


// Smoothing filters for images
std::vector<cv::Mat>& SmoothingFilters(std::vector<cv::Mat>& images, float ssigma, int filter){

    if(filter == 1)
    {
        // Apply box filter:
        for (int i = 0; i < images.size(); i++){
            cv::blur(images[i], images[i], cv::Size(3,3));
        }
    }
    else if(filter == 2)
    {
        // Apply Gaussian filter:
        for (int i = 0; i < images.size(); i++){
            cv::GaussianBlur(images[i], images[i], cv::Size(0,0), ssigma);
        }
    }

    return images;
}

/// Returns a set of masks that indicate thresholded temporal gradients using a 1x3 filter
std::vector<cv::Mat> TemporalGradientDoG(std::vector<cv::Mat> images, uint8_t thresh, float tsigma){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> mask;
    // Apply Gaussian filter:
    for (int i = 0; i < images.size(); i++){
        cv::Mat temp_grad;
        cv::GaussianBlur(images[i], temp_grad, cv::Size(0, 0), tsigma);
        images[i] = temp_grad;
    }
    // Find DoG
    for (int i = 1; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::addWeighted(images[i-1], -1, images[i+1], 1, 0, temp_grad);

        // Thresholding
        cv::threshold(temp_grad, temp_grad, thresh, 255, cv::THRESH_BINARY);
        mask.push_back(temp_grad);
    }
    return mask;
}


/// main
int main(int argc, char** argv)
{
    int dataset;
    int filtering;
    float sigma;
    bool videoWriter;
    std::vector<cv::Mat> images;
    // Take in user input for dataset and filtering
    std::cout << "Enter dataset option (1:Office/2:RedChair): ";
    std::cin >> dataset;
    std::cout << "Enter filtering option (1:Simple/2:GradientFilter/3:GradientDoG): ";
    std::cin >> filtering;
    std::cout << "Write to video file? (0/1): ";
    std::cin >> videoWriter;

    // Read in images
    std::cout << "\n Reading images..." << std::endl;
    if (dataset == 1)
    {
        images = ReadImages("Office");
    }
    else if (dataset == 2)
    {
        images = ReadImages("RedChair");
    }
    else
    {
        std::cerr << "Invalid dataset input!!" << std::endl;
        return -1;
    }
    std::cout << "\n Done reading images!" << std::endl;
    
    std::vector<cv::Mat> output;

    // Call the function based on the filtering
    if (filtering == 1)
    {
        output = TemporalGradientSimple(images, 10);
    }
    else if (filtering == 2)
    {
        output = TemporalGradientFilter(images, 10);
    }
    else if (filtering == 3)
    {
        std::cout << "Enter sigma value: ";
        std::cin >> sigma;
        output = TemporalGradientDoG(images, 10, sigma);
    }
    else
    {
        std::cerr << "Invalid filtering input!!" << std::endl;
        return -1;
    }

    // if videoWriter is true, write the output to a video file
    if (videoWriter)
    {
        VideoFileWriter(images, output);
    }
    else
    {
        // Only Show the output
        for (int i = 0; i < output.size(); i++)
        {
            cv::hconcat(images[i], output[i], output[i]);
            cv::imshow("Output", output[i]);
            // Show live and wait for a key with timeout long enough to show images
            if (cv::waitKey(30) >= 0)
                break;
        }
    }
    return 0;   
}
