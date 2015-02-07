//--------------------------------------------------------------
// TEST PRESSING
//--------------------------------------------------------------

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetVerticalSync(true);
    ofEnableLighting();
    ofEnableAlphaBlending();
    ofEnableSmoothing();
    ofSetFrameRate(60);
    
    // Load the settings file for the spring parameters
    if( XML.loadFile("settings.local.xml") ){
        cout << "settings.xml loaded" << endl;
    } else {
        cout << "unable to load settings.local.xml check data/ folder" << endl;
    }
    
    string fileIndex = "a";
    fileName = XML.getValue("file-index:" + fileIndex + ":name", "none");
    fileLength = XML.getValue("file-index:" + fileIndex + ":length", 60);
    
    ofLog() << fileName;
    ofLog() << fileLength;
    
    decayRate = XML.getValue("settings:decay-rate", 0.97);
    frequencyScale = XML.getValue("settings:frequency-scale", 100);
    radPostStart = XML.getValue("settings:radial-position-start", 10);
    radPosEnd = XML.getValue("settings:radial-position-end", 1000);
    lineResolution = XML.getValue("settings:line-resolution", 1);
    
    // Set up sound sample
    sound.loadSound(fileName);
    sound.setLoop(true);
    sound.play();
    sound.setVolume(1.0);
    
    numSpectrumBands = 256;              // Number of bands in spectrum 256
    spectrum.resize(numSpectrumBands);
    
    // Set spectrum values to 0
    for (int i = 0; i < numSpectrumBands; i++) {
        spectrum[i] = 0.0f;
    }
    
    cam.setFarClip(20000);
    cam.setDistance(500);
    
    lightAbove.enable();
    lightAbove.setPointLight();
    lightAbove.setPosition(-50, -50, 300);
    
    lightBelow.enable();
    lightBelow.setPointLight();
    lightBelow.setPosition(1500, 200, 0);
    
    time0 = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::update(){
    // Update sound engine
    ofSoundUpdate();
    
    // Get current spectrum with N bands
    float *val = ofSoundGetSpectrum(numSpectrumBands);
    // Don't release memory of val because it is manged by sound engine
    
    // Update smoothed spectrum by slowly decreasing its values and getting max with val
    // so that there are slowly falling peaks with the spectrum
    for (int i = 0; i < numSpectrumBands; i++) {
        spectrum[i] *= decayRate;
        spectrum[i] = max(spectrum[i], val[i]);
    }
    
    // Is this stable?
    // Add a line to the mesh for the current spectrum values
    //    int callsPerSecond = 10;
    //    float period = 1.0 / (float)callsPerSecond;
    
    //lineResolution = lines per second
    
    float period = 1 / lineResolution; // Add a line every period seconds
    
    float time = ofGetElapsedTimef();
    float dt = ofClamp(time - time0, 0, 1);
    
    // Add a line to the mesh ten times a second
    if (dt >= period) {
        // how much must the angle be increased by to complete one full rotation in fileLength time
        float angleVelocity = TWO_PI / (float)fileLength * period;  // rads per second
        currentAngle += angleVelocity;  // Update global value
        
        if(!bFinishMesh) {
            addNextSpectrumToMesh(period);
            addBaseToMesh(period);
            addCentreToMesh(period);
            addSideToMesh(period);
        }
        
        time0 = time;
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(230, 230, 230);
    
    // Draw the mesh
    cam.begin();
    ofEnableDepthTest();
//    mesh.draw();
    mesh.drawWireframe();
    ofSetColor(255,255,255);
    lightAbove.draw();
    lightBelow.draw();
    
    ofDisableDepthTest();
    cam.end();
    
    // Draw GUI over the top
    if(bShowInfo) {
        // Draw spectrum
        ofSetColor(0, 0, 0);
        for (int i = 0; i < numSpectrumBands; i++) {
            // Draw spectrum graph
            ofRect(10 * i + 5, 600, 3, -spectrum[i] * 100);
        }
        
        // Output the info string
        // Current volume
        stringstream reportStream;
        reportStream << "filename: " << fileName << endl;
        reportStream << "filetime (s): " << fileLength << endl;
        reportStream << "set volume: " << volume << " (press: + -)" << endl;
        reportStream << "elapsed time: " << sound.getPosition() << endl;
        reportStream << "(dump mesh: 'm', dump image: 's', toggle volume: spacebar)" << endl;
        reportStream << "(hide info: 'h')" << endl;
        reportStream << "mesh vertices: " << mesh.getNumVertices() << endl;
        reportStream << "mesh triangles: " << mesh.getNumIndices() / 3 << endl;
        
        ofDrawBitmapString(reportStream.str(), 20, 622);
    }
}

//--------------------------------------------------------------
void ofApp::addNextSpectrumToMesh(float period) {
    
    // Where does the radius start and end
    float radialPosStart = radPostStart; // Values for these are imported from settings.xml file
    float radialPosEnd = radPosEnd;
    float radialPos = radialPosStart; // current radius between start and end
    
    float pctStep = 1 / (float)(numSpectrumBands);  // percentage interpolated between start and end radial pos
    float pct = 0.0f;
    
    //    float currentPct = 0;
    
    // Add the vertices for the new line
    for (int i = 0; i < numSpectrumBands; i++) {
        
        pct = pctStep * i;
        radialPos = (1 - pct) * radialPosStart + (pct) * radialPosEnd;
        
        // set point x and y from currentAngle rotation
        float x = cos(currentAngle) * radialPos;
        float y = sin(currentAngle) * radialPos;
        float z = spectrum[i] * frequencyScale;
        
        // Add each point to the mesh
        ofVec3f p(x, y, z);
        mesh.addVertex(p);
        mesh.addColor(ofColor::seaGreen);
        
        // Add a normal for each added vertex
        ofVec3f n(0, 0, 1);
        mesh.addNormal(n);
    }
    
    // Weave the new vertices into the existing mesh
    int numVertices = mesh.getNumVertices();    // At least 256 * 3 (for the line added at the base as well)
    
    // Ensure there are at least two rows of vertices for the top surface - there will be one row of vertices inbetween each
    // surface line of vertices which added for the base
    // As a new spectrum band is generated stitch it into the existing mesh
    // Get the vertex index for the first vertex on each of the two lines the last one and the new one
    // and work along it adding two triangles for each two vertices
    
    if (numVertices >= (numSpectrumBands * 3)) {
        
        for(int j = 0; j < numSpectrumBands - 1; j++) {
            
            // Vertex indices
            // We want to stitch the last spectrum band added to the spectrum band for the surface two previous
            int i1 = numVertices - (numSpectrumBands * 3) + j;
            int i2 = numVertices - (numSpectrumBands * 3) + 1 + j;
            int i3 = numVertices - numSpectrumBands + j;
            int i4 = numVertices - numSpectrumBands + 1 + j;
            
            mesh.addTriangle(i1, i2, i4);
            mesh.addTriangle(i4, i3, i1);
            
            // Get the vertices to calulate the normals
            ofVec3f v1 = mesh.getVertex(i1);
            ofVec3f v2 = mesh.getVertex(i2);
            ofVec3f v3 = mesh.getVertex(i3);
            ofVec3f v4 = mesh.getVertex(i4);
            
            // Face normal for the first triangle
            ofVec3f nTri1 = ( (v2 - v1).crossed( v4 - v1 ) ).normalized();
            
            // Face normal for the second triangle
            ofVec3f nTri2 = ( (v3 - v4).crossed( v1 - v4 ) ).normalized();
            
            // Get the corresponding normals for i1-4, accumulate, normalise and reset in the mesh
            ofVec3f n1 = mesh.getNormal(i1);
            ofVec3f n2 = mesh.getNormal(i2);
            ofVec3f n3 = mesh.getNormal(i3);
            ofVec3f n4 = mesh.getNormal(i4);
            
            ofVec3f newN1 = n1 + nTri1 + nTri2;
            newN1.normalize();
            mesh.setNormal(i1, newN1);
            
            ofVec3f newN2 = n2 + nTri2;
            newN2.normalize();
            mesh.setNormal(i2, newN2);
            
            ofVec3f newN3 = n3 + nTri2;
            newN3.normalize();
            mesh.setNormal(i3, newN3);
            
            ofVec3f newN4 = n4 + nTri1 + nTri2;
            newN4.normalize();
            mesh.setNormal(i4, newN4);
            
        }
        
    }

}

//--------------------------------------------------------------
void ofApp::connectLastSpectrumToFirst() {
    // Get the total number of vertices
    // numSpectrumBands

    int numVertices = mesh.getNumVertices();
    
    
    for (int i = 0; i < numSpectrumBands - 1; i++) {
        // Get the indices of each vertex, starting from the centre
        int lastIdx1 = numVertices - numSpectrumBands + i;
        int lastIdx2 = numVertices - numSpectrumBands + 1 + i;

        int firstIdx1 = i;
        int firstIdx2 = i + 1;
        
        mesh.addTriangle(lastIdx1, lastIdx2, firstIdx2);
        mesh.addTriangle(firstIdx2, firstIdx1, lastIdx1);
        
    }
    
    // Subtract num vertices per spectrum to get the first one
    // 0 to num vertices per spectrum is the first line
    // work down the line and add triangles
    
    // connect the rim vertices
    // connect the inner vertices
}

//--------------------------------------------------------------
void ofApp::addBaseToMesh(float period) {
    
    // Where does the radius start and end
    float radialPosStart = radPostStart; // Values for these are imported from settings.xml file
    float radialPosEnd = radPosEnd;
    float radialPos = radialPosStart; // current radius between start and end
    
    float pctStep = 1 / (float)(numSpectrumBands);  // percentage interpolated between start and end radial pos
    float pct = 0.0f;
    
    // Add vertices for the base
    // Follow the same process of zig-zag stitching between the two line but with height 0
    for (int i = 0; i < numSpectrumBands; i++) {
        
        pct = pctStep * i;
        radialPos = (1 - pct) * radialPosStart + (pct) * radialPosEnd;
        
        // set point x and y from currentAngle rotation
        float x = cos(currentAngle) * radialPos;
        float y = sin(currentAngle) * radialPos;
        float z = -5;    //
        
        // Add each point to the mesh
        ofVec3f p(x, y, z);
        mesh.addVertex(p);
        mesh.addColor(ofColor::seaGreen);
        
        // Add a normal for each added vertex
        ofVec3f n(0, 0, -1);
        mesh.addNormal(n);
    }
    
    // Add the triangles ensure to take the last chunk of vertices and don't include the surface ones
    
    // Get the last batch of vertices that were added to the mesh
    // Weave the new vertices into the existing mesh
    // This will now be at least 512 * 4, another line has been added for the base of the mesh and will be two previous lines for the base
    int numVertices = mesh.getNumVertices();
    
    // Need to start at total vertices - 2 * num spectrum bands and work through to the end
    
    if (numVertices >= (numSpectrumBands * 4)) {

        for(int j = 0; j < numSpectrumBands - 1; j++) {

            // Vertex indices
            int i1 = numVertices - (numSpectrumBands * 3) + j;
            int i2 = numVertices - (numSpectrumBands * 3) + 1 + j;
            int i3 = numVertices - numSpectrumBands + j;
            int i4 = numVertices - numSpectrumBands + 1 + j;

            mesh.addTriangle(i1, i2, i4);
            mesh.addTriangle(i4, i3, i1);
            
            // The normals that are added for the base all point down (z = -1) so there's no need to
            // add and normalize them for each triangle
            
        }
        
    }
    
}


//--------------------------------------------------------------
void ofApp::addCentreToMesh(float period) {
    // For each line get the base vertex and the
    // Follow the same logic as adding the side triangles to the mesh

    int numVertices = mesh.getNumVertices();
    
    if (numVertices >= (numSpectrumBands * 4)) {
        
        // Get first vertex index in the last line
        int i1 = numVertices - numSpectrumBands;
        
        // Get the vertex of the next to last line of the base
        int i2 = numVertices - (numSpectrumBands * 2);
        
        // Get the end vertex index of the last spectrum line
        int i3 = numVertices - (numSpectrumBands * 3);
        
        // And the last vertex index of the spectrum line before the last one
        int i4 = numVertices - (numSpectrumBands * 4);
        
        // Add triangles to the mesh
        mesh.addTriangle(i1, i2, i4);
        mesh.addTriangle(i4, i3, i1);
        
        // Get the vertices to calulate the normals
        ofVec3f v1 = mesh.getVertex(i1);
        ofVec3f v2 = mesh.getVertex(i2);
        ofVec3f v3 = mesh.getVertex(i3);
        ofVec3f v4 = mesh.getVertex(i4);
        
        // Face normal for the first triangle
        ofVec3f nTri1 = ( (v2 - v1).crossed( v4 - v1 ) ).normalized();
        
        // Face normal for the second triangle
        ofVec3f nTri2 = ( (v3 - v4).crossed( v1 - v4 ) ).normalized();
        
        // Get the corresponding normals for i1-4, accumulate, normalise and reset in the mesh
        ofVec3f n1 = mesh.getNormal(i1);
        ofVec3f n2 = mesh.getNormal(i2);
        ofVec3f n3 = mesh.getNormal(i3);
        ofVec3f n4 = mesh.getNormal(i4);
        
        ofVec3f newN1 = n1 + nTri1 + nTri2;
        newN1.normalize();
        mesh.setNormal(i1, newN1);
        
        ofVec3f newN2 = n2 + nTri2;
        newN2.normalize();
        mesh.setNormal(i2, newN2);
        
        ofVec3f newN3 = n3 + nTri2;
        newN3.normalize();
        mesh.setNormal(i3, newN3);
        
        ofVec3f newN4 = n4 + nTri1 + nTri2;
        newN4.normalize();
        mesh.setNormal(i4, newN4);
        
    }
    
}



//--------------------------------------------------------------
void ofApp::addSideToMesh(float period) {
    // Called after the vertices for the spectrum surface and the vertices for the base have been added.
    // Get the end vertices for each line - again the line just added (total num vertices less num vertices in each spectrum band)
    // And the end vertices for lines that make up the base
    // Normals should point away from the centre
    
    int numVertices = mesh.getNumVertices();
    
    if (numVertices >= (numSpectrumBands * 4)) {
        
        // Get the end vertex of the last line of the base
        int i1 = numVertices - 1;

        // Get the vertex of the next to last line of the base
        int i2 = numVertices - 1 - (numSpectrumBands * 2);
        
        // Get the end vertex index of the last spectrum line
        int i3 = numVertices - 1 - numSpectrumBands;
        
        // And the last vertex index of the spectrum line before the last one
        int i4 = numVertices - 1 - (numSpectrumBands * 3);
        
        // Add triangles to the mesh
        mesh.addTriangle(i1, i2, i4);
        mesh.addTriangle(i4, i3, i1);
        
        // Get the vertices to calulate the normals
        ofVec3f v1 = mesh.getVertex(i1);
        ofVec3f v2 = mesh.getVertex(i2);
        ofVec3f v3 = mesh.getVertex(i3);
        ofVec3f v4 = mesh.getVertex(i4);
        
        // Face normal for the first triangle
        ofVec3f nTri1 = ( (v2 - v1).crossed( v4 - v1 ) ).normalized() * -1;
        
        // Face normal for the second triangle
        ofVec3f nTri2 = ( (v3 - v4).crossed( v1 - v4 ) ).normalized() * -1;
        
        // Get the corresponding normals for i1-4, accumulate, normalise and reset in the mesh
        ofVec3f n1 = mesh.getNormal(i1);
        ofVec3f n2 = mesh.getNormal(i2);
        ofVec3f n3 = mesh.getNormal(i3);
        ofVec3f n4 = mesh.getNormal(i4);
        
        ofVec3f newN1 = n1 + nTri1 + nTri2;
        newN1.normalize();
        mesh.setNormal(i1, newN1);
        
        ofVec3f newN2 = n2 + nTri2;
        newN2.normalize();
        mesh.setNormal(i2, newN2);
        
        ofVec3f newN3 = n3 + nTri2;
        newN3.normalize();
        mesh.setNormal(i3, newN3);
        
        ofVec3f newN4 = n4 + nTri1 + nTri2;
        newN4.normalize();
        mesh.setNormal(i4, newN4);
    
    }
    
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
    switch (key) {
            
        // Save an image
        case 's': {
            ofImage image;
            
            image.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
            
            // Save the image to the dir from XML settings, path should end in /
            image.saveImage("screengrab_" + ofToString(ofGetUnixTime()) + ".png");
            cout << "Manual image saved : screengrab_" + ofToString(ofGetUnixTime()) + ".png" << endl;
            
            break;
        }
            
            // Toggle the spectrum
        case 'h': {
            bShowInfo = !bShowInfo;
            break;
        }
            
            // Export a PLY mesh
        case 'm': {
            bFinishMesh = true;
            
            // Call functions to connect the last line into the first as well as the rim and inner inners
            
            connectLastSpectrumToFirst();
            mesh.save("meshdump_" + ofToString(ofGetUnixTime()) + ".ply");
            cout << "Manual mesh dump : meshdump_" + ofToString(ofGetUnixTime()) + ".ply" << endl;
            break;
        }
            
            // Set volume
        case '+':
        case '=':
            volume += 0.1;
            if (volume > 1) volume = 1;
            sound.setVolume(volume);
            break;
            
        case '-':
            volume -= 0.1;
            if (volume < 0) volume = 0;
            sound.setVolume(volume);
            break;
            
            // Toggle sound
        case ' ':
            if(sound.getVolume() > 0) {
                sound.setVolume(0);
            } else {
                sound.setVolume(volume);
            }
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
