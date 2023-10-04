#include "timeographer.h"

Timeographer::Timeographer(string filepath)
{
    cout<<"timeographer created for playback of "<<filepath<<"\n";
    vidIn.setType(IS_TYPES::VID_FILE);
    if(vidIn.setupInput(filepath, true)){
        //implemented frame by frame (jog) behaviour with true arg
        //I had forgotten this so the buffers were the wrong size...crashed repeatedly
        grabW = vidIn.getWidth();
        grabH = vidIn.getHeight();
        buildBuffers();
    }else{
        cerr<<"Timeographer could not find the specified video file\n";
        ofExit(1);
    }
}

Timeographer::Timeographer(int w, int h, int device_id)
{
    cout<<"timeographer created for deviceId: "<<device_id<<"\n";
    vidIn.setType(IS_TYPES::VID_DEVICE);
    vidIn.setupInput(w,h,device_id);
    grabW = w;
    grabH = h;
    buildBuffers();
}

Timeographer::~Timeographer()
{
    deleteBuffers();
    cout << "Timeographer destructor has run\n";
}

void Timeographer::deleteBuffers()
{
    //delete all of the arrays and clear the ofPixels objects
    //free up the free store (is there a song about that!??)
    if (buff != nullptr) delete[] buff;
    if (timeograph != nullptr) delete[] timeograph;
    if (timeoframe != nullptr) delete[] timeoframe;
    if (texOut.isAllocated()) texOut.clear();
    if (pixIn.isAllocated()) pixIn.clear();
    if (diffMap.isAllocated()) diffMap.clear();   
}

//bool if checking is required later
bool Timeographer::buildBuffers()
{
    cout<<"timeographer building buffers.";
    //GL_RGB for ofTexture, OF_IMAGE_COLOR for ofImage, OF_PIXELS_RGB for ofPixels
    texOut.allocate(grabW,grabH,GL_RGB);
    buff = new unsigned char[grabW*grabH*3];
    cout<<".";
    timeograph = new double[grabW*grabH*3];
    cout<<".";
    timeoframe = new double[grabW*grabH*3];
    cout<<".";
    //pixIn = new unsigned char[grabW*grabH*3];
    //converting to an ofPixels object
    pixIn.allocate(grabW, grabH, GL_RGB);
    cout<<".buffers built\n";
    //this has fixed the bug where it doesn't run on the first attempt
    //essentially we are making a blank image that goes into texOut
    makeTimeograph();
    return true;
}

//can't change these whilst it's running
void Timeographer::setExposure(int exp_t, int t_frames)
{
    //lock whilst the 'shutter is open'
    if(recording){cerr<<"[error] setExposure called whilst recording\n";return;}
    //flag that buffers are empty to start with
    timeograph_ready = false;
    /*check args, not sure what ranges yet
     * I think I just need to protect against setting
     * it a ridiculous task but want to capture decent
     * amounts of time...
     */
    exp_t = (abs(exp_t)>max_exp_t)?max_exp_t:abs(exp_t);
    t_frames = (abs(t_frames)>max_t_frames)?max_t_frames:abs(t_frames);
    exposure_time = exp_t;
    time_frames = t_frames;
    cout<<"Exposure set: exposure time: "<<exposure_time<<"\ttime frames: "<<time_frames<<"\n";
    //a flag to know if it's already set for the next exposure...
    recording = true;
}

