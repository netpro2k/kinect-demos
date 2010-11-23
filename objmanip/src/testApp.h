#ifndef _TEST_APP
#define _TEST_APP


#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxKinect.h"

class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

	private:
		// Instance of the kinect object
		ofxKinect kinect;
		
		// Current camera tilt angle
		int camTilt;
		
		// The calibration offsets to align depth and RGB cameras
		float xOff;
		float yOff;
		
		// Quick utility function to apply the offsets to the camera matrix
		void setCalibrationOffset(float x, float y);
		
		
		// Used for storing each RGB frame
		ofxCvColorImage	colorImg;
		
				
		// Used to store each depth frame
		ofxCvGrayscaleImage grayImage;
		// Used to store captured depth bg
		ofxCvGrayscaleImage grayBg;
		// Used to store the processed depth image
		ofxCvGrayscaleImage grayDiff;
		
		// Used to find blobs in the filtered depthmap
		ofxCvContourFinder 	contourFinder;
		
		// Flag to capture the background in the next update()
		bool bLearnBakground;
		// distance at which depth map is "cut off"
		int	threshold;
		
		// The current angl and size of the teapot
		float potZangle;
		float potYangle;
		float potSize;
	
};

#endif
