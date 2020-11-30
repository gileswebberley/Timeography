#pragma once

#include "ofMain.h"
#include "timeographer.h"
//#include "expressionstrack.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

    int grabW{640}, grabH{480};
    //create new in setup()
    Timeographer* timeography;
    //expressionsTrack* tracker;
    //so we're adding a gui, wish me luck
    //just for original mode and file input atm
    ofxIntSlider exposure_time, exposure_number;
    //a button to set things off and running
    ofxButton exposure_go_button;
    bool is_exp_go{false};
    ofxPanel exposure_settings;
    bool show_gui{true};

public:
    void setup();
    void update();
    void draw();
    void exit() override;

    void keyPressed(int key);
    void expGoButtonPressed();
};