//returns whether it is currently recording, use it to poll whether to start another one
bool Timeographer::shutterRelease()
{
    if(recording){
        //check that we have a new frame to process..
        if(vidIn.updateInput()){
            if(difference_learn){
                //the first frame of the exposure whilst in difference mode
                learnDifference();
            }else if(diff_mode){
                //it's not the first frame but is in difference mode
                if(timeframe_cnt >= time_frames){
                    //we have completed the exposures and time frames so we create the final picture and skip to the end
                    //This ends the recording session
                    makeTimeograph();
                    //then reset our flags and counter ready for another time frame on the next run
                    diff_mode = false; difference_learn = true;
                    timeframe_cnt = 0;
                }
                else if(exposure_cnt < exposure_time){
                    //we are still doing an exposure so continue with that and skip to the end
                    buildDifference();
                }
                else if(timeframe_cnt < time_frames){
                    //we have completed an exposure so add it as a time frame
                    makeFrame();
                    //reset flags and counter ready for another exposure if there is one
                    diff_mode = false; difference_learn = true;
                    exposure_cnt = 0;
                }
            }
            else{//we're not in difference mode, we're in original mode
                if(timeframe_cnt >= time_frames){
                    //we have completed the exposures and time frames so we create the final picture and skip to the end
                    makeTimeograph();
                    timeframe_cnt = 0;
                }
                else if(exposure_cnt<exposure_time){
                    //we are still doing an exposure so continue with that and skip to the end
                    timeExposure();
                }
                else if(timeframe_cnt<time_frames){
                    //we have completed an exposure so add it as a time frame
                    makeFrame();
                    exposure_cnt = 0;
                }

            }//end else not a difference
        }
        //we have done our stuff with a new frame
        return true;
    }//end if recording
    //we are not recording
    return false;
}

//+ be good to make a broadcast event when it's finished an exposure?

bool Timeographer::saveAsJpeg(string filename)
{
    cout << "SaveAsJpeg()";
    if(isReady())
    {
        cout << "SaveAsJpeg() image isReady()";
        ofFileDialogResult saveDirResult = ofSystemLoadDialog("Select a folder to save your Timeograph", true, ofFilePath::getUserHomeDir());
        if (saveDirResult.bSuccess){
            cout << "SaveAsJpeg() save directory is"+saveDirResult.filePath;
            filename += " exp_t";
            filename += to_string(exposure_time);
            filename += " exp_c";
            filename += to_string(time_frames);
            filename += "diff"+to_string(difference_learn);
            if (difference_learn)filename += "threshold" + to_string(difference_threshold);
            filename += ".jpg";
            cout<<"save file as "<<filename<<"\n";
            //implemented scaling with interpolation
            ofPixels tmpResize;
            //copy pixels from graphics card to an ofPixels temp variable
            texOut.readToPixels(tmpResize);
            //the ofPixels resize() allows the interpolation argument
            tmpResize.resize(texOut.getWidth()*2,texOut.getHeight()*2,OF_INTERPOLATE_BICUBIC);
            //then an ofImage to allow for the save()
            photo.setFromPixels(tmpResize);
            //this "/" won't work in windows, is there a way to get a localised version? YES
            string dir_path{ofFilePath::addTrailingSlash(saveDirResult.filePath)};
            //save as a full quality jpeg
            return photo.save(dir_path+filename,OF_IMAGE_QUALITY_BEST);
        }
    }
    return false;
}

void Timeographer::drawInput(bool scale)
{
    vidIn.drawInput(scale);
    if(info_on)displayInfo();
}

void Timeographer::displayInfo()
{
    stringstream shutterInfo;
    shutterInfo<<"exposure "<<exposure_cnt<<":"<<exposure_time<<"\t timeframe "<<timeframe_cnt<<":"<<time_frames;
    ofDrawBitmapString(shutterInfo.str(),10,grabH-50);
}

void Timeographer::drawExposure()
{
    drawExposure(grabW,grabH);
}

void Timeographer::drawExposure(int w, int h)
{
    //ofEnableBlendMode(OF_BLENDMODE_SCREEN);
    if(isReady())texOut.draw(0,0,w,h);
    if(info_on) displayInfo();
}

/*
 *
 */
