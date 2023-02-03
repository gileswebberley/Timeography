#include "inputselector2.h"

InputSelector::InputSelector()
{
    //InputSelector(NO_INPUT);
}
InputSelector::InputSelector(IS_TYPES type):type_flag{type}{

}

void InputSelector::setType(IS_TYPES type){
    type_flag = type;
}

//maybe for image sequence make one that takes an ofDirectory obj?

bool InputSelector::setupInput(string filepath, bool jog)
{
    if(type_flag == IS_TYPES::VID_FILE)
    {
        //using the gstreamer method to get the jog working reliably (not working)
//        if(jog) {auto player = std::make_shader<ofGstVideoPlayer>();
//        player.setFrameByFrame(true);
//        vidPlayer.setPlayer(player);}
        if(vidPlayer.load(filepath)){
            cout<<"VID_FILE has loaded....\n";
            isWidth = vidPlayer.getWidth();
            isHeight = vidPlayer.getHeight();
            ofSetVerticalSync(true);
            //of_loop_palindrome causes critical GStreamer error
            vidPlayer.setLoopState(OF_LOOP_NORMAL);
            vidPlayer.setVolume(0.f);
            vidPlayer.play();
            if(toggleJog() != jog)toggleJog();
            return true;
        }
        cerr<<"InputSelector: could not load "<<filepath<<"\n";
        return false;
    }
    cerr<<"InputSelector: tried to load a file whilst type was not IS_TYPES::VID_FILE\n";
    return false;
}

bool InputSelector::toggleJog()
{
    if(is_jog){
        cout<<"InputSelector: jog mode off\n";
        is_jog = false;
        if(type_flag == IS_TYPES::VID_FILE)vidPlayer.setPaused(false);
    }else{
        cout<<"InputSelector: jog mode is on\n";
        is_jog = true;
        if(type_flag == IS_TYPES::VID_FILE)vidPlayer.setPaused(true);
    }
    return is_jog;
}

bool InputSelector::setupInput()
{
    if(setupInput(640,360,0))return true;
    return false;

}

/*
 * need to add auto max resolution see https://forum.openframeworks.cc/t/list-available-webcam-resolutions/28420
 */
bool InputSelector::setupInput(int w, int h, int deviceId){
    if(type_flag == IS_TYPES::VID_DEVICE)
    {
        cout<<"in VID_DEVICE setup......\n";
        vidGrabber.setVerbose(true);
        vidGrabber.listDevices();
        vidGrabber.setDeviceID(deviceId);
        if(vidGrabber.initGrabber(w,h)){
            cout<<"VID_DEVICE is initialised...\n";
            isWidth = vidGrabber.getWidth();
            isHeight = vidGrabber.getHeight();
            ofSetVerticalSync(true);
            return true;
        }
        cerr<<"InputSelector: could not initialise the device with id:"<<deviceId<<"\n";
        return false;
    }
    cerr<<"InputSelector: tried to set up a video device whilst type was not IS_TYPES::VID_DEVICE\n";
    return false;
}

bool InputSelector::updateInput(){
    if(type_flag == IS_TYPES::VID_DEVICE){
        vidGrabber.update();
        if(vidGrabber.isFrameNew())return true;
        return false;
    }
    if(type_flag == IS_TYPES::VID_FILE){
        vidPlayer.update();
        //didn't loop in jog mode...
        if(vidPlayer.getIsMovieDone()){
            vidPlayer.firstFrame();
        }
        if(vidPlayer.isFrameNew()){
            //try to make frame shuttle work in here?
            //ok, so had to pause the video then put this inside the isFrameNew test
            if(is_jog)vidPlayer.nextFrame();
            return true;
        }
        return false;
    }
    //no input is set
    return false;
}

const ofPixels& InputSelector::getPixelRead()
{
    if(type_flag == IS_TYPES::VID_DEVICE){
        if(vidGrabber.isFrameNew()){
            currentPixels = vidGrabber.getPixels();
        }
    }else if(type_flag == IS_TYPES::VID_FILE){
        if(vidPlayer.isFrameNew()){
            currentPixels = vidPlayer.getPixels();
        }
    }
    return currentPixels;
}

void InputSelector::drawInput(){drawInput(false);}

void InputSelector::drawInput(bool scale)
{
    float sc;
    if(type_flag == IS_TYPES::VID_DEVICE){
        //+new
        if(scale){
            sc = ofGetWidth()/vidGrabber.getWidth();
            ofScale(sc);
        }
        vidGrabber.draw(0,0);
        if(scale) ofScale(1);
    }else if(type_flag == IS_TYPES::VID_FILE){
        //+new
        if(scale){
            sc = ofGetWidth()/vidPlayer.getWidth();
            ofScale(sc);
        }
        vidPlayer.draw(0,0);
        if(scale) ofScale(1);
    }
}
