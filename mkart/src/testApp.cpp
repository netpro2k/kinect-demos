#include "testApp.h"
#include "ofxKinect.h"
#include <OpenGL/glu.h>

//--------------------------------------------------------------
void testApp::setCalibrationOffset(float x, float y) {
	ofxMatrix4x4 matrix = kinect.getRGBDepthMatrix();
	matrix.getPtr()[3] = x;
	matrix.getPtr()[7] = y;
	kinect.setRGBDepthMatrix(matrix);
}

//--------------------------------------------------------------
void testApp::sendKeystrokeToProcess(CGKeyCode code, bool down) {
	// This is a Mac specific way of sending global keystroke
	// Note, th reason I am using this call and not a Core Foundation
	// call is that most emulators do not recieve input through Core
	// Foundation, so they do not detect keystrokes sent that way
	AXUIElementRef axSystemWideElement = AXUIElementCreateSystemWide();
	AXUIElementPostKeyboardEvent(axSystemWideElement, 0, code, down);
	CFRelease(axSystemWideElement);
}

//--------------------------------------------------------------
void testApp::setup(){
	// Setup kinect
	kinect.init();
	kinect.setVerbose(true);
	kinect.open();
	
	// Allocate space for all the images
	colorImg.allocate(kinect.width, kinect.height);
	
	grayImage.allocate(kinect.width, kinect.height);
	grayBg.allocate(kinect.width, kinect.height);
	grayDiff.allocate(kinect.width, kinect.height);
	footDiff.allocate(kinect.width, kinect.height );
	
	// Don't capture the background at startup
	bLearnBakground = false;
	
	// set up sensable defaults for threshold and calibration offsets
	// Note: these are empirically set based on my kinect, they will likely need adjusting
	threshold = 72;
	
	xOff = 13.486656;
	yOff = 34.486656;	
	setCalibrationOffset(xOff, yOff);
	
	// Set depth map so near values are higher (white)
	kinect.enableDepthNearValueWhite(true);
	
	// Setup window
	ofSetFullscreen(true);
	ofSetFrameRate(30);	
}