void Timeographer::setupDifference(int d_t, bool outline)
{
    //don't want anything happening whilst recording
    if(recording){cerr<<"[error]setupDifference called whilst recording\n"; return;}
    //make sure the input value is valid
    difference_threshold = (d_t < 0) ? abs(d_t) : d_t;//
    difference_threshold = (difference_threshold >255)?255: difference_threshold;
    if(difference_learn){
        cerr<<"[error] setUpDifference has already been set up, changing threshold...\n";
    }
    differenceGrayRgb.allocate(grabW,grabH);
    differenceGray.allocate(grabW,grabH);
    differenceGrayBg.allocate(grabW,grabH);
    differenceGrayAbs.allocate(grabW,grabH);
    //adding in the last frames difference mask
    previousDifferenceGrayAbs.allocate(grabW, grabH);
    //diff map is just black & white
    diffMap.allocate(grabW,grabH,OF_IMAGE_GRAYSCALE);
    //adding a flag so that this gets cleared up even
    //if this mode has been switched off during a session
    diff_has_been = true;
    difference_learn = true;
    do_outline = outline;
}

bool Timeographer::clearDifference(){
    if(recording) return false;
    //**FIXED with diff_has_been flag - then later by converting diffMap to an ofPixels object
    //this is making it crash when turning off diff mode
    //but it feels like this means that it won't clear up
    //the memory in the destructor?? that's a leak right?
    //if(diff_mode || difference_learn) delete [] diffMap;
    difference_learn = false;
    diff_mode = false;
    do_outline = false;
    return true;
}

//private method for the first frame whilst in difference mode
void Timeographer::learnDifference()
{
    pixIn = vidIn.getPixelRead();//.getData();
    int inW = vidIn.getWidth();//belt & braces, could be grabW
    int inH = vidIn.getHeight();
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            //int grayIndex = (j/3)*inW+(i/3);
            int rgb = 0;
            while(rgb<3){
                //set image as original frame
                timeoframe[indexIn + rgb] = buff[indexIn + rgb] = (double)pixIn[indexIn+rgb];
                ++rgb;
            }
        }
    }
    //should surely be here...
    difference_learn = false;
    diff_mode = true;
    exposure_cnt++;
    differenceGrayBg.setFromPixels(buff,grabW,grabH);
    //differenceGrayBg.blur(difference_blur);
    //trying to look after the first frame using the previous abs image - hmm docs say value of 1.0-255.0 but I'm testing for 0 
    differenceGrayAbs.set(0.0f);
    //this clears the 'mask history' if using that
    previousDifferenceGrayAbs.set(0.0f);
}

//difference mode equivalent to timeExposure() which builds alternative version of timeoframes
    /*
    This looks at the changes from frame to frame to produce trails
    It uses a mask produced by the absDiff method ofxCvGrayscaleImage and compares
    it with the previous frame's mask to record what new and not what changed last time
*/
void Timeographer::buildDifference()
{
    //This is the latest frame from the imputSelector object - this is the ofxCvColorImage that can be converted to b&w for creating the mask
    differenceGrayRgb.setFromPixels(vidIn.getPixelRead());
    //This is the ofPixels object that is used as the master copy of this current frame - converted from raw array during development
    pixIn = differenceGrayRgb.getPixels();
    //automagically convert to a grayscale version of the new frame
    differenceGray = differenceGrayRgb;
    //THIS!! this is how to look at what was in the last frame
    //hence the fact we draw from the buffer!!! so painful to work this out
    differenceGrayBg += differenceGray;

    //'Mask History' mode - this is an attempt to make the trails nicer by not overwritting what has already been written to throughout the 'exposure'
    //previousDifferenceGrayAbs += differenceGrayAbs;
    //previousDiffMap = previousDifferenceGrayAbs.getPixels();
    
    //previous frame's mask - made it black in the setup for first frame - this is the preffered altenative to the above
    previousDiffMap = differenceGrayAbs.getPixels();
    //what ISN'T in this frame that is in the last one - create the new 'mask'
    differenceGrayAbs.absDiff(differenceGrayBg, differenceGray);//swapped these round 05-09-23 to do bird tracking picture - made it work again
    differenceGrayAbs.threshold(difference_threshold);
    //this adds an outline to each render, set in setUpDifference()
    if(do_outline) differenceGrayAbs.erode_3x3();
    //convert the absDiff mask to this ofPixels object for use in the building loop below
    diffMap = differenceGrayAbs.getPixels();
    //differenceGrayBg = differenceGray;//this got rid of the irritating line around scans, it suddenly just looked defunct when I came back to this

    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int grayIndex = (j/3)*inW+(i/3);
            int rgb = 0;
            //if it's black then it hasn't changed
            if(diffMap[grayIndex] == 0 || previousDiffMap[grayIndex] >= 1){
            while(rgb<3){
                //YES!! setting from what WAS there last time
                timeoframe[indexIn+rgb] =  buff[indexIn+rgb];
                buff[indexIn+rgb] = timeoframe[indexIn+rgb];
                ++rgb;
            }
            //else it's white so has changed, therefore store
            //the changes in the buffer array - added if it changed last time/before don't overwrite
            }else if(diffMap[grayIndex] >= 1 && previousDiffMap[grayIndex] == 0){
                while(rgb<3){
                    //YES!! fuckin done it, can't believe it took so long!!
                    buff[indexIn + rgb] = pixIn[indexIn + rgb];
                    timeoframe[indexIn + rgb] = buff[indexIn + rgb];
                rgb++;
                }
            }
        }
    }
    exposure_cnt++;
}

