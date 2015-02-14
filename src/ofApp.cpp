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
    surfaceDepth = XML.getValue("settings:base-surface-depth", -20);
    
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
    cam.setDistance(2000);
    
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
        
        // If the key to dump a mesh .ply file has been pressed then we shouldn't add any more spectrum lines
        if(!bFinishMesh) {
            addNextSpectrumToMesh(period);
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
    mesh.draw();
//    mesh.drawWireframe();
//    mesh.drawVertices();

    if(bShowInfo) {
        ofSetColor(255,255,255);
        lightAbove.draw();
        lightBelow.draw();
    }
    
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
    int numVertices = mesh.getNumVertices();    // At least 256 * 2 (for the line added at the base as well)
    
    // Ensure there are at least two rows of vertices for the top surface - there will be one row of vertices inbetween each
    // surface line of vertices which added for the base
    // As a new spectrum band is generated stitch it into the existing mesh
    // Get the vertex index for the first vertex on each of the two lines the last one and the new one
    // and work along it adding two triangles for each two vertices
    
    if (numVertices >= (numSpectrumBands * 2)) {
        
        for(int j = 0; j < numSpectrumBands - 1; j++) {
            
            // Vertex indices
            // We want to stitch the last spectrum band added to the spectrum band for the surface two previous
            int i1 = numVertices - (numSpectrumBands * 2) + j;
            int i2 = numVertices - (numSpectrumBands * 2) + 1 + j;
            int i3 = numVertices - numSpectrumBands + j;
            int i4 = numVertices - numSpectrumBands + 1 + j;
            
            // Push the index of the first vertex in the previous line into an array
            if(j == 0) {
                
                // And the first time through add the first top vertex
                if(numVertices == (numSpectrumBands * 2)) {
                    innerVertexIndices.push_back(i1);
                }
                
                innerVertexIndices.push_back(i3);
            }
            
            // Push the end vertex of each line into an array
            if(j == numSpectrumBands - 2) {
                
                if(numVertices == (numSpectrumBands * 2)) {
                    outerVertexIndices.push_back(i2);
                }
                
                outerVertexIndices.push_back(i4);
            }
            
            mesh.addTriangle(i1, i2, i4);
            mesh.addTriangle(i4, i3, i1);
            
            // Recalculate the normals
            updateNormals(i1, i2, i3, i4, false);
            
        }
        
    }

}

//--------------------------------------------------------------
void ofApp::connectLastSpectrumToFirst() {
    
    int numVertices = mesh.getNumVertices();
    
    for (int i = 0; i < numSpectrumBands - 1; i++) {
        
        // Get each vertices on each line of the first and last spectrum lines
        int lastIdx1 = numVertices - numSpectrumBands + i;
        int lastIdx2 = numVertices - numSpectrumBands + 1 + i;
        
        int firstIdx1 = i;
        int firstIdx2 = i + 1;
        
        mesh.addTriangle(lastIdx1, lastIdx2, firstIdx2);
        mesh.addTriangle(firstIdx2, firstIdx1, lastIdx1);
        
        updateNormals(lastIdx1, lastIdx2, firstIdx1, firstIdx2, false);
        
    }
    
}

