A quick demo of on-screen object manipulation with hand gestures

A captured clean depthmap and threshold are used on the live depthmap to filter out everything but my hands. Then blob detection is used to locate their centers. This information is then used to scale and rotate an onscreen object.

Note that because the Kinect provides depth information, the object can be rotated on both its Z and Y axis. With a bit of work, a gesture could theoretically also be made to rotate about the X axis.

Check out the [Video](http://vimeo.com/17045326)

_sorry about the flickering, this is an artifact of screen recored I am using, and is not visible in actual use_