//this is the equivalent of a traditional long exposure in camera
void Timeographer::timeExposure()
{
    //cout<<"timeExposure....\n";
    //getData returns a ptr to the underlying char[]*/
    //now changed to ofPixels for stability
    pixIn = vidIn.getPixelRead();//.getData();
    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int rgb = 0;
            while(rgb<3){
                //creating long exposure...
                //posterising fixed with double cast cos it's char/int
                timeoframe[indexIn+rgb] += (double)pixIn[indexIn+rgb]/(exposure_time);
                //make sure the value doesn't overflow
                timeoframe[indexIn + rgb] = clampBounds(timeoframe[indexIn + rgb]);
                rgb++;
            }
        }
    }
    exposure_cnt++;
}

//This is the timeograph style exposure, where shadow has an effect within 'exposed' areas of the frame
void Timeographer::makeFrame()
{
    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int rgb = 0;
            while(rgb<3){
                //want the shadows to have as much affect as
                //the highlights....
                double nVal = timeoframe[indexIn+rgb];
                //as timeograph is slowly built up from tiny exposures we have to adjust the new frame so that it is at
                //the equivalent point of exposure to compare, this makes the whole point of it work a bit better
                double comp = (nVal/time_frames) - timeograph[indexIn + rgb]/timeframe_cnt;
                //if the new value is brighter than already exists
                if(comp >= 0.0 || diff_mode){//think it was making the image a negative when in diff mode
                    //adding long exposure to timeograph
                    timeograph[indexIn+rgb] += (double)nVal/(time_frames);
                }else{
                    //subtracting long exposure to timeograph 
                    timeograph[indexIn + rgb] -= (double)nVal / time_frames;
                }
                //check the values for the 0-255 bounds
                timeograph[indexIn + rgb] = clampBounds(timeograph[indexIn + rgb]);
                //clear the long exposure
                timeoframe[indexIn+rgb] = 0.f;
                rgb++;
            }
        }
    }
    timeframe_cnt++;
}

//create a little utility to clamp the pixel values to 0-255
double Timeographer::clampBounds(double in)
{
    double out = (in > 255.0) ? 255.0 : in;
    out = (out < 0.0) ? 0.0 : out;
    return out;
}

//Create the final image from the timeograph buffer and clear both that and the final timeoframe from which it made up of
void Timeographer::makeTimeograph()
{
    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int rgb = 0;
            while(rgb<3){
                //create timeograph by rounding from dbl to int, floor to round down (avoid peaking)
                buff[indexIn+rgb] = (int)floor(timeograph[indexIn+rgb]);
                //clear the timeograph and frame data
                timeoframe[indexIn+rgb] = 0.f;
                timeograph[indexIn+rgb] = 0.f;
                rgb++;
            }
        }
    }
    //flag set when first available
    timeograph_ready = true;
    //load data into the oftexture from the buffer we've just populated
    texOut.loadData(buff,grabW,grabH,GL_RGB);
    cout<<"timeframe\n";
    //switch off the shutterRelease loop
    recording = false;
}
