/// Based on https://github.com/opencv/opencv_contrib/blob/master/modules/tracking/samples/csrt.cpp

#pragma comment(lib, "opencv_core440.lib")
#pragma comment(lib, "opencv_tracking440.lib")
#pragma comment(lib, "opencv_highgui440.lib")
#pragma comment(lib, "opencv_videoio440.lib")
#pragma comment(lib, "opencv_imgproc440.lib")
#pragma comment(lib, "opencv_face440.lib")
#pragma comment(lib, "opencv_objdetect440.lib")

#include "opencv2\opencv.hpp"
#include "opencv2\objdetect.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\tracking.hpp"
#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstring>
#include <Windows.h>

#include "FramesSharedMemory.h"
#include "TelemetrySharedMemory.h"

using namespace std;
using namespace cv;


void Track(Rect2d, char);


/// Shared memory receive buffer pointer
unsigned char *shmemBufferFrames;
int frameWidth, frameHeight, frameNumber, frameSize;

/// Shared memory for telemetry sending
unsigned char *shmemBufferTelemetry;


bool needInitTracker = true;

/// Create the tracker
Ptr<TrackerCSRT> tracker;

/// Frame object
Mat frame;

/// Target bounding box
Rect2d roi;

double last_x = 0, last_y = 0, last_w = 0, last_h = 0;
bool isfound = false;

int show_debug = 0;


std::string getCmdOption(int argc, char* argv[], const std::string& option)
{
	std::string cmd;
	for (int i = 0; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (0 == arg.find(option))
		{
			cmd = arg.substr(option.length());
			return cmd;
		}
	}
	return cmd;
}



int main(int argc, char** argv)
{
	string process_id = getCmdOption(argc, argv, "--id=");
	show_debug = atoi(getCmdOption(argc, argv, "--debug=").c_str());
	int skip_frames = atoi(getCmdOption(argc, argv, "--skip=").c_str());
	string cmd_x = getCmdOption(argc, argv, "--x=");
	string cmd_y = getCmdOption(argc, argv, "--y=");
	string cmd_w = getCmdOption(argc, argv, "--w=");
	string cmd_h = getCmdOption(argc, argv, "--h=");


	double tracking_x = atof(cmd_x.c_str());
	double tracking_y = atof(cmd_y.c_str());
	double tracking_w = atof(cmd_w.c_str());
	double tracking_h = atof(cmd_h.c_str());

	if (tracking_x == 0 && tracking_y == 0 && tracking_w == 0 && tracking_h == 0) {
		return 1;
	}

	if (shmemFramesInit(process_id)) return 1;
	shmemBufferFrames = shmemFramesGetBuffer();

	if (shmemTelemetryInit(process_id)) return 1;


	roi = cv::Rect2d(0, 0, 0, 0);

	int skipFrameCounter = 0;

	for (;;) {

		/// Receive new frame from shared memory
		if (shmemFramesReceive(&frameWidth, &frameHeight, &frameNumber, &frameSize) == SHMEM_OK) {

			//printf("frame %d %dx%d %d bytes\n", frameNumber, frameWidth, frameHeight, frameSize);

		} else {
			Sleep(5);
			continue;
		}

		if (skip_frames) {
			skipFrameCounter++;
			if (skipFrameCounter % 2 == 0) {
				continue;
			}
		}

		/// Convert byte buffer to frame
		
		/// Grayscale
		//cv::Mat frame(cv::Size(frameWidth, frameHeight), CV_8UC1, shmemBufferFrames + 11, cv::Mat::AUTO_STEP);

		/// BGR
		cv::Mat frame(cv::Size(frameWidth, frameHeight), CV_8UC3, shmemBufferFrames + 11, cv::Mat::AUTO_STEP);

		if (frame.empty() || frame.rows == 0 || frame.cols == 0)
		{
			printf("failed to read frame\n");
			break;
		}

		


		/// Init tracker if needed
		if (needInitTracker) {

			needInitTracker = false;

			roi.x = (tracking_x - tracking_w/2.0) * frameWidth;
			roi.y = (tracking_y - tracking_h/2.0) * frameHeight;
			roi.width = tracking_w * frameWidth;
			roi.height = tracking_h * frameHeight;

			/// Create tracker
			tracker = TrackerCSRT::create();

			printf("init tracker ROI %1.3f %1.3f %1.3f %1.3f\n", roi.x, roi.y, roi.width, roi.height);

			/// Init tracker for ROI
			tracker->init(frame, roi);
		}
		
			
		isfound = tracker->update(frame, roi);
		
		/// If target lost
		if (!isfound) {
		
			printf("The target has been lost...\n");
		
			roi.empty();
		
			/// Set latest tracking coordinates for reinitialization
			tracking_x = last_x;
			tracking_y = last_y;
			tracking_w = last_w;
			tracking_h = last_h;
		
			/// Reinitialize
			needInitTracker = true;
		}
		
		
		/// Do tracking
		Track(roi, isfound);

		
		/// Draw the tracked object and show the image
		if (show_debug) {
			rectangle(frame, roi, Scalar(255, 0, 0), 2, 1);
			imshow("Tracker", frame);
		}


		/// Quit on ESC button
		if (waitKey(1) == 27) break;
	}
}


void Track(Rect2d roi, char state) {

	double x_rel = (roi.x + (roi.width / 2.0)) / (double)frameWidth;
	double y_rel = (roi.y + (roi.height / 2.0)) / (double)frameHeight;
	double w_rel = roi.width / (double)frameWidth;
	double h_rel = roi.height / (double)frameHeight;

	if (x_rel < 0) x_rel = 0;
	if (x_rel > 1) x_rel = 1;
	if (y_rel < 0) y_rel = 0;
	if (y_rel > 1) y_rel = 1;
	if (w_rel < 0) w_rel = 0;
	if (w_rel > 1) w_rel = 1;
	if (h_rel < 0) h_rel = 0;
	if (h_rel > 1) h_rel = 1;

	if (show_debug) {
		printf("center %1.3f %1.3f   %1.3f %1.3f\n", x_rel, y_rel, w_rel, h_rel);
	}

	shmemTelemetrySend(state, x_rel, y_rel, w_rel, h_rel);

	if (state) {
		/// Save tracking coordinates
		last_x = x_rel;
		last_y = y_rel;
		last_w = w_rel;
		last_h = h_rel;
	}
}
