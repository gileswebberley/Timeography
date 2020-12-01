#include "timeographer.h"

Timeographer::Timeographer(string filepath)
{
    cout<<"timeographer created for playback of "<<filepath<<"\n";
    vidIn.setType(IS_TYPES::VID_FILE);
    //+needs work, fails on every update somewhere...
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
}

void Timeographer::deleteBuffers()
{
    //delete all of the elements and the pointer
    //free up the free store (is there a song about that!??)
    delete[] buff;
    delete[] timeograph;
    delete[] timeoframe;
    delete[] pixIn;
    //++ I think this all needs to become a class heirarchy tbh
    if(diff_mode || difference_learn) delete[] diffMap;
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
    pixIn = new  unsigned char[grabW*grabH*3];
    cout<<".buffers built\n";
    return true;
}

//can't change these whilst it's running
void Timeographer::setExposure(int exp_t, int t_frames)
{
    //lock whilst the 'shutter is open'...
    if(recording){cerr<<"[error] setExposure called whilst recording\n";return;}
    //flag that buffers are empty to start with
    timeograph_ready = false;
    /*check args, not sure what ranges yet
     * I think I just need to protect against setting
     * it a ridiculous task but want to capture decent
     * amounts of time...
     */
    exp_t = (abs(exp_t)>300)?300:abs(exp_t);//10secs @ 30fps
    t_frames = (abs(t_frames)>1000)?1000:abs(t_frames);//166mins (2hrs 46mins) @ 300exp_t
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
                //the first frame whilst in difference mode
                learnDifference();
            }else if(diff_mode){
                //it's not the first frame in difference mode
                //differenceShutter();
                if(timeframe_cnt >= time_frames){
                    makeTimeograph();
                    //moved this from makeFrame()
                    diff_mode = false; difference_learn = true;
                    timeframe_cnt = 0;
                }
                else if(exposure_cnt<exposure_time){
                    buildDifference();
                }
                else if(timeframe_cnt<time_frames){
                    makeFrame();
                    //moved this from makeFrame()
                    diff_mode = false; difference_learn = true;
                    exposure_cnt = 0;
                }
            }//we're not in difference mode, we're in original mode
            else{
                if(timeframe_cnt >= time_frames){
                    makeTimeograph();
                    timeframe_cnt = 0;
                }
                else if(exposure_cnt<exposure_time){
                    //surely I should be able to check which mode we are here??
                    //if(diff_mode) etc??
                    timeExposure();
                }
                else if(timeframe_cnt<time_frames){
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

//+ be good to make a broadcast event when it's finished an exposure

bool Timeographer::saveAsJpeg(string filename)
{
    if(isReady())
    {
        cout<<"save file as "<<filename<<".jpg\n";
        ofFileDialogResult saveDirResult = ofSystemLoadDialog("Select a folder to save your Timeograph", true);
        if (saveDirResult.bSuccess){
            //implemented scaling with interpolation
            ofPixels tmpResize;
            //copy pixels from graphics card to an ofPixels temp variable
            texOut.readToPixels(tmpResize);
            //the ofPixels resize() allows the interpolation argument
            tmpResize.resize(texOut.getWidth()*2,texOut.getHeight()*2,OF_INTERPOLATE_NEAREST_NEIGHBOR);
            //then an ofImage to allow for the save()
            photo.setFromPixels(tmpResize);
            //this "/" won't work in windows, is there a way to get a localised version? YES??
            string dir_path{ofFilePath::addTrailingSlash(saveDirResult.filePath)};
            return photo.save(dir_path+filename+".jpg",OF_IMAGE_QUALITY_HIGH);
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
    if(timeograph_ready)texOut.draw(0,0,w,h);
    if(info_on) displayInfo();
}

/*
 *
 */
void Timeographer::setupDifference(int d_t, bool outline)
{
    //don't want anything happening whilst recording
    if(recording){cerr<<"[error]setupDifference called whilst recording\n"; return;}
    differenceGrayRgb.allocate(grabW,grabH);
    differenceGray.allocate(grabW,grabH);
    differenceGrayBg.allocate(grabW,grabH);
    differenceGrayAbs.allocate(grabW,grabH);
    //diff map is just black & white
    diffMap = new unsigned char[grabW*grabH];
    difference_learn = true;
    difference_threshold = (abs(d_t)>255)?255:abs(d_t);/*
    //trying to implement frame by frame
    vidIn.toggleJog();*/
    do_outline = outline;
}

//private method for the first frame whilst in difference mode
void Timeographer::learnDifference()
{
    pixIn = vidIn.getPixelRead().getData();
    int inW = vidIn.getWidth();//belt & braces, could be grabW
    int inH = vidIn.getHeight();
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            //int grayIndex = (j/3)*inW+(i/3);
            int rgb = 0;
            while(rgb<3){
                //set image as original frame
                timeoframe[indexIn+rgb] = buff[indexIn+rgb] = pixIn[indexIn+rgb];
                //why have I left this in the loop??
//                difference_learn = false;
//                diff_mode = true;
                ++rgb;
            }
        }
    }
    //should surely be here...
    difference_learn = false;
    diff_mode = true;
    exposure_cnt++;
    differenceGrayBg.setFromPixels(buff,grabW,grabH);
    differenceGrayBg.blur(3);
}

//private method - difference mode equivalent to timeExposure()
void Timeographer::buildDifference()
{

    /*my brain hurts trying to think how to express
      an addition to the background...need to find a way to remove the space left last time
*/
    differenceGrayRgb.setFromPixels(vidIn.getPixelRead());
    //make sure we're looking at the same frame as is being compared for change
    pixIn = differenceGrayRgb.getPixels().getData();
    //gray version of the new frame
    differenceGray = differenceGrayRgb;
    //THIS!! this is how to look at what was in the last frame
    //hence the fact we draw from the buffer!!! so painful to work this out
    differenceGrayBg += differenceGray;
    //blur to make it less pixely
    differenceGray.blur(3);
    //what ISN'T in this frame that is in the last one
    differenceGrayAbs.absDiff(differenceGray, differenceGrayBg);
    differenceGrayAbs.threshold(difference_threshold);
    //this adds an outline to each render, set in setUpDifference()
    if(do_outline) differenceGrayAbs.erode_3x3();
    //set the raw array to reflect the processed absolute difference
    diffMap = differenceGrayAbs.getPixels().getData();
    //#1...better creation of white birds
    differenceGrayBg = differenceGray;

    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int grayIndex = (j/3)*inW+(i/3);
            int rgb = 0;
            //if it's black then it hasn't changed
            if(diffMap[grayIndex] == 0){
            while(rgb<3){
                //YES!! setting from what WAS there last time
                timeoframe[indexIn+rgb] =  buff[indexIn+rgb];
                buff[indexIn+rgb] = timeoframe[indexIn+rgb];
                ++rgb;
            }
            //else it's white so has changed, therefore store
            //the changes in the buffer array
            }else{
                while(rgb<3){
                    //YES!! fuckin done it, can't believe it took so long!!
                buff[indexIn+rgb] = pixIn[indexIn+rgb];
                rgb++;
                }
            }
        }
    }
    exposure_cnt++;
}

