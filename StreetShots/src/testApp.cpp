#include "testApp.h"
#include "ofxXmlSettings.h"

//--------------------------------------------------------------
void testApp::setup(){
    
	cam.setFarClip(10000.0f);
    ofSetFrameRate(60);
    ofBackground(0);
    ofHideCursor();
	ofToggleFullscreen();
	//scrubMode = true;
	scrubMode = false;
	//debugView = true;
	debugView = false;

	filler.setIterations(2);
	filler.setKernelSize(1);

	rx.setup(1400);
    if(!playlist.loadFile("playlist.xml")){
        ofLogError("Error loading playlist xml");
    }
    string takesfolder = playlist.getValue("takesfolder", "");
    cout << "takes folder " << takesfolder << endl;
    int numTakes = playlist.getNumTags("take");
    for(int i = 0; i < numTakes; i++){
        StreetTake take;
        take.xmlIndex = i;
        playlist.pushTag("take", i);
        take.path = ofToDataPath(takesfolder + "/" + playlist.getValue("folder", ""));
        take.images.loadSequence( take.path );        
        take.intime  = playlist.getValue("intime", 0);
		take.outtime = playlist.getValue("outtime", int(take.images.getDurationInMillis()));
		take.name = playlist.getValue("name", "noname");
		//take.outtime = take.images.getDurationInMillis();
		cout << take.intime << " " << take.outtime << " " << take.images.getDurationInMillis() << endl;
        take.durationInSeconds = playlist.getValue("duration", 10);

        playlist.popTag();
        sequences.push_back(take);

		//TEMP PLEASE REMOVE
//		if(i == 1) break;
    }
    currentTake = 0;
	string host1 = playlist.getValue("ip1", "localhost");
	string host2 = playlist.getValue("ip2", "localhost");
	int port = playlist.getValue("port", 1200);
	cout << "setting up ip " << host1 << " " << host2 << " port " << port << endl;
	sendera.setup(host1, port);
	senderb.setup(host2, port);
	

	startTime = ofGetElapsedTimeMillis();
}

//--------------------------------------------------------------

void loadMatrix(ofxOscMessage & msg, ofMatrix4x4 & matrix)
{
	if (msg.getNumArgs() != 16)
		return;

	for(int i = 0; i < 16; i++){
		matrix.getPtr()[i] = msg.getArgAsFloat(i);
	}
}

void testApp::update(){

	while(rx.hasWaitingMessages()){
		ofxOscMessage msg;
		rx.getNextMessage(&msg);

		if(msg.getAddress() == "/view")
		{
			loadMatrix(msg, view);
		}
		if(msg.getAddress() == "/projection")
		{
			loadMatrix(msg, projection);			
		}
	}

	if(scrubMode){
        long time = ofMap(mouseX, 0, ofGetWidth(), 0, sequences[currentTake].images.getDurationInMillis(), true);
        sequences[currentTake].images.selectTime(time);
    }
}

//--------------------------------------------------------------
void testApp::draw(){
	drawPointcloud();
    if(scrubMode){
        ofSetColor(255);
        ofDrawBitmapString("Current Path " + sequences[currentTake].path, 50, 50);
    }
    else{
		long duration = (sequences[currentTake].outtime - sequences[currentTake].intime);
		long currentMillis = (ofGetElapsedTimeMillis() - startTime);
		long pointInTrack = currentMillis % sequences[currentTake].images.getDurationInMillis();
		//cout << "duration is " << duration << " current millis " << currentMillis << " point in track " << pointInTrack << endl;
		if(currentMillis/1000.0 > sequences[currentTake].durationInSeconds){
            nextTake();			
		}
		else{
			sequences[currentTake].images.selectTime(pointInTrack);    		
		}
		
		//cout << "start time " << startTime << " millis " << currentMillis << " " << sequences[currentTake].images.getDurationInMillis() << endl;
        if(currentMillis < sequences[currentTake].outtime){
			
		}
        else {
        }
    }
}

