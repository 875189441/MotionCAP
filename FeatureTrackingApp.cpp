#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/video.hpp>
 //includes for background subtraction
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/video.hpp>
#include "opencv2/features2d.hpp"

#include "CinderOpenCV.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h" //add - needed for capture
#include "cinder/Log.h" //add - needed to log errors
#include "ExampleSquares.h"
#define SAMPLE_WINDOW_MOD 300 //how often we find new features -- that is 1/300 frames we will find some features
#define MAX_FEATURES 300 //The maximum number of features to track. Experiment with changing this number


using namespace cinder;
using namespace ci::app;
using namespace std;

class FeatureTrackingApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
    void keyDown(KeyEvent event) override;
   
    
protected:
    CaptureRef                 mCapture; //uses video camera to capture frames of data.
    gl::TextureRef             mTexture; //the current frame of visual data in OpenGL format.
   
    //for optical flow
    vector<cv::Point2f>        mPrevFeatures, //the features that we found in the last frame
                               mFeatures; //the feature that we found in the current frame
    cv::Mat                    mPrevFrame; //the last frame
    ci::SurfaceRef             mSurface; //the current frame of visual data in CInder format.
    vector<uint8_t>            mFeatureStatuses; //a map of previous features to current features
    cv::Ptr<cv::BackgroundSubtractor> mBackgroundSubtract;
    void findOpticalFlow(); //finds the optical flow -- the visual or apparent motion of features (or persons or things or what you can detect/measure) through video
    void frameDifference(cv::Mat& outputImg);
    bool mOpticalFlow = false;
   
    cv::Mat                    mFrameDifference;
    SquaresFrameDiff           squareFrame; 
    SquaresFeatures            squareFea; 
    int N = 1;
};

void FeatureTrackingApp::setup()
{
    //set up our camera
    try {
        mCapture = Capture::create(640, 480); //first default camera
        mCapture->start();
    }
    catch( ci::Exception &exc)
    {
        CI_LOG_EXCEPTION( "Failed to init capture ", exc ); //oh no!!
    }
    
    mPrevFrame.data = NULL; //initialize our previous frame to null since in the beginning... there no previous frames!
    mBackgroundSubtract = cv::createBackgroundSubtractorKNN();
}

//maybe you will add mouse functionality!
void FeatureTrackingApp::mouseDown( MouseEvent event )
{

}

void FeatureTrackingApp::update()
{
    //update the current frame from camera
    if(mCapture && mCapture->checkNewFrame()) //is there a new frame???? (& did camera get created?)
    {
        mSurface = mCapture->getSurface(); //get the current frame and put in the Cinder datatype.
        
        //translate the cinder frame (Surface) into the OpenGL one (Texture)
        if(! mTexture)
            mTexture = gl::Texture::create(*mSurface);
        else
            mTexture->update(*mSurface);
    }
    
    if (!mOpticalFlow) {
        findOpticalFlow();
    }
    else  frameDifference(mFrameDifference);
}

void FeatureTrackingApp::findOpticalFlow()
{
    if(!mSurface) return; //don't go through with the rest if we can't get a camera frame!
    
    //convert gl::Texturer to the cv::Mat(rix) --> Channel() -- converts, makes sure it is 8-bit
    cv::Mat curFrame = toOcv(Channel(*mSurface));
    
    
    //if we have a previous sample, then we can actually find the optical flow.
    if( mPrevFrame.data ) {
        
        // pick new features once every SAMPLE_WINDOW_MOD frames, or the first frame
        
        //note: this means we are abandoning all our previous features every SAMPLE_WINDOW_MOD frames that we
        //had updated and kept track of via our optical flow operations.
        
        if( mFeatures.empty() || getElapsedFrames() % SAMPLE_WINDOW_MOD == 0 ){
            
            /*
             parameters for the  call to cv::goodFeaturesToTrack:
             curFrame - img,
             mFeatures - output of corners,
             MAX_FEATURES - the max # of features,
             0.005 - quality level (percentage of best found),
             3.0 - min distance
             
             note: its terrible to use these hard-coded values for the last two parameters. perhaps you will fix your projects.
             
             note: remember we're finding corners/edges using these functions
             */
            cv::goodFeaturesToTrack( curFrame, mFeatures, MAX_FEATURES, 0.005, 3.0 );
        }
        
        vector<float> errors; //there could be errors whilst calculating optical flow
        
        mPrevFeatures = mFeatures; //save our current features as previous one
        
        //This operation will now update our mFeatures & mPrevFeatures based on calculated optical flow patterns between frames UNTIL we choose all new features again in the above operation every SAMPLE_WINDOW_MOD frames. We choose all new features every couple frames, because we lose features as they move in and out frames and become occluded, etc.
        if( ! mFeatures.empty() )
            cv::calcOpticalFlowPyrLK( mPrevFrame, curFrame, mPrevFeatures, mFeatures, mFeatureStatuses, errors );
        
    }
    
    //set previous frame
    mPrevFrame = curFrame;
    
}
 
void FeatureTrackingApp::keyDown(KeyEvent event)
{
    
    if (event.getChar() == '1') {
     

        squareFrame.setN(10);
        squareFea.setN(10);
    }
    if (event.getChar() == '2')
    {
        N = 20;
        squareFrame.setN(30);
        squareFea.setN(30);
    }
    if (event.getChar() == '3')
    {
        N = 40;
        squareFrame.setN(60);
        squareFea.setN(60);
    }
    if (event.getChar() == 'f') {
        mOpticalFlow = !mOpticalFlow;

    }
}
void FeatureTrackingApp::draw()
{
    gl::clear(Color(0, 0, 0));

    //color the camera frame normally
    gl::color(1, 1, 1, 0.55);


    //draw the camera frame
    if (mTexture)
    {
        gl::draw(mTexture);
    }

    // draw all the old points @ 0.5 alpha (transparency) as a circle outline
    gl::color(1, 0, 0, 0.55);
    for (int i = 0; i < mPrevFeatures.size(); i++)
        gl::drawStrokedCircle(fromOcv(mPrevFeatures[i]), 3);


    // draw all the new points @ 0.5 alpha (transparency)
    gl::color(0, 0, 1, 0.5f);
    for (int i = 0; i < mFeatures.size(); i++)
        gl::drawSolidCircle(fromOcv(mFeatures[i]), 3);

    //draw lines from the previous features to the new features
    //you will only see these lines if the current features are relatively far from the previous
    gl::color(0, 1, 0, 0.5f);
    gl::begin(GL_LINES);
    for (size_t idx = 0; idx < mFeatures.size(); ++idx) {
        if (mFeatureStatuses[idx]) {
            gl::vertex(fromOcv(mFeatures[idx]));
            gl::vertex(fromOcv(mPrevFeatures[idx]));
        }
    }
    gl::end();
    if (mOpticalFlow) {
        squareFea.drawRect(mFeatures);

    }
    else if
         (!mOpticalFlow) {
            gl::draw(gl::Texture::create(fromOcv(mFrameDifference)));
            squareFrame.drawRect(mFrameDifference);
        }


}

CINDER_APP( FeatureTrackingApp, RendererGl )
