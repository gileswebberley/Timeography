#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    cout<<"IN SETUP....\n";
    //{grabW, grabH, deviceId} for webcam...
    //Of looks after these pointers, right? Valgrind finds nothing
    //timeography = new Timeographer{640,480,0};
    timeography = new Timeographer{"ed_portrait.mp4"};
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    //use for diff style timeograph or comment out for normal mode
    //use up to 120ish for low contrast images, otherwise about 10
    //timeography->setupDifference(30,true);
    timeography->setExposure(60,17);
    cout<<"...SETUP COMPLETE\n";
}

//--------------------------------------------------------------
void ofApp::update(){
    if(!timeography->shutterRelease()){
        timeography->shutterRelease();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //ofSetColor(255);
    timeography->drawInput(true);
    if(timeography->isReady())timeography->drawExposure();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    //press space-bar to save the current timeograph
    if(key == ' '){
        //returns false if it's not ready
        timeography->saveAsJpeg("timeograph"+ofGetTimestampString());
    }/*
       little experiment to manually take frames...
else if(key == OF_KEY_RETURN){
        if(!timeography->shutterRelease()){
            timeography->shutterRelease();
        }
    }*/
}