//--------------------------------------------------------------
void ofApp::addCentralCylinder() {
    
    // Get the last vertex in the array - used later to know where to attach the last spectrum line back into the first
    ofIndexType lastVertex = mesh.getNumVertices();
    
    float largestZ = 0;
    
    // An array to push all of the vertices that we're adding here on the top rim
    // This will be passed into a separate function to add the flat top surface
    vector<ofIndexType> topRimVertices;
    
    // Find the tallest point and use that as the z value for the upper circle rim
    for (int i = 0; i < innerVertexIndices.size(); i++) {
        
        ofVec3f v1 = mesh.getVertex(innerVertexIndices[i]);
        if(v1.z > largestZ) {
            largestZ = v1.z;
        }
        
    }
    
    // Add a vertex above each of the inner vertices
    for (int i = 0; i < innerVertexIndices.size(); i++) {
        ofVec3f v1 = mesh.getVertex(innerVertexIndices[i]);
        
        // Create a new point and add it to the mesh
        ofVec3f p(v1.x, v1.y, largestZ);
        mesh.addVertex(p);
        mesh.addColor(ofColor::seaGreen);
        
        // Add a normal for each added vertex - we'll figure out the correct normal vector later
        ofVec3f n(0, 0, 1);
        mesh.addNormal(n);
        
        // Cache the mesh index of point just added - this will be used to construct the top surface
        topRimVertices.push_back(mesh.getNumVertices() - 1);
        
        if (i > 0) {
            // Add two triangles between the vertex just added + the previous just added vertex (index will be this one -1)
            // with the inner ring vertices
            int currSpectrumIdx = innerVertexIndices[i];
            int prevSpectrumIdx = innerVertexIndices[i - 1];
            
            int currTopRingIdx = topRimVertices[i];
            int prevTopRingIdx = topRimVertices[i - 1];
            
            mesh.addTriangle(currSpectrumIdx, prevSpectrumIdx, prevTopRingIdx);
            mesh.addTriangle(prevTopRingIdx, currTopRingIdx, currSpectrumIdx);

            
            // Maybe the triangle winding is incorrect - the normals need to be inverted
            updateNormals(currSpectrumIdx, prevSpectrumIdx, currTopRingIdx, prevTopRingIdx, true);
            
        }
    }
    
    // Add the inner wall triangles between the first and last upper vertices and the first and last spectrum vertices
    ofIndexType topRimFirstVertex = topRimVertices[0];
    ofIndexType topRimLastVertex = topRimVertices.back();
    
    ofIndexType spectrumFirstVertex = 0;
    ofIndexType spectrumLastVertex = lastVertex - numSpectrumBands;
    
    mesh.addTriangle(topRimFirstVertex, topRimLastVertex, spectrumLastVertex);
    mesh.addTriangle(spectrumLastVertex, spectrumFirstVertex, topRimFirstVertex);
    
    updateNormals(topRimFirstVertex, topRimLastVertex, spectrumFirstVertex, spectrumLastVertex, false);
    
    // Add the top plane to the cylinder in the centre
    addMeshCap(topRimVertices, largestZ);
    
}

//--------------------------------------------------------------
void ofApp::addMeshCap(vector<ofIndexType> vertices, float height) {
    
    // This will add the triangles for the top and bottom surfaces of the mesh
    // Add the centre vertex of the top surface
    // Create a new point and add it to the mesh
    ofVec3f p(0, 0, height);
    mesh.addVertex(p);
    mesh.addColor(ofColor::seaGreen);
    
    ofVec3f n(0, 0, 0);
    // Add a normal the centre top or bottom
    if (height > 0) {
        n.z = 1;  // A normal facing up for the top surface
    } else {
        n.z = -1; // A normal facing down for the bottom surface
    }
    
    mesh.addNormal(n);
    
    // Get the vertex index of the point just added
    int centreVertexIndex = mesh.getNumVertices() - 1;
    
    // Loop over the arrays of top vertices and connect them all to each other and the central vertex
    
    for (int i = 0; i < vertices.size(); i++) {
        
        if(i < vertices.size() - 1) {
            mesh.addTriangle(vertices[i + 1], vertices[i], centreVertexIndex);
        } else {
            mesh.addTriangle(vertices[0], vertices[i], centreVertexIndex);
        }
        
        mesh.addColor(ofColor::seaGreen);
        
    }
    
}

