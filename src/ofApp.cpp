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
    //if(openAndCreateFileTimeographer()) cout<<"...SETUP COMPLETE\n";
}

bool ofApp::openAndCreateFileTimeographer(){
    //make system selection for the video file
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
                if(timeography != nullptr) timeography = nullptr;
                //does that clear up?? seems to be working acc. to valgrind check?
                timeography = new Timeographer{tmpFile.path()};
                //as it is set to jog it should pause until exposure is set
                /*use for diff style timeograph or comment out for normal mode
                ** use up to 120ish for low contrast images, otherwise about 10
*/
                //timeography->setupDifference(30,true);

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
    ofColor bgc{ofColor(42,82,4)};
    ofColor bgtc{ofColor(217,245,191)};
    ofColor bgfc{ofColor::teal};//bgfc{ofColor(98,171,38)};
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
    //exposure_settings.add(exp_information.setup(how_to,500,300));
    exposure_settings.add(load_video_button.setup("Click to select input file (MP4 or MOV only)"));
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
        timeography->setExposure(exposure_time,exposure_number);
        is_exp_go = true;
        show_gui = false;
    }
}

void ofApp::loadVidButtonPressed(){
    if(show_gui) openAndCreateFileTimeographer();
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
    if(timeography != nullptr){
    if(timeography->isReady()){
        timeography->drawExposure();
        show_gui = true;
    }
    }
    if(show_gui){
        exposure_settings.draw();

        string how_to =
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
        "to save it to a location of your choosing.";
        ofDrawBitmapString(how_to,13,exposure_settings.getHeight()+30);
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