void testApp::drawPointcloud(){

    ofShortPixels& pix = sequences[currentTake].images.getPixels();
    filler.close(pix);

	glEnable(GL_DEPTH_TEST);
	ofMesh mesh;

	ofRectangle rect = ofRectangle(0,0, ofGetWidth(), ofGetHeight());
    glEnable(GL_POINT_SMOOTH);
    //glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);	// allows per-point size
    glPointSize(1.5);
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	int lightPlane = 3000;
	if(sequences[currentTake].name == "jenny"){
		lightPlane = 1600;
	}
	else if(sequences[currentTake].name == "alice"){
		lightPlane = 2700;
	}
	else if(sequences[currentTake].name == "alasdair"){
		lightPlane = 2700;
	}
	else if(sequences[currentTake].name == "lisa"){
		lightPlane = 3200;
	}

	for(int y = 0; y < 480; y++){
		for(int x = 0; x < 640; x++){
			//0.104200 ref dist 120.000000
			double ref_pix_size = 0.104200;
			double ref_distance = 120.000000;
			double wz = pix.getPixels()[y*640+x];
			double factor = 2 * ref_pix_size * wz / ref_distance;
			double wx = (double)(x - 640/2) * factor;
			double wy = (double)(y - 480/2) * factor;
            mesh.addVertex(ofVec3f(wx,-wy,-wz));
			float color = ofMap(abs(wz-lightPlane), 0, 1500, 1.0, .0, true);
			color *= color;
			mesh.addColor(ofFloatColor(color,color,color,1.0));
		}
	}
    

	if (debugView) {
		cam.begin(rect);
	} else {
		ofPushView();
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projection.getPtr());
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(view.getPtr());
	}

	glEnable(GL_DEPTH_FUNC);
	ofPushMatrix();
	ofTranslate(2.0f, 1.0f, 0.0f);
	ofScale(0.001f, 0.001f, 0.001f);
	mesh.drawVertices();

	ofPopMatrix();
	if (debugView){
		
		ofPushStyle();
		ofSetColor(100,50,50);
		ofDrawGrid(10,5,true);
		ofSetColor(255);
		ofPushMatrix();
		glMultMatrixf((view * projection).getInverse().getPtr());
		ofNoFill();
		ofBox(2.0f);
		ofPopMatrix();
		ofPopStyle();
	}
	glDisable(GL_DEPTH_FUNC);

	if (debugView)
		cam.end();
	else
		ofPopView();
    
	glDisable(GL_DEPTH_TEST);	

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if (key == 'd')
		debugView ^= true;

	if(key == ' '){
        scrubMode = !scrubMode;
        if(!scrubMode){
            startTime = ofGetElapsedTimeMillis();
        }
    }
    if(scrubMode && key == '['){
        long time = ofMap(mouseX, 0, ofGetWidth(), 0, sequences[currentTake].images.getDurationInMillis(), true);
   		sequences[currentTake].intime = time;

		playlist.pushTag("take",sequences[currentTake].xmlIndex);
        playlist.setValue("intime", int(time));
        playlist.popTag();
        playlist.saveFile();
        cout << "set in to " << time << endl;
    }
    if(scrubMode && key == ']'){
    	long time = ofMap(mouseX, 0, ofGetWidth(), 0, sequences[currentTake].images.getDurationInMillis(), true);
		sequences[currentTake].outtime = time;
		playlist.pushTag("take",sequences[currentTake].xmlIndex);
        playlist.setValue("outtime", int(time));
        playlist.popTag();
        playlist.saveFile();
        
        cout << "set out to " << time << endl;
    }
    
    if(scrubMode && key == OF_KEY_LEFT){
        nextTake();
    }
    if(scrubMode && key == OF_KEY_RIGHT){
        previousTake();
    }

    if(key == 'f'){
		ofToggleFullscreen();  
    }

	if(key == 'N'){
		nextTake();
	}
}

void testApp::nextTake(){
    currentTake = (currentTake + 1) % sequences.size();   
	cout << currentTake << " /  " << sequences.size() << endl;
	startTime = ofGetElapsedTimeMillis();
	ofxOscMessage m1;
	m1.setAddress("/person");
	m1.addStringArg(sequences[currentTake].name);

	ofxOscMessage m2;
	m2.setAddress("/person");
	m2.addStringArg(sequences[currentTake].name);
	cout << "sending " << sequences[currentTake].name << endl;
	sendera.sendMessage(m1);
	senderb.sendMessage(m2);

}

void testApp::previousTake(){
    currentTake--;
    if(currentTake < 0){
        currentTake = sequences.size()-1;
    }
   
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}