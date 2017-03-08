/*
* Tracking
* started by Suraj Nair
* nair@in.tum.de
*/

#include <opencv2/opencv.hpp>
#include <application/Application.h>


int main() {
    cv::Mat img;
    cv::namedWindow("img", 1);
    int k = 0;

    application::Application app;

    while(true)
    {
        app.run();

        app.getTrackerDebugImage(img);

        cv::imshow("img", img);
        k = cv::waitKey(10);

        if((int)(k & 0xFF) == 27)
            break;
    }

    cv::destroyWindow("img");

    return 1;
}


