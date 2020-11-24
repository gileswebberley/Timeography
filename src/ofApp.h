#pragma once

#include "ofMain.h"
#include "timeographer.h"
//#include "expressionstrack.h"
#include "ofxOpenCv.h"

class ofApp : public ofBaseApp{

    int grabW{640}, grabH{480};
    //create new in setup()
    Timeographer* timeography;
    //expressionsTrack* tracker;

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);

};
