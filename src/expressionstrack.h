#ifndef EXPRESSIONSTRACK_H
#define EXPRESSIONSTRACK_H

#import "ofxOpenCv.h"
#import "inputselector2.h"


class expressionsTrack
{
    InputSelector vidIn{IS_TYPES::VID_DEVICE};
    ofxCvHaarFinder finder;
    ofxCvColorImage rgbimg;
    ofxCvGrayscaleImage grayimg;
    //the cropped image area that makes up the bounding box of finder's blobs
    vector<ofImage*> bodyBoxes;
    ofPolyline line;
    vector<ofPolyline> lines;
    vector<ofPoint> positions;
    vector<ofColor> colours;
    int grabW{352}, grabH{288};

public:
    expressionsTrack();
    //constructor that calls setupTrack
    expressionsTrack(string haarFilepath);
    //as we have ptrs around we need to clear up after ourselves
    ~expressionsTrack();
    bool setupTrack(string haarFilepath);
    bool doFinding();
    void drawFindings();
};

#endif // EXPRESSIONSTRACK_H
