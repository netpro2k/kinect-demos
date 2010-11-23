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
	
	// Don't capture the background at startup
	bLearnBakground = false;
	
	// set up sensable defaults for threshold and calibration offsets
	// Note: these are empirically set based on my kinect, they will likely need adjusting
	threshold = 104;
	
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
	
    // cut off anything that is too far away
	grayDiff.threshold(threshold);
	
	// Find blobs (should be hands) in the filtered depthmap
    contourFinder.findContours(grayDiff, 1000, (kinect.width*kinect.height)/2, 5, false);
	
	// if at least 2 blobs were detected (presumably 2 hands), figure out
	// their locations and calculate the new size and rotation of the teapot
	if (contourFinder.blobs.size() >= 2) {
		// Find the x,y, and z of the center of the first 2 blobs
		float x1 = contourFinder.blobs[0].centroid.x;
		float y1 = contourFinder.blobs[0].centroid.y;
		float x2 = contourFinder.blobs[1].centroid.x;
		float y2 = contourFinder.blobs[1].centroid.y;
		float z1 = kinect.getDistanceAt(x1,y1);
		float z2 = kinect.getDistanceAt(x2,y2);
		
		// zp# are used to rotate about the z axis
		// the x1<x2 check is to ensure that p1 is always the leftmost blob (right hand)
		ofPoint zp1(x1<x2 ? x1 : x2,x1<x2 ? y1 : y2, 0);
		ofPoint zp2(x2<x1 ? x1 : x2,x2<x1 ? y1 : y2, 0);
		ofxVec3f zVec = zp1-zp2; // Vector connecting both hands excluding z direction
		
		// yp# about the y
		ofPoint yp1(x1<x2 ? x1 : x2, 0, x1<x2 ? z1 : z2);
		ofPoint yp2(x2<x1 ? x1 : x2, 0, x1>x2 ? z1 : z2);
		ofxVec3f yVec = yp1-yp2; // Vector connecting both hands excluding y direction
		
		// flat vector to base angle offsets on
		ofxVec3f horizonVec(zp1.x+1,0,0);
		
		// calculate the rotation angle about the z axis by finding the 
		// the anlge between the horizon and the respective hand vectors
		potZangle = (zp1.y > zp2.y ? -1 : 1 ) * horizonVec.angle(zVec);
		potYangle = (yp1.z > yp2.z ? -10 : 10 ) * horizonVec.angle(yVec);
		
		// calculate scale based on the distance of the hands from eachother
		potSize = zVec.length();
		
	}
}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetHexColor(0xffffff);
	
	// Draw some debug images along the top
	kinect.drawDepth(10, 10, 315, 236);
	grayDiff.draw(335, 10, 315, 236);
	
	// Draw a larger image of the calibrated RGB camera
	// and overlay the found blobs on top of it
	colorImg.draw(10,256);
	contourFinder.draw(10,256);
	
	// Save matrix state so ofTranslate's and ofRotate's dont mess anything up
	ofPushMatrix();
	{
		// canter the pot horizontaly, and 2/3 of the way down the screen vertically
		ofTranslate(ofGetWidth()*2/3, ofGetHeight()/2, 0);
		
		// rotate the pot based on calculated values
		ofRotateZ(potZangle);
		ofRotateY(potYangle);
		
		// Enable ligh
		glEnable(GL_DEPTH_TEST); 
		glEnable(GL_LIGHTING); 
		glEnable(GL_LIGHT0);
		
		// Draw the teapot
		glutSolidTeapot(potSize);
		
		// Disable lighting and depth testing so that it doesnt interfere 
		// with drawing the images in the next itteration of draw()
		glDisable(GL_DEPTH_TEST); 
		glDisable(GL_LIGHTING); 
	}
	ofPopMatrix();
	
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

