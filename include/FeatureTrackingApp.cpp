


#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

 //includes for background subtraction
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/video.hpp>


#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h" //needed for capture
#include "cinder/Log.h" //needed to log errors

#include "Osc.h" //add to send OSC

#include <math.h>
#include"ExampleSquares.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//for networking
#define LOCALPORT 8887 //we just bind here to send.
#define DESTHOST "127.0.0.1" //this is sending to our OWN computer's IP address
#define DESTPORT 8888 //this is the port we are sending to -- take note. This will have to match in the next code.

//set up our OSC addresses
#define WHERE_OSCADDRESS "/MakeItArt/Where"
#define MOUSEDOWN_OSCADDRESS "/MakeItArt/Down"


class MakeItArtMouseApp : public App {
public:
    void setup() override;
    void keyDown(KeyEvent event) override;
    void mouseDrag(MouseEvent event) override;
    void mouseDown(MouseEvent event) override;
    void mouseUp(MouseEvent event) override;



    void update() override;
    void draw() override;

    MakeItArtMouseApp();

    ci::vec2 curMousePosLastDown;
    bool isMouseDown;


protected:
    CaptureRef                 mCapture;
    gl::TextureRef             mTexture;

    //for framedifferencing
    cv::Mat                    mPrevFrame;
    cv::Mat                    mFrameDifference;
    ci::SurfaceRef             mSurface;

    //Sending OSC
    osc::SenderUdp                mSender; //sends the OSC via the UDP protocol
    void sendOSC(std::string addr, float x, float y); //sending the OSC values
    void sendOSC(std::string addr, float down);//just one -- this is inelegant but effective for now

    void frameDifference(cv::Mat& outputImg);

};

MakeItArtMouseApp::MakeItArtMouseApp() : mSender(LOCALPORT, DESTHOST, DESTPORT) //initializing our class variables
{

}

//sets the mouse drag values
void MakeItArtMouseApp::mouseDrag(MouseEvent event)
{
    curMousePosLastDown = event.getPos();
    isMouseDown = true;

}
//sets the mouse down values
void MakeItArtMouseApp::mouseDown(MouseEvent event)
{
    curMousePosLastDown = event.getPos();
    isMouseDown = true;

}


//sets the mouse up values
void MakeItArtMouseApp::mouseUp(MouseEvent event)
{
    isMouseDown = false;
}


void MakeItArtMouseApp::sendOSC(std::string addr, float x, float y) //sending the OSC values
{
    osc::Message msg;
    msg.setAddress(addr); //sets the address

    msg.append(x);
    msg.append(y);

    mSender.send(msg);
}

void MakeItArtMouseApp::sendOSC(std::string addr, float down) //sending the OSC values
{
    osc::Message msg;
    msg.setAddress(addr); //sets the address

    msg.append(down);

    mSender.send(msg);
}

//initialization
void MakeItArtMouseApp::setup()
{
    //set up our OSC sender and bind it to our local port
    try {
        mSender.bind();
    }
    catch (osc::Exception& e)
    {
        CI_LOG_E("Error binding" << e.what() << " val: " << e.value());
        quit();
    }

    //set up our camera
    try {
        mCapture = Capture::create(640, 480); //first default camera
        mCapture->start();
    }
    catch (ci::Exception& exc)
    {
        CI_LOG_EXCEPTION("Failed to init capture ", exc);
    }


    mPrevFrame.data = NULL;
    mFrameDifference.data = NULL;

}

void MakeItArtMouseApp::keyDown(KeyEvent event)
{
    //TODO: save the current frame as the background image when user hits a key

    //eg:
    if (event.getChar() == ' ')
    {
        //TODO: do a thing. Like save the current frame.
    }

}

void MakeItArtMouseApp::update()
{
    //does frame-differencing stuff
    if (mCapture && mCapture->checkNewFrame()) //is there a new frame???? (& did camera get created?)
    {
        mSurface = mCapture->getSurface();

        if (!mTexture)
            mTexture = gl::Texture::create(*mSurface);
        else
            mTexture->update(*mSurface);

        //do the frame-differencing
        frameDifference(mFrameDifference);

    }

    //send the OSC re: mouse values
    //& normalize the positions to 0. to 1. for easy scaling in processing program
    sendOSC(WHERE_OSCADDRESS, (float)curMousePosLastDown.x / (float)getWindowWidth(), (float)curMousePosLastDown.y / (float)getWindowHeight());
    sendOSC(MOUSEDOWN_OSCADDRESS, isMouseDown);

}


//find the difference between 2 frames + some useful image processing
void MakeItArtMouseApp::frameDifference(cv::Mat& outputImg)
{

    outputImg.data = NULL;
    if (!mSurface) return;

    //the current surface or frame in cv::Mat format
    cv::Mat curFrame = toOcv(Channel(*mSurface));

    if (mPrevFrame.data) {

        //blur --> this means that it will be resilient to a little movement
        //params are: cv::Mat Input1,
//                    cv::Mat Result,
//                    cv::Size - size of blur kernel (correlates to how blurred - must be positive & odd integers),
//                               the bigger the size, the more the blur & also the larger the sigmas the more the blur.
//                    double size of sigma X Gaussian kernel standard deviation in X direction
//                    double size of sigma Y Gaussian kernel standard deviation in Y direction (optional, not used)
//      More on Gaussian blurs here: https://homepages.inf.ed.ac.uk/rbf/HIPR2/gsmooth.htm
//      Interestingly, we can think of them as a low-pass filter in 2D -- (if you know them from audio dsp, sound)
        cv::GaussianBlur(curFrame, curFrame, cv::Size(3, 3), 0);

        //find the difference
        //params are: cv::Mat Input1, cv::Mat Input2, cv::Mat Result
        cv::absdiff(curFrame, mPrevFrame, outputImg);

        //take threshhold values -- think of this as image segmentation, see notes above in desc. header
        //we will go further into image segmentation next week
//        https://docs.opencv.org/2.4/modules/imgproc/doc/miscellaneous_transformations.html?highlight=threshold#threshold
//    Parameters:
//        src – input array (single-channel, 8-bit or 32-bit floating point).
//        dst – output array of the same size and type as src.
//        thresh – threshold value.
//        maxval – maximum value to use with the THRESH_BINARY and THRESH_BINARY_INV thresholding types.
//        type – thresholding type (see the details below).
        cv::threshold(outputImg, outputImg, 50, 255, cv::THRESH_BINARY);

    }

    mPrevFrame = curFrame;
}

void MakeItArtMouseApp::draw()
{
    gl::clear(Color(0, 0, 0));

    gl::color(1, 1, 1, 1);
    //
    //    if( mTexture )
    //    {
    //        gl::draw( mTexture );
    //    }

        //if the frame difference isn't null, draw it.
    if (mFrameDifference.data)
    {
        gl::draw(gl::Texture::create(fromOcv(mFrameDifference)));
    }

}

CINDER_APP(MakeItArtMouseApp, RendererGl,
    [](MakeItArtMouseApp::Settings* settings) //note: this part is to fix the display after updating OS X 1/15/18
    {
        settings->setHighDensityDisplayEnabled(true);
    })
