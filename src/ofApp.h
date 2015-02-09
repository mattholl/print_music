#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
    // ---
    // Audio player
    ofSoundPlayer sound;
    float volume = 1.0;                   // The output volume +/- space to stop music
    
    ofxXmlSettings XML;             // Load the settings from bin/data/settings.xml
    string fileName;                // Global so it can be ouput with the info
    int fileLength;                 // In seconds
    float decayRate;                // The rate at which the spectrum peaks fall
    float frequencyScale;           // Used to increase the height of the peaks if required
    float radPostStart;             // Distance from the centre that the radial line will start
    float radPosEnd;                // Furthest radial point
    float lineResolution;             // Lines per second
    
    
    int numSpectrumBands;           // Number of bands in spectrum
    vector<float> spectrum;         // Smoothed spectrum values
    vector<int> innerVertexIndices; // Keep an array of all of the first and last vertices in each line
    vector<int> outerVertexIndices;
    
    // ---
    // Runtime info
    bool bShowInfo = true;          // Draw the spectrum and reportstream data or not
    
    bool bFinishMesh = false;       // Set this tie the last FFT spectrum band into the first,
                                    // when set to true no more bands will be added to the mesh
    
    int surfaceDepth;               // How far below the surface FFT will the base be placed
    
    //    int bandRad = 2;                // Band index spectrum bass drum
    //    int bandVel = 100;              // Band index in spectrum snare
    
    // ---
    // Mesh setup and rendering
    ofEasyCam cam;
    ofMesh mesh;
    ofLight lightAbove;
    ofLight lightBelow;
    
    float currentAngle = 0;         // Accumlated rotation for the current line
    float time0;          // Regularly add a line to the mesh
    
    // Function which will add vertices and triangles to the mesh
    void addNextSpectrumToMesh(float period);
    void addBaseToMesh(float period);
    void addCentreToMesh(float period);
    void addSideToMesh(float period);
    void connectLastSpectrumToFirst();
};
