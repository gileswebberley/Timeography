#pragma once

#include "ofMain.h"
#include "timeographer.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

    int grabW{640}, grabH{480};
    //create new in setup() set to nullPtr for initialisation/instansiation thing
    Timeographer* timeography{nullptr};
    /*add in the name of the loaded video
     *so it can be used for saving
     */
    string original_name;
    //so we're adding a gui, wish me luck
    //just for original mode and file input atm
    ofxIntSlider exposure_time, exposure_number;
    //a button to set things off and running
    ofxButton exposure_go_button,load_video_button;
    ofxPanel exposure_settings;
    //flags for flow control
    bool show_gui{true}, is_exp_go{false};

    void makeExposureGui();
    bool openAndCreateFileTimeographer();

public:
    void setup();
    void update();
    void draw();
    void exit() override;

    void keyPressed(int key);
    //listener methods
    void expGoButtonPressed();
    void loadVidButtonPressed();
};
