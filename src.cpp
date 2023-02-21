#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <algorithm>
#include <numeric>

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
        if (cv::waitKey(25) >= 0)
            break;
    }
}


/// Returns a set of masks that indicate thresholded temporal gradients
std::vector<cv::Mat> TemporalGradientSimple(std::vector<cv::Mat> images){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> temporal_gradient_mask;
    for (int i = 0; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::absdiff(images[i], images[i+1], temp_grad);
        temporal_gradient_mask.push_back(temp_grad);
    }
    return temporal_gradient_mask;
}

/// Returns a set of masks that indicate thresholded temporal gradients using a 1x3 filter
std::vector<cv::Mat> TemporalGradientFilter(std::vector<cv::Mat> images){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> mask;
    for (int i = 1; i < images.size() - 1; i++){
        cv::Mat temp_grad;
        cv::addWeighted(images[i-1], -0.5, images[i+1], 0.5, 0, temp_grad);
        mask.push_back(temp_grad);
    }

    return mask;
}


std::vector<cv::Mat> TemporalGradientDoG(std::vector<cv::Mat> images, float tsigma){
    // Convert to find temporal gradient mask
    std::vector<cv::Mat> mask;
    // Define the filter parameters
    int fsize = 7;
    cv::Mat gaussFilter = cv::getGaussianKernel(fsize, tsigma, CV_64F);
    // Normalize the filter
    gaussFilter /= sum(gaussFilter)[0];

    // Set the number of weights
    int num_weights = fsize;
    

    // Loop through the images
    for (int i = 3; i < images.size() - 4; i++) {
        cv::Mat temp_grad = cv::Mat::zeros(images[i].rows, images[i].cols, CV_8U);

        // Apply the filter to the current image
        for (int j = 0; j < num_weights; j++) {
            cv::Mat temp;            
            cv::addWeighted(images[i+j-3], gaussFilter.at<float>(0, j), temp_grad, 1, 0, temp);
            temp_grad = temp.clone();
        }

        // Store the filtered image in the output vector
        mask.push_back(temp_grad);
    }

    // derivative
    return TemporalGradientFilter(mask);
}

void Threshold(std::vector<cv::Mat> images, uint8_t thresh){
    for(auto& v : images){
        // Thresholding
        cv::threshold(v, v, thresh, 255, cv::THRESH_BINARY);
    }
}

// Smoothing filters for images based on the selected filter.
void SmoothingFilters(std::vector<cv::Mat>& images, float ssigma, int filter)
{

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
}

void evaluateImages(std::vector<cv::Mat>& images, std::vector<double>&  means, std::vector<double>& stdevs)
{
    std::cout << "Evaluating noise in images..." << std::endl;
    cv::Scalar mean, std;
    for(const auto image : images)
    {
        cv::meanStdDev(image, mean, std);
        means.push_back(mean[0]);
        stdevs.push_back(std[0]);
    }
}

/// main
int main(int argc, char** argv)
{
    int dataset;
    int filtering;
    float tsigma;
    float ssigma;
    bool videoWriter;
    bool imageSeq;
    bool applySmoothing;
    int smoothing;

    std::vector<cv::Mat> images;
    // Take in user input for dataset and filtering
    std::cout << "Enter dataset option (1:Office/2:RedChair): ";
    std::cin >> dataset;
    std::cout << "Enter filtering option (1:Simple/2:GradientFilter/3:GradientDoG): ";
    std::cin >> filtering;
    std::cout << "Write to video file? (0/1): ";
    std::cin >> videoWriter;
    if(!videoWriter){
        std::cout << "Write to image sequence? (0/1): ";
        std::cin >> imageSeq;
    }
    std::cout << "Apply smoothing? (0/1): ";
    std::cin >> applySmoothing;
    if (applySmoothing)
    {
        std::cout << "Enter smoothing option (1:Box/2:Gaussian): ";
        std::cin >> smoothing;
        if (smoothing == 2)
        {
            std::cout << "Enter ssigma value: ";
            std::cin >> ssigma;
        }
    }

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
    std::cout << "\n Done reading images! \n" << std::endl;

    // Apply smoothing
    if (applySmoothing)
    {
        SmoothingFilters(images, ssigma, smoothing);
    }
    
    std::vector<cv::Mat> output;

    // Call the function based on the filtering
    if (filtering == 1)
    {

        output = TemporalGradientSimple(images);
    }
    else if (filtering == 2)
    {
        output = TemporalGradientFilter(images);
    }
    else if (filtering == 3)
    {
        std::cout << "Enter tsigma value: ";
        std::cin >> tsigma;
        output = TemporalGradientDoG(images, tsigma);
    }
    else
    {
        std::cerr << "Invalid filtering input!!" << std::endl;
        return -1;
    }


    // if we evaluate the images record noise
    std::vector<double> means;
    std::vector<double> stdevs;
    evaluateImages(output, means, stdevs);
    std::ofstream datafile("noise.txt");
    for(size_t i = 0; i < means.size(); i++)
    {
        datafile << i << " " << means[i] << " " << stdevs[i] << std::endl;
    }

    // threshold images
    double average_std  = std::accumulate(stdevs.begin(), stdevs.end(), 0) / stdevs.size();
    double average_mean = std::accumulate(means.begin(), means.end(), 0) / means.size();
    printf("std: %.2f, mean: %.2f\n", average_std, average_mean);
    Threshold(output, 5*average_std + average_mean);

    // create folder for images if desired
    if(imageSeq){
        std::filesystem::remove_all("output");
        std::filesystem::create_directory("output");
    }

    // if videoWriter is true, write the output to a video file
    if (videoWriter)
    {
        VideoFileWriter(images, output);
    }
    else
    {
        // Only Show the output
        char buff[32];
        for (int i = 0; i < output.size(); i++)
        {
            cv::hconcat(images[i], output[i], output[i]);
            cv::imshow("Output", output[i]);
            // Show live and wait for a key with timeout long enough to show images
            if (cv::waitKey(30) >= 0)
                break;

            // if imageSeq is true, write the output to an image sequence
            if(imageSeq){
                sprintf(buff, "output/%d.jpg", i);
                cv::imwrite(buff, output[i]);
            }
        }
    }
    
    // Plotting the data using gnuplot. **NOTE: Gotta install gnuplot first using sudo apt-get install gnuplot**
    FILE* gp = popen("gnuplot -persistent", "w");
    fprintf(gp, "plot 'noise.txt' using 1:3 with lines title 'Stddev', 'noise.txt' using 1:2 with lines title 'mean'\n");
    fflush(gp);
    pclose(gp);
    return 0;   
}
