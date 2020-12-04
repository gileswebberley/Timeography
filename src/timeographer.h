#ifndef TIMEOGRAPHER_H
#define TIMEOGRAPHER_H

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "inputselector2.h"

class Timeographer
{
    /*ok going to try to implement difference monitoring
     so perhaps we can build just the background then put on changes?
after beating my head got this implemented (see your ranting!?)
but now it is seeming apparent that this should be expressed as a
type hierarchy I think? also, maybe start giving all these
creatitons a pointer to a central InputSelector so they can
all get access to, ooh, it's a singleton isn't it?? shared-ptr
if it's the same device, collection of object ptrs for the files?*/
    ofxCvColorImage differenceGrayRgb;
    ofxCvColorImage differenceRgb;
    ofxCvGrayscaleImage differenceGray;
    ofxCvGrayscaleImage differenceGrayBg;
    ofxCvGrayscaleImage differenceGrayAbs;
    //using input selector to look after file/device
    InputSelector vidIn{IS_TYPES::VID_FILE};
    //always the current timeograph
    ofTexture texOut;
    //used to create the saveable jpeg
    ofImage photo;
    /*buff is the converted version of timeograph that can
    be given to the texOut (char rounded conversion of dbl val)*/
    unsigned char* buff;
    /*the array for the difference map*/
    unsigned char* diffMap;
    /*need to make each frame a non-local variable cos of differenceShutter() */
    const unsigned char* pixIn;
    /*essentially each exposure that makes up the timeograph*/
    double* timeoframe;
    /*this is the actual data representation of the timeograph*/
    double* timeograph;
    /*flags to take care of safe running and state awareness*/
    bool timeograph_ready{false}, recording{false}, info_on{true}, difference_learn{false}, diff_mode{false}, do_outline{false}, diff_has_been{false};
    /*default values for the parameters needed*/
    int grabW{640}, grabH{480}, exposure_time{10},time_frames{30}, exposure_cnt{0}, timeframe_cnt{0}, difference_threshold{10};

    /*create all of the pixel arrays and texture the
    size of the input (got to be careful not to go
    out of bounds when you're dealing with raw arrays*/
    bool buildBuffers();
    //clear up all the arrays and ptrs from the heap
    void deleteBuffers();
    //it's getting messy and inefficient so broke out the if-elses
    //set the background to compare to (difference 1st exposure)
    void learnDifference();
    //add the pixels that are different then update the
    //comparison image (difference exposure)
    void buildDifference();
    //the original exposure frame function
    void timeExposure();
    //add all the exposures to a time frame
    void makeFrame();
    //assemble the time frames into the final image
    void makeTimeograph();

public:
    /*this was causing some trouble so I withdrew it
    Timeographer(IS_TYPES input_type);*/
    //to use a video file
    explicit Timeographer(string filepath);
    /*to use a video grabber at device_id default[0]*/
    Timeographer(int w, int h, int device_id=0);
    /*we're using free-store memory so destructor...*/
    ~Timeographer();
    /*exp_t - the amount of frames to build a sng exposure
    larger numbers will cause blur (a long exposure)
    t-frames - how many exposures are included in the
    final timeograph
    recommendations from playing
    - for a "difference" timeograph exp_t[5..30] t_frames[1..3]
    - for a standards timeograph exp_t[1..50] t_frames[10..80]
*/
    void setExposure(int exp_t, int t_frames);
    /*creates a sng exp_t, loop in update() to run
    in 'real-time' else asign it to a key press
    and build the timeograph with sng frames
    locks itself and returns true whilst recording*/
    bool shutterRelease();
    //false until the first texOut is created
    bool isReady(){return timeograph_ready;}
    //filepath relative to data folder, don't add the extension
    bool saveAsJpeg(string filepath);
    /*s - scale to app window width?
    call in draw() to show the video input*/
    void drawInput(bool s=false);
    //exposure count and frame count overlayed
    //++make positionable?
    void displayInfo();
    //drawExposure(grabW, grabH)
    void drawExposure();
    //display texOut (0,0,w,h)
    void drawExposure(int w, int h);
    //for the difference version....
    //diff_threshold [1..255]
    void setupDifference(int diff_threshold=10, bool outline=false);
    //returns true if it's able to clear up
    bool clearDifference();
};

#endif // TIMEOGRAPHER_H