//--------------------------------------------------------------
void testApp::update(){
	
	ofBackground(100, 100, 100);
	kinect.update();
	
	// Pull in new frame
	kinect.update();
	grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
	colorImg.setFromPixels(kinect.getCalibratedRGBPixels(), kinect.width, kinect.height);
	
	// Quick and dirty noise filter on the depth map. Needs work
	grayImage.dilate();
	grayImage.erode();
	
	// If the user pressed spacebar, capture the depth iamge and save for later
    if (bLearnBakground == true){
        grayBg = grayImage;
        bLearnBakground = false;
    }
	
	// The next few steps mask the depthmap so that only pixels 
	// that have changed since background was captured are considered
	
    // Subtract the saved background from the current one
	grayDiff = grayImage;
	grayDiff -= grayBg;
	
	// anything that is > 1 has changed, so keep it
    grayDiff.threshold(1);
	// multiply in the current depth values, to mask it 
	grayDiff *= grayImage;
	
	// Copy the filtered depthmap so we can use it for detecting feet 
	footDiff = grayDiff;
	// for feet we want to focus on only the bottom part of the image
	// so we set the region of interest to the bottom 180 px
	footDiff.setROI(0, 300,footDiff.width, 480-300);
	
	// cut off anything that is too far away
    grayDiff.threshold(104); // TODO: This should be configurable as well
	footDiff.threshold(threshold);
	
	// since we set ROI, we need to reset it
	footDiff.resetROI();
	// also, since ROI was on when we did the above threshold
	// we clear out all pixels that are not fully white 
	//(which ends up being only the upper part of the iamge)
	footDiff.threshold(254);
	
	// Find blobs (should be hands and foot) in the filtered depthmap
	contourFinder.findContours(grayDiff, 1000, (kinect.width*kinect.height)/2, 5, false);
	footContourFinder.findContours(footDiff, 1000, (kinect.width*kinect.height)/2, 5, false);
	
	// if at least 2 blobs were detected (presumably 2 hands), figure out
	// their locations and calculate which way to "steer"
	if (contourFinder.blobs.size() >= 2) {
		// Find the x,y cord of the center of the first 2 blobs
		float x1 = contourFinder.blobs[0].centroid.x;
		float y1 = contourFinder.blobs[0].centroid.y;
		float x2 = contourFinder.blobs[1].centroid.x;
		float y2 = contourFinder.blobs[1].centroid.y;
		
		// the x1<x2 check is to ensure that p1 is always the leftmost blob (right hand)
		ofPoint p1(x1<x2 ? x1 : x2,x1<x2 ? y1 : y2, 0);
		ofPoint p2(x2<x1 ? x1 : x2,x2<x1 ? y1 : y2, 0);
		
		// if the "steering wheel" is sufficently rotated
		if(abs(p1.y-p2.y) > 50){
			if(p1.y < p2.y ){ // turning left
				if(!leftDown){ // if left is already down, dont send key even again
					// Send the key down event for left, and up event for right
					sendKeystrokeToProcess((CGKeyCode) kVK_LeftArrow ,true);
					sendKeystrokeToProcess((CGKeyCode) kVK_RightArrow ,false);
					leftDown = true;
					rightDown = false;
				}
			} else { // turning right
				if(!rightDown){ // if left is already down, dont send key even again
					// Send the key down event for right, and up event for left
					sendKeystrokeToProcess((CGKeyCode) kVK_RightArrow ,true);
					sendKeystrokeToProcess((CGKeyCode) kVK_LeftArrow ,false);
					rightDown = true;
					leftDown = false;
				}
			}
		} else { // "steering weheel" centered
			if(leftDown){
				sendKeystrokeToProcess((CGKeyCode) kVK_LeftArrow ,false);
				leftDown = false;
			}
			if(rightDown){
				sendKeystrokeToProcess((CGKeyCode) kVK_RightArrow ,false);
				rightDown = false;
			}
		}
	} else { // no hands detected
		if(leftDown){
			sendKeystrokeToProcess((CGKeyCode) kVK_LeftArrow ,false);
			leftDown = false;
		}
		if(rightDown){
			sendKeystrokeToProcess((CGKeyCode) kVK_RightArrow ,false);
			rightDown = false;
		}
	}

	// if any blob is detected in the foot map, it can be considered a foot
	if(footContourFinder.blobs.size() >= 1) {
		ofBackground(0,255,0); // set background to green for debugging
		if(!footDown) {
			sendKeystrokeToProcess((CGKeyCode) 6 ,true);
			footDown = true;
		}
	} else {
		ofBackground(100,100,100);
		if(footDown) {
			sendKeystrokeToProcess((CGKeyCode) 6 ,false);
			footDown = false;
		}
	}
}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetHexColor(0xffffff);
	
	// Draw some debug images along the top
	kinect.drawDepth(10, 10, 315, 236);
	grayDiff.draw(335, 10, 315, 236);
	footDiff.draw(660, 10, 315, 236);
	
	
	// Draw a larger image of the calibrated RGB camera
	// and overlay the found blobs on top of it
	colorImg.draw(10,256);
	contourFinder.draw(10,256);
		
	// Display some debugging info
	char reportStr[1024];
	sprintf(reportStr, "left: %i right: %i foot: %i", leftDown, rightDown, footDown);
	ofDrawBitmapString(reportStr, 20, 800);
	
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	// set keybindings for capturing the background, manipulating the offset, and tilting the kinect
	switch (key)
	{
		case ' ':
			bLearnBakground = true;
			break;
		case '+':
			threshold++;
			break;
		case '-':
			threshold--;
			break;
		case OF_KEY_UP:
			yOff++;
			setCalibrationOffset(xOff, yOff);
			break;
		case OF_KEY_DOWN:
			yOff--;
			setCalibrationOffset(xOff, yOff);
			break;
		case OF_KEY_LEFT:
			xOff--;
			setCalibrationOffset(xOff, yOff);
			break;
		case OF_KEY_RIGHT:
			xOff++;
			setCalibrationOffset(xOff, yOff);
			break;
		// Note these are currently not enabled in ofxKinect as of 11/23/2010
		case 'h':
			camTilt++;
			kinect.setCameraTiltAngle(camTilt);
			break;
		case 'n':
			camTilt--;
			kinect.setCameraTiltAngle(camTilt);
			break;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}

