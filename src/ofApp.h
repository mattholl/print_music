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
    
    
    // ---
    // Runtime info
    bool bShowInfo = true;          // Draw the spectrum and reportstream data or not
    
    //    int bandRad = 2;                // Band index spectrum bass drum
    //    int bandVel = 100;              // Band index in spectrum snare
    
    // ---
    // Mesh setup and rendering
    ofEasyCam cam;
    ofMesh mesh;
    ofLight light;
    
    
    float currentAngle = 0;         // Accumlated rotation for the current line
    float time0;          // Regularly add a line to the mesh
    
    void addNextSpectrumToMesh(float period);
    void addBaseToMesh(float period);
};
