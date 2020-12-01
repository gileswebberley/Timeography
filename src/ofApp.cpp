#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    cout<<"IN SETUP....\n";
    //set up listeners for gui, remove in exit()
    exposure_go_button.addListener(this, &ofApp::expGoButtonPressed);
    //Of looks after these pointers, right? Valgrind finds nothing
    //{grabW, grabH, deviceId} for webcam...
    //timeography = new Timeographer{640,480,0};
    //make gui selection for the video file
    ofFileDialogResult open_result = ofSystemLoadDialog("Select an mp4 or mov as Timeograph source");
    if(open_result.bSuccess){
        //if a file was selected
        //cerate ofFile from the selected
        ofFile tmpFile(open_result.getPath());
        if(tmpFile.exists()){
            string tmpExt = ofToLower(tmpFile.getExtension());
            if(tmpExt == "mp4" || tmpExt == "mov"){
                //getBaseName actually gets the name stripped of extension eg "file.jpg"->"file"
                original_name = tmpFile.getBaseName();
                cout<<original_name<<" is being set up as source of Timeographer\n";
                //as it is set to jog it should pause until exposure is set
                timeography = new Timeographer{tmpFile.path()};
                /*use for diff style timeograph or comment out for normal mode
                ** use up to 120ish for low contrast images, otherwise about 10
*/
                //timeography->setupDifference(30,true);
                makeExposureGui();
            }else{
                cerr<<"selected file was not of a type allowable ie mp4 or mov\n";
            }
        }else{
            cerr<<"File selected does not exist\n";
        }
    }else{
        cerr<<"Failed to select a file\n";
    }
    //{fiespath} for video file
//    timeography = new Timeographer{"ed_portrait.mp4"};
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    cout<<"...SETUP COMPLETE\n";
}

void ofApp::makeExposureGui(){
    exposure_settings.setup("Exposure Settings");
    exposure_settings.add(exposure_time.setup("exposure time",30,1,300,300,20));
    exposure_settings.add(exposure_number.setup("frame count",100,1,1000,300,20));
    exposure_settings.add(exposure_go_button.setup("Run"));
}

void ofApp::expGoButtonPressed(){
    /* (exp_t, t_frames) exp_t is how many video frames make up each
     * exposure, whilst t_frames is how many of those exposures will
     * be blended to make up the final timeograph
     */
    orig_exp_t = exposure_time;//need to extract for naming but can't find the int value to make string
    orig_exp_n = exposure_number;
    timeography->setExposure(orig_exp_t,orig_exp_n);
    is_exp_go = true;
    show_gui = false;
}

void ofApp::exit(){
    exposure_go_button.removeListener(this, &ofApp::expGoButtonPressed);
}

//--------------------------------------------------------------
void ofApp::update(){
    if(is_exp_go){
    //shutterRelease returns true whilst it is 'recording'
    if(!timeography->shutterRelease()){
        cout<<"ofApp::update shutter release\n";
        timeography->shutterRelease();
        is_exp_go = false;
    }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //ofSetColor(255);
    if(timeography->isReady()){
        timeography->drawExposure();
        show_gui = true;
    }
    if(show_gui){
        exposure_settings.draw();
    }else{
        timeography->drawInput(true);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    //press space-bar to save the current timeograph
    if(key == ' '){
        //returns false if it's not ready
        timeography->saveAsJpeg("tm-"+original_name+"-"+ofGetTimestampString());
    }/*
       little experiment to manually take frames...
else if(key == OF_KEY_RETURN){
        if(!timeography->shutterRelease()){
            timeography->shutterRelease();
        }
    }*/
}