//--------------------------------------------------------------
void ofApp::addSideToMesh() {
    
    // An array to cache the indices of the vertices that we're adding.
    // This will be passed into another function for adding the triangles at the base
    vector<ofIndexType> lowerRimVertices;
    
    // Subract the vertices added when tying up the inner vertices so that we get the last vertex of the surface lines
    ofIndexType lastVertex = mesh.getNumVertices() - innerVertexIndices.size() - 2; // Minus two here because another vertex was added for the top centre
    
    // Add an outer rim of triangles
    for (int i = 0; i < outerVertexIndices.size(); i++) {
        ofVec3f v1 = mesh.getVertex(outerVertexIndices[i]);
        
        // Create a new point and add it to the mesh
        // surfaceDepth is read from the configuration file
        ofVec3f p(v1.x, v1.y, surfaceDepth);
        mesh.addVertex(p);
        mesh.addColor(ofColor::seaGreen);
        
        // Add a normal for each added vertex - we'll figure out the correct normal vector later
        ofVec3f n(0, 0, 1);
        mesh.addNormal(n);
        
        // Cache the mesh index of point just added - this will be used to construct the top surface
        lowerRimVertices.push_back(mesh.getNumVertices() - 1);
        
        if (i > 0) {
            // Add two triangles between the vertex just added + the previous just added vertex (index will be this one -1)
            // with the inner ring vertices
            int currOuterEdgeVertex = outerVertexIndices[i];
            int prevOuterEdgeVertex = outerVertexIndices[i - 1];
            
            int currLowerRimVertex = lowerRimVertices[i];
            int prevLowerRimVertex = lowerRimVertices[i - 1];
            
            mesh.addTriangle(currOuterEdgeVertex, prevOuterEdgeVertex, prevLowerRimVertex);
            mesh.addTriangle(prevLowerRimVertex, currLowerRimVertex, currOuterEdgeVertex);
            
            
            // Maybe the triangle winding is incorrect - the normals need to be inverted
            updateNormals(currOuterEdgeVertex, prevOuterEdgeVertex, currLowerRimVertex, prevLowerRimVertex, false);
            
        }
    }
    
    // Join up the last with the first vertices
    ofIndexType lowerRimFirstVertex = lowerRimVertices[0];
    ofIndexType lowerRimLastVertex = lowerRimVertices.back();
    
    ofIndexType spectrumFirstVertex = numSpectrumBands - 1;
    ofIndexType spectrumLastVertex = lastVertex;
    
    mesh.addTriangle(lowerRimFirstVertex, lowerRimLastVertex, spectrumLastVertex);
    mesh.addTriangle(spectrumLastVertex, spectrumFirstVertex, lowerRimFirstVertex);
    
    updateNormals(lowerRimFirstVertex, lowerRimLastVertex, spectrumFirstVertex, spectrumLastVertex, true);
    
    // Add the top plane to the cylinder in the centre
    addMeshCap(lowerRimVertices, 0.0);
}

//--------------------------------------------------------------
void ofApp::updateNormals(ofIndexType i1, ofIndexType i2, ofIndexType i3, ofIndexType i4, bool invert) {
    
    // Four the vertices of two adjacent triangles, get the cross product and reset the corresponding normal for each vertex
    // This is probably too reliant on the order that the vertex indices are passed into the function
    
    int direction = 1;
    
    if(invert) {
        direction = -1;
    }
    
    // Get the vertices to calculate the normals
    ofVec3f v1 = mesh.getVertex(i1);
    ofVec3f v2 = mesh.getVertex(i2);
    ofVec3f v3 = mesh.getVertex(i3);
    ofVec3f v4 = mesh.getVertex(i4);
    
    // Face normal for the first triangle
    ofVec3f nTri1 = ( (v2 - v1).crossed( v4 - v1 ) ).normalized() * direction;
    
    // Face normal for the second triangle
    ofVec3f nTri2 = ( (v3 - v4).crossed( v1 - v4 ) ).normalized() * direction;
    
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
            
            // Call the functions to finish off the mesh.
            // The order matters, adding the central cylinder and edges adds more vertices to the mesh,
            // connecting the last to the first lines is reliant on the order of the vertices as the were put into the mesh
            connectLastSpectrumToFirst();
            addCentralCylinder();
            addSideToMesh();
            
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
