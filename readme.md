This is just a series of demos I have done using libfreenect, ofxKinect and openFrameworks. These are by no means optimized or extensive demoes, and are really just a log of my tinkering/learning process that others may learn from

# How to build

These instructions assume you are on a mac. The demos may work on other systems supported by libfreenect, but I can't really speak to how to set them up.

## Get OpenFrameworks

	git clone git://github.com/openframeworks/openFrameworks.git

## Get ofxKinect

	cd openFrameworks/addons
	mkdir kinect
	cd kinect
	git clone git://github.com/ofTheo/ofxKinect.git

## test ofxKinect

	open ofxKinect/ofxKinect.xcodeproj
	# Click "Build -> Build and Run" (Command-Enter)
	
_Instructions from [freenect.com](http://www.freenect.com/kinect-now-supported-in-openframeworks-cool-d)_

If you have made it this far you are now ready to run my demos

	cd .. #to get back to base addons directory
	ln -s kinect/ofxKinect/ ofxKinect
	cd ../apps
	git clone git://github.com/netpro2k/kinect-demos
	open kinect-demos
	
Open the demo of your choice and click "Build -> Build and Run". In general each demo builds off (at least parts) of the previous demoes, so if you are new to OpenFrameworks or ofxKinect you likely want to look at them in the order presented here.

Current demos:

- **Parallax**: Demo showing background removal and parallax effect
- **objManip**: Demo showing on screen object manipulation with hand gestures
- **mKart**: maps hand gestures to keyboard strokes to control "Super Mario Kart"

_Note: libfreenect and consequently ofxFreenect are evolving very rapidly. It is quite likely that these demos will break with certain library updates. Usually fixing the issues is quite trivial, but it is something to be aware of. Also, the thresholds and calibrations for all demos may need to be adjusted to fit your Kinect and your environment_
