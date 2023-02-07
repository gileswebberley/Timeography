#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    cout<<"IN SETUP....\n";
    //Of looks after these pointers, right? Valgrind finds nothing
    //{grabW, grabH, deviceId} for webcam...
    //timeography = new Timeographer{640,480,0};
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    makeExposureGui();
    cout<<"...SETUP COMPLETE\n";
}

bool ofApp::openAndCreateFileTimeographer(){
    //make system selection for the video file
    ofFileDialogResult open_result = ofSystemLoadDialog("Select an mp4 or mov as Timeograph source",false,ofFilePath::getUserHomeDir());
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
                if (timeography != nullptr) {
                    //terrifyingly I think I had introduced a memory leak by not deleting!!
                    //this should call the destructor to clear up all the buffers
                    delete timeography;
                    timeography = nullptr;
                }
                //does that clear up?? seems to be working acc. to valgrind check?
                timeography = new Timeographer{tmpFile.path()};
                //as it is set to jog it should pause until exposure is set

                return true;
            }else{
                cerr<<"selected file was not of a type allowable ie mp4 or mov\n";
                return false;
            }
        }else{
            cerr<<"File selected does not exist\n";
            return false;
        }
    }else{
        cerr<<"Failed to select a file\n";
        return false;
    }
}

void ofApp::makeExposureGui(){
    //set up listeners for gui, remove in exit()
    exposure_go_button.addListener(this, &ofApp::expGoButtonPressed);
    load_video_button.addListener(this, &ofApp::loadVidButtonPressed);
    diff_mode_toggle.addListener(this, &ofApp::diffToggled);
    ofColor bgc{ofColor(42,82,4)};
    ofColor bgtc{ofColor(217,245,191)};
    ofColor bgfc{ofColor::teal};
    //setting default colours does not influence the contained widgets :(
    exposure_settings.setDefaultWidth(500);
    exposure_settings.setDefaultHeight(30);
    exposure_settings.setup("EXPOSURE SETTINGS");
    exposure_settings.setHeaderBackgroundColor(ofColor::teal);
    exposure_settings.setTextColor(ofColor::ivory);
    load_video_button.setBackgroundColor(bgc);
    load_video_button.setTextColor(bgtc);
    exposure_go_button.setBackgroundColor(bgc);
    exposure_go_button.setTextColor(bgtc);
    exposure_time.setFillColor(bgfc);
    exposure_time.setBackgroundColor(bgc);
    exposure_time.setTextColor(bgtc);
    exposure_number.setFillColor(bgfc);
    exposure_number.setBackgroundColor(bgc);
    exposure_number.setTextColor(bgtc);
    difference_threshold.setFillColor(bgfc);
    difference_threshold.setBackgroundColor(bgc);
    difference_threshold.setTextColor(bgtc);
    diff_mode_toggle.setFillColor(bgfc);
    diff_mode_toggle.setBackgroundColor(bgc);
    diff_mode_toggle.setTextColor(bgtc);
    exposure_settings.add(load_video_button.setup("Click to select input file (MP4 or MOV only)"));
    exposure_settings.add(diff_mode_toggle.setup("Set to use Difference Mode",false));
    exposure_settings.add(difference_threshold.setup("Difference (set high for low contrast)",10,1,200));
    exposure_settings.add(exposure_time.setup("EXPOSURE TIME (blur 30 = 1sec)",30,1,300,300));
    exposure_settings.add(exposure_number.setup("FRAME COUNT (blend exposures)",10,1,1000));
    exposure_settings.add(exposure_go_button.setup("Click the box to Run Timeographer"));
}

//listener methods

void ofApp::expGoButtonPressed(){
    /* (exp_t, t_frames) exp_t is how many video frames make up each
     * exposure, whilst t_frames is how many of those exposures will
     * be blended to make up the final timeograph
     */
    if(timeography != nullptr){
        //timeography->clearDifference();
        if(is_diff_mode){
            /*use for diff style timeograph or comment out for normal mode
        ** use up to 120ish for low contrast images, otherwise about 10
false is bool for outline renedering*/
            timeography->setupDifference(difference_threshold,false);
        }else{
            timeography->clearDifference();
        }
        timeography->setExposure(exposure_time,exposure_number);
        is_exp_go = true;
        show_gui = false;
    }
}

void ofApp::loadVidButtonPressed(){
    if(show_gui) openAndCreateFileTimeographer();
}

void ofApp::diffToggled(bool& val){
    cout<<"Difference Mode is "<<val<<"\n";
    if(val != is_diff_mode){
        is_diff_mode = val;
        //if(!val && timeography != nullptr) timeography->clearDifference();
    }
}
void ofApp::exit(){
    exposure_go_button.removeListener(this, &ofApp::expGoButtonPressed);
    load_video_button.removeListener(this, &ofApp::loadVidButtonPressed);
    diff_mode_toggle.removeListener(this, &ofApp::diffToggled);
}

//--------------------------------------------------------------
void ofApp::update(){
    if(is_exp_go){

        //cout << "ofApp::update() is_exp_go == true\n";
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
    if(timeography != nullptr){
        if(timeography->isReady()){
            timeography->drawExposure();
            show_gui = true;
        }
    }
    if(show_gui){
        exposure_settings.draw();
        //doesn't seem to be a textbox in ofxGui?
        string how_to = "A LITTLE HELPING HAND\n"
                        "Simply select a video source file first, remembering that you\n"
                        "can only use either mp4 or mov formats.\n"
                        "Now use the sliders to set the exposure time, which is kinda\n"
                        "how blurry each 'frame' is, and the frame count, which is how\n"
                        "many exposures are blended into the final Timeograph.\n"
                        "I recommend an exp_t [3..30] and t_frames can be worked out\n"
                        "as (length in secs*30)/exp_t.\n At this point you can set it\n"
                        "running and check the progress via the information at the\n"
                        "bottom left of screen.\n"
                        "If you're happy with the result then just hit the spacebar\n"
                        "to save it to a location of your choosing then either\n."
                        "run again from where you left the video or load another.";
        //set the position relative to other gui elements..works nicely
        ofDrawBitmapString(how_to,exposure_settings.getPosition().x,exposure_settings.getPosition().y+exposure_settings.getHeight()+30);
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
    }
}
