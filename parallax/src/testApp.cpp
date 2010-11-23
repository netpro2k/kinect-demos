#include "testApp.h"
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
	colorBg.allocate(kinect.width, kinect.height);
	
	grayImage.allocate(kinect.width, kinect.height);
	grayBg.allocate(kinect.width, kinect.height);
	grayDiff.allocate(kinect.width, kinect.height);
	
	maskedImg.allocate(kinect.width, kinect.height,GL_RGBA);
	
	// Don't capture the background at startup
	bLearnBakground = false;
	
	// set up sensable defaults for threshold and calibration offsets
	// Note: these are empirically set based on my kinect, they will likely need adjusting
	threshold = 80;
	
	xOff = 13.486656;
	yOff = 34.486656;	
	setCalibrationOffset(xOff, yOff);
	
	// Set depth map so near values are higher (white)
	kinect.enableDepthNearValueWhite(true);
	
	// Set which direction virtual cameara is animating
	eyeDir = 1;
	
	// Setup window
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
	
	// If the user pressed spacebar, capture the depth and RGB images and save for later
    if (bLearnBakground == true){
        grayBg = grayImage;
		colorBg = colorImg;
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
	
	
	// The next block uses the finalized depth map we calculated
	// above to mask the current RGB frame
	unsigned char * pixels = new unsigned char[kinect.width*kinect.height*4];
	unsigned char * colorPixels = colorImg.getPixels();
	unsigned char * alphaPixels = grayDiff.getPixels();
	
	for (int i = 0; i < kinect.width; i++){
		for (int j = 0; j < kinect.height; j++){
			int pos = (j * kinect.width + i);
			pixels[pos*4  ] = colorPixels[pos * 3];
			pixels[pos*4+1] = colorPixels[pos * 3+1];
			pixels[pos*4+2] = colorPixels[pos * 3+2];
			pixels[pos*4+3] = alphaPixels[pos];
		}
	}
	maskedImg.loadData(pixels, kinect.width,kinect.height,GL_RGBA);
	delete [] pixels;
	
	// Move the "eye" back and forth automatically, comment
	// this out if you want to contorl with the mouse
	eyeX += 20*eyeDir;
	if(eyeX > 300)
		eyeDir = -1;
	else if(eyeX < -300)
		eyeDir = 1;
}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetHexColor(0xffffff);

	// Draw some debug images along the top
	kinect.drawDepth(10, 10, 300, 225);
	grayImage.draw(320, 10, 300, 225);
	grayDiff.draw(640, 10, 300, 225);
	kinect.draw(960, 10, 300, 225);
	
	// Save matrix state so ofTranslate's dont mess anything up
	ofPushMatrix();
	{
		// Set the virtual camera position
		gluLookAt(eyeX, eyeY, 330, eyeX, eyeY, 0, 0, 1, 0);
		
		// Draw the captured background
		colorBg.draw(ofGetWidth()/2-colorBg.width/2, 350);

		// Draw the foreground (arbitrarily) 300 units forward
		ofTranslate(0, 0, 300);
		maskedImg.draw(ofGetWidth()/2-colorBg.width/2, 350);
	}
	ofPopMatrix();
	
	// Output some help text
	char reportStr[1024];
	sprintf(reportStr, "press ' ' to capture bg\nthreshold %i (press: +/-)\nfps: %f\nArrows to calibrate\nxOffset: %f  yOffset: %f", threshold, ofGetFrameRate(), xOff, yOff);
	ofDrawBitmapString(reportStr, 20, 650);
	
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
	// move the virtual camera around with the mouse
	eyeX = x-(ofGetWidth()/2);
	eyeY = y-(ofGetHeight()/2);
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

