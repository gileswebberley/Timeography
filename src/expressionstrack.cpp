#include "expressionstrack.h"

expressionsTrack::expressionsTrack()
{
    //set the live video device up with inputSelector
    vidIn.setupInput(grabW,grabH);
    cout<<"Constructor of tracker\n";
}

expressionsTrack::expressionsTrack(string haarpath)
{
    cout<<"Constructor of tracker\n";
    vidIn.setupInput(grabW,grabH);
    setupTrack(haarpath);
}

expressionsTrack::~expressionsTrack()
{
    for(ofImage* im : bodyBoxes){
        delete im;
    }
}


bool expressionsTrack::setupTrack(string haarpath)
{
    cout<<"TRACKER SETUP\n";
    finder.setup(haarpath);
    rgbimg.allocate(grabW,grabH);
    rgbimg.setUseTexture(true);
    grayimg.allocate(grabW,grabH);
    //not drawing so using the texture makes it more efficient I think
    grayimg.setUseTexture(true);
    return true;
}

bool expressionsTrack::doFinding()
{
    if(vidIn.updateInput()){
        rgbimg.setFromPixels(vidIn.getPixelRead());
        grayimg.setFromColorImage(rgbimg);
        finder.findHaarObjects(grayimg);
        //if we haven't found anything then give up this time
        if(finder.blobs.size()==0)return false;
        int blobCnt = finder.blobs.size();
        bodyBoxes.clear();
        //positions.clear();
        colours.clear();
//        points.clear();
        bodyBoxes.assign(blobCnt,new ofImage);
        positions.assign(blobCnt,ofPoint{0,0});
        //lines.assign(blobCnt,ofPolyline);
        colours.assign(blobCnt, ofColor{0});
        //cout<<"Tracker has found "<<blobCnt<<" blobs\n";
        for(int i=0; i < blobCnt; i++){
            ofRectangle bb = finder.blobs[i].boundingRect;
            bodyBoxes.at(i)->cropFrom(rgbimg.getPixels(),bb.x,bb.y,bb.width,bb.height);
            positions.at(i) = finder.blobs[i].centroid;//ofPoint(bb.x,bb.y);
            line.addVertex(positions.at(i));
            //keep the line short by removing older points
            if(line.size()>20){
                line.getVertices().erase(line.getVertices().begin());
            }
//            for(int j=0;j<finder.blobs[i].nPts; j++){
//                points.push_back(finder.blobs[i].pts.at(j));
//            }
        }
        return true;
    }
    return false;
}

void expressionsTrack::drawFindings()
{
    int bbc = 0;
    //draw a rectangle without full alpha to fade...
    ofSetColor(255,255,255,60);
    ofDrawRectangle(0,0,ofGetWidth(),ofGetHeight());
    ofSetColor(255);
    vidIn.drawInput();
    for(ofImage* img : bodyBoxes){
        //img->mirror(false,true);
        int iw, ih;
        float ix,iy;
        iw = img->getWidth();
        ih = img->getHeight();
        ix = positions.at(bbc).x;
        iy = positions.at(bbc).y;
        ofColor samplecolour = img->getColor(iw/2,ih-3);
        colours.at(bbc) = samplecolour;
        cout<<"Tracker: drawing img #"<<bbc<<" at position "<<ix<<":"<<iy<<" with colour "<<samplecolour<<"\n";
        float sc = ofGetWidth()/vidIn.getWidth();
        //img->draw(ix*sc,0,iw,ih);
        ofSetColor(samplecolour);
        //ofDrawCircle(ix*sc,ofRandom(100,ofGetHeight()), iw/10);
        line.simplify(3.0);
        int ptcnt = 0;
        ofSetLineWidth(3);
        ofScale(sc);
        for(auto& pt : line.getVertices())
        {
            //bit of random movement
            pt.x += ofRandomf();//-10,10);
            pt.y += ofRandomf();//ofRandom(-10,10);
            ofVec3f norm = line.getNormalAtIndex(ptcnt)*50;
            //ofRotateDeg(10);hmm, no not really...
            ofDrawCurve(pt.x,pt.y,pt.x-norm.x,pt.y-norm.y,pt.x+norm.x,pt.y+norm.y,pt.y,pt.x);
            ptcnt++;
        }
        //line.draw();
        bbc++;
    }
}