void Timeographer::timeExposure()
{
    //cout<<"timeExposure....\n";
    //copied from ofBook but had to add .getData()!??
    //getData returns a ptr to the underlying char[]*/
    pixIn = vidIn.getPixelRead().getData();
    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int rgb = 0;
            while(rgb<3){
                //creating long exposure...
                //posterising fixed with double cast cos it's an int/int
                timeoframe[indexIn+rgb] += (double)pixIn[indexIn+rgb]/(exposure_time);
                rgb++;
            }
        }
    }
    exposure_cnt++;
}

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
                //gonna try removing and adding??
                double nVal = timeoframe[indexIn+rgb];
                double comp = nVal - timeograph[indexIn+rgb];
                //timeograph[indexIn+rgb] += comp/time_frames;
                if(comp > 0.0){
                //adding long exposure to timeograph
                //timeograph[indexIn+rgb] += (double)timeoframe[indexIn+rgb]/(time_frames);
                //try to soften the effect
                timeograph[indexIn+rgb] += (double)comp/(time_frames);
                }else{
                    //subtracting long exposure to timeograph
                    //timeograph[indexIn+rgb] -= (double)timeoframe[indexIn+rgb]/(time_frames);
                    //try to soften the effect
                    timeograph[indexIn+rgb] -= (double)abs(comp)/(time_frames);
                }
                //clear the long exposure
                timeoframe[indexIn+rgb] = 0.f;
                rgb++;
            }
        }
    }
    timeframe_cnt++;
}

void Timeographer::makeTimeograph()
{
    int inW = grabW;
    int inH = grabH;
    for(int j=0;j<inH*3; j+=3){
        for(int i=0;i<inW*3;i+=3){
            int indexIn = j*inW+i;
            int rgb = 0;
            while(rgb<3){
                //create timeograph by rounding from dbl to int
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
    //load data into the oftexture
    texOut.loadData(buff,grabW,grabH,GL_RGB);
    cout<<"timeframe\n";
    recording = false;
}
