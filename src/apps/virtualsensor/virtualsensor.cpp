/*
 ** Copyright (C) 2000 George Tzanetakis <gtzan@cs.princeton.edu>
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 ** 
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** 
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software 
 ** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/** 
record: record a clip using AudioSource
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "Collection.h"
#include "MarSystemManager.h"
#include "Accumulator.h"
#include "MidiInput.h"
#include "Esitar.h"
#include "MidiOutput.h"
#include "CommandLineOptions.h"
#include "mididevices.h"


#include <string> 
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;
using namespace Marsyas;

#define DEVIBOT_DHA 3
#define DEVIBOT_TU 2
#define DEVIBOT_GE 1
#define DEVIBOT_NA 0

int helpopt;
int usageopt;
int trainopt;
mrs_natural instrumentopt;
mrs_real lengthopt;
mrs_real sropt;
mrs_natural copt;

CommandLineOptions cmd_options;



void printUsage(string progName)
{
    MRSDIAG("record.cpp - printUsage");
    cerr << "Usage : " << progName << " [-l length(seconds)] [-h help] [-u usage] soundfile1 soundfile2 soundfile3" << endl;
    cerr << endl;
    exit(1);
}

void printHelp(string progName)
{
    MRSDIAG("virtualsensor.cpp - printHelp");
    cerr << "virtualsensor, MARSYAS, Copyright George Tzanetakis " << endl;
    cerr << "--------------------------------------------" << endl;
    cerr << "Prints information about the sound files provided as arguments " << endl;
    cerr << endl;
    cerr << "Usage : " << progName << "soundfile1 soundfile2 soundfile3" << endl;
    cerr << endl;
    cerr << "where file1, ..., fileN are sound files in a Marsyas supported format" << endl;
    cerr << "Help Options:" << endl;
    cerr << "-u --usage      : display short usage info" << endl;
    cerr << "-h --help       : display this information " << endl;
    cerr << "-l --length     : record length in seconds " << endl;
    cerr << "-s --srate      : samping rate " << endl;
    cerr << "-c --channels   : number of channels to record " << endl;
    cerr << "-i --instrument : 0: drum or 1: Read Sitar 2: Record Sitar Sensors 3: Read Fret to Pitch 4: read collection of files for READ " << endl;
    cerr << endl;
    exit(1);
}




void initOptions()
{
    cmd_options.addBoolOption("help", "h", false);
    cmd_options.addBoolOption("trainopt", "t", false);
    cmd_options.addBoolOption("usage", "u", false);
    cmd_options.addBoolOption("verbose", "v", false);
    cmd_options.addRealOption("length", "l", 48.0);
    cmd_options.addRealOption("srate", "s", 44100.0);
    cmd_options.addNaturalOption("channels", "c", 1);
    cmd_options.addNaturalOption("instrument", "i", 2);
}


void loadOptions()
{
    helpopt = cmd_options.getBoolOption("help");
    usageopt = cmd_options.getBoolOption("usage");
    lengthopt = cmd_options.getRealOption("length");
    sropt = cmd_options.getRealOption("srate"); 
    copt = cmd_options.getNaturalOption("channels");
    trainopt = cmd_options.getBoolOption("trainopt");
    instrumentopt = cmd_options.getNaturalOption("instrument");
}



void readCollection(Collection& l, string name)
{
    MRSDIAG("sfplay.cpp - readCollection");
    ifstream from1(name.c_str());
    mrs_natural attempts  =0;


    MRSDIAG("Trying current working directory: " + name);
    if (from1.good() == false)
    {
        attempts++;
    }
    else
    {
        from1 >> l;
        l.setName(name.substr(0, name.rfind(".", name.length())));
    }


    if (attempts == 1) 
    {
        string warn;
        warn += "Problem reading collection ";
        warn += name; 
        warn += " - tried both default mf directory and current working directory";
        MRSWARN(warn);
        exit(1);

    }
} 


void recordVirtualSensor(mrs_real length) 
{
    MarSystemManager mng;

    MarSystem* pnet = mng.create("Series", "pnet");

    MarSystem* srm = mng.create("SilenceRemove", "src"); 

    MarSystem* recordNet = mng.create("Series", "recordNet");
    MarSystem* asrc = mng.create("AudioSource", "asrc");
    MarSystem* dest = mng.create("SoundFileSink", "dest");

    
    MidiInput* midiin = new MidiInput("midiin");

    recordNet->addMarSystem(asrc);
    recordNet->addMarSystem(midiin);
    srm->addMarSystem(recordNet);

    // Make Virtual Midin Port!
    recordNet->updctrl("MidiInput/midiin/mrs_bool/virtualPort", true);
    
    // recordNet->addMarSystem(dest); 
    // OSX tends to like 44100 sampling rate so we make it happy
    recordNet->updctrl("mrs_real/israte", 44100.0); 
    recordNet->updctrl("mrs_real/osrate", 44100.0); 
    recordNet->linkctrl("mrs_bool/notEmpty", "AudioSource/asrc/mrs_bool/notEmpty");
    // this buffer size is needed for the Tascam FW 1804
    //recordNet->updctrl("AudioSource/asrc/mrs_natural/bufferSize",6144);
    recordNet->updctrl("AudioSource/asrc/mrs_bool/initAudio", true);

    pnet->addMarSystem(srm);
    pnet->addMarSystem(dest);

    // pnet->addMarSystem(mng.create("PlotSink", "psink"));
    pnet->addMarSystem(mng.create("Windowing", "ham"));
    pnet->addMarSystem(mng.create("Spectrum", "spk"));
    pnet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
    MarSystem* features = mng.create("Fanout", "features");
    features->addMarSystem(mng.create("Centroid", "cntrd"));
    features->addMarSystem(mng.create("Rolloff", "rolloff"));
    features->addMarSystem(mng.create("MFCC", "mfcc"));
    pnet->addMarSystem(features);

    pnet->addMarSystem(mng.create("Annotator", "ann"));
    pnet->addMarSystem(mng.create("WekaSink", "wsink"));

    pnet->updctrl("WekaSink/wsink/mrs_natural/nLabels", 2);
    pnet->updctrl("WekaSink/wsink/mrs_string/labelNames", "edge, middle");
    pnet->updctrl("WekaSink/wsink/mrs_string/filename", "vsensor.arff");

    pnet->updctrl("mrs_real/israte", 44100.0); 
    //pnet->updctrl("mrs_real/osrate", 44100.0); 
    // output of all hits concatenated
    pnet->updctrl("SoundFileSink/dest/mrs_string/filename", "vsens.au");   

    mrs_real srate = recordNet->getctrl("AudioSource/asrc/mrs_real/israte")->to<mrs_real>();
    mrs_natural inSamples = recordNet->getctrl("AudioSource/asrc/mrs_natural/inSamples")->to<mrs_natural>();
    mrs_natural iterations = (mrs_natural)((srate * length) / inSamples);

    int r;

    cout << *recordNet << endl; 
    for (mrs_natural t = 0; t < iterations; t++)
      {
	r = 0; // quick hack - needs to be fixed 
	// r = midiin->rval;
	cout << "rval: " << r << endl;
	cout << t << " of "<< iterations << endl; 
	if (r> 61)
	  {
	    cout << "middle" << endl;
	    pnet->setctrl("Annotator/ann/mrs_natural/label", 1);
	  }
	/* else if (r > 61) 
	   {
	   cout << "middle" << endl;
	   pnet->setctrl("Annotator/ann/mrs_natural/label", 1);
	   }
	*/ 
	else
	  {
	    pnet->setctrl("Annotator/ann/mrs_natural/label", 0);
	    cout << "edge" << endl;
	  }
	
	pnet->tick();
      }
    
    /*
    // uncomment this block if you would like to see the midi message during a hit
    for (i=0; i < nBytes; i++) 
        std::cout << "Byte " << i << " = " << (int) message[i] << ", "; 
    std::cout << endl;
    */
}

void testrmspreprocess() {
    // -i 8

    mrs_natural windowsize = 512;
    mrs_string sfname = "input.wav";
   
    MarSystemManager mng;
    MarSystem* playbacknet = mng.create("Series", "playbacknet");
    playbacknet->addMarSystem(mng.create("SoundFileSource", "src"));
    //playbacknet->addMarSystem(mng.create("Rms", "rms"));
    playbacknet->addMarSystem(mng.create("SoundFileSink", "dest"));
    //playbacknet->addMarSystem(mng.create("PeakerAdaptive", "peaker"));
    playbacknet->addMarSystem(mng.create("Peaker", "peaker"));


    playbacknet->updctrl("mrs_natural/inSamples", windowsize);
    playbacknet->updctrl("mrs_real/israte", 44100.0);
    playbacknet->updctrl("mrs_real/osrate", 44100.0);
    playbacknet->updctrl("SoundFileSource/src/mrs_string/filename", sfname);
    playbacknet->updctrl("SoundFileSink/dest/mrs_string/filename", "rmsOutput.wav");
    int srate = playbacknet->getctrl("mrs_natural/inSamples")->to<mrs_natural>();

    /*
    // values optimized for window size of 512
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/inSamples", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakSpacing", 4.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakStrength", 0.7);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStart", 0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakEnd", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakGain", 1.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStrengthReset", 2);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakDecay", 0.999);
    */

    playbacknet->updctrl("Peaker/peaker/mrs_real/peakStrength", 0.7);
    playbacknet->updctrl("Peaker/peaker/mrs_real/peakSpacing", 0.3);

    while(playbacknet->getctrl("SoundFileSource/src/mrs_bool/notEmpty")->to<mrs_bool>())
        playbacknet->tick();
    //cout << *playbacknet << endl;

    delete playbacknet;
}


void extractHits() {
    // -i 6

    MarSystemManager mng;

    mrs_natural windowsize = 8;
    mrs_string sfname = "input.wav";
   
    MarSystem* playbacknet = mng.create("Series", "playbacknet");
    playbacknet->addMarSystem(mng.create("SoundFileSource", "src"));
    //playbacknet->addMarSystem(mng.create("PeakerAdaptive", "peaker"));
    playbacknet->addMarSystem(mng.create("Peaker", "peaker"));
    playbacknet->addMarSystem(mng.create("SoundFileSink", "dest"));

    playbacknet->updctrl("mrs_natural/inSamples", windowsize);
    playbacknet->updctrl("mrs_real/israte", 44100.0);
    playbacknet->updctrl("mrs_real/osrate", 44100.0);

    playbacknet->updctrl("SoundFileSource/src/mrs_string/filename", sfname);
    playbacknet->updctrl("SoundFileSink/dest/mrs_string/filename", "output.wav");
    int srate = playbacknet->getctrl("mrs_natural/inSamples")->to<mrs_natural>();

    /*
    // values optimized for window size of 512
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/inSamples", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakSpacing", 1.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakStrength", 0.1);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStart", 0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakEnd", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakGain", 1.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStrengthReset", 1);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakDecay", 0.99);
    */


    playbacknet->updctrl("Peaker/peaker/mrs_real/peakStrength", 0.001);
    playbacknet->updctrl("Peaker/peaker/mrs_real/peakSpacing", 0.0);
    //playbacknet->updctrl("Peaker/peaker/mrs_natural/peakEnd", srate);

    while(playbacknet->getctrl("SoundFileSource/src/mrs_bool/notEmpty")->to<mrs_bool>())
        playbacknet->tick();
    //cout << *playbacknet << endl;

    delete playbacknet;
}

void drumExtract3() {
    MarSystemManager mng;

    mrs_natural windowsize = 512;

    MarSystem* audioget= mng.create("Series", "playbacknet");
    audioget->addMarSystem(mng.create("AudioSource", "src"));
    //audioget->addMarSystem(mng.create("SoundFileSink", "predest"));
    audioget->updctrl("mrs_natural/inSamples", windowsize);
    audioget->updctrl("mrs_real/israte", 44100.0);
    audioget->updctrl("mrs_real/osrate", 44100.0);
    //audioget->updctrl("AudioSource/src/mrs_natural/device", 2);
    audioget->updctrl("AudioSource/src/mrs_bool/initAudio", true);

    MarSystem* playbacknet = mng.create("Series", "playbacknet");
    //playbacknet->addMarSystem(mng.create("AudioSource", "src"));
    //playbacknet->addMarSystem(mng.create("SoundFileSink", "predest"));
    playbacknet->addMarSystem(mng.create("RadioDrumInput", "rd"));
    playbacknet->addMarSystem(mng.create("PeakerAdaptive", "peaker"));
    //playbacknet->addMarSystem(mng.create("SoundFileSink", "dest"));
        
    playbacknet->updctrl("mrs_natural/inSamples", windowsize);
    //playbacknet->updctrl("mrs_natural/onSamples", windowsize);
    playbacknet->updctrl("mrs_real/israte", 44100.0);
    playbacknet->updctrl("mrs_real/osrate", 44100.0);
    //playbacknet->updctrl("AudioSource/src/mrs_natural/device", 2);
    //playbacknet->updctrl("AudioSource/src/mrs_bool/initAudio", true);
    playbacknet->updctrl("RadioDrumInput/rd/mrs_bool/initmidi", true);

    //audioget->updctrl("SoundFileSink/predest/mrs_string/filename", "prepeakpicking.au");
    //playbacknet->updctrl("SoundFileSink/dest/mrs_string/filename", "postpeakpicking.au");
    //cout << *playbacknet<< endl;

    int srate = playbacknet->getctrl("mrs_natural/inSamples")->to<mrs_natural>();

    // values optimized for window size of 512
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/inSamples", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakSpacing", 4.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakStrength", 0.7);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStart", 0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakEnd", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakGain", 1.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStrengthReset", 2);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakDecay", 0.999);

    MarSystem* extractNet= mng.create("Series", "extractNet");

    //extractNet->addMarSystem(mng.create("ZeroCrossings", "zerocross"));
    //extractNet->addMarSystem(mng.create("RMS", "rms"));
    extractNet->addMarSystem(mng.create("Windowing", "ham"));
    extractNet->addMarSystem(mng.create("Spectrum", "spk"));
    extractNet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
    MarSystem* featureFanout = mng.create("Fanout", "featureFanout");
    featureFanout->addMarSystem(mng.create("Centroid", "centroid"));
    featureFanout->addMarSystem(mng.create("Rolloff", "rolloff"));
    featureFanout->addMarSystem(mng.create("MFCC", "mfcc"));
    featureFanout->addMarSystem(mng.create("Kurtosis", "kurtosis"));
    featureFanout->addMarSystem(mng.create("Skewness", "skewness"));
    extractNet->addMarSystem(featureFanout);

    extractNet->addMarSystem(mng.create("Annotator", "ann"));
    extractNet->addMarSystem(mng.create("WekaSink",  "wsink"));

    extractNet->updctrl("WekaSink/wsink/mrs_natural/nLabels", 2);
    extractNet->updctrl("WekaSink/wsink/mrs_string/labelNames","center, edge");  
    extractNet->updctrl("WekaSink/wsink/mrs_string/filename", "art.arff");

    /*
    // use this block to label regression
    extractNet->updctrl("WekaSink/wsink/mrs_bool/regression", true);
    extractNet->updctrl("WekaSink/wsink/mrs_bool/putHeader", true);    
    extractNet->updctrl("WekaSink/wsink/mrs_string/filename", "art.arff");
    */

    extractNet->updctrl("mrs_natural/inSamples",windowsize);
    extractNet->updctrl("mrs_natural/onSamples",windowsize);
    extractNet->updctrl("mrs_real/israte", 44100.0);
    extractNet->updctrl("mrs_real/osrate", 44100.0);

//    cout << *extractNet << endl;

    realvec in1, out1, analysisvec, out2,  peakpickout;
    in1.create(audioget->getctrl("mrs_natural/inObservations")->to<mrs_natural>(), 
            audioget->getctrl("mrs_natural/inSamples")->to<mrs_natural>());
    out1.create(audioget->getctrl("mrs_natural/onObservations")->to<mrs_natural>(), 
            audioget->getctrl("mrs_natural/onSamples")->to<mrs_natural>());
    
    peakpickout.create(playbacknet->getctrl("mrs_natural/onObservations")->to<mrs_natural>(), 
            playbacknet->getctrl("mrs_natural/onSamples")->to<mrs_natural>());

    analysisvec.create(extractNet->getctrl("mrs_natural/inObservations")->to<mrs_natural>(), 
            extractNet->getctrl("mrs_natural/inSamples")->to<mrs_natural>());
    out2.create(extractNet->getctrl("mrs_natural/onObservations")->to<mrs_natural>(), 
            extractNet->getctrl("mrs_natural/onSamples")->to<mrs_natural>());

    mrs_natural count = 1;
    mrs_natural radialposition;
    
    while ( true ) 
    { 
        audioget->process(in1,out1);
        playbacknet->process(out1,peakpickout);

        for (int i = 0; i < windowsize;i++)
        {   
            if ( peakpickout(i) > 0)
            {
                int windowposition = i;
        
                for (int j = 0; j < windowsize; j++){
                    analysisvec(windowposition++) = out1(j);
                }

                audioget->process(in1,out1);

                for (int j = 0; j < windowsize-i; j++){
                    analysisvec(windowposition++) = out1(j);
                }
                   
                radialposition = playbacknet->getctrl("RadioDrumInput/rd/mrs_natural/rightstickx")->to<mrs_natural>(); 
		// use this for regression and comment out the next block
                //extractNet->setctrl("Annotator/ann/mrs_natural/label", radialposition);
	
                if (radialposition > 79)
                {
                    // hitting the edge
                    extractNet->setctrl("Annotator/ann/mrs_natural/label", 1);
                }
                /* else if (r > 61) 
                   {
                   cout << "middle" << endl;
                   pnet->setctrl("Annotator/ann/mrs_natural/label", 1);
                   }
                 */ 
                else
                {
                    // hitting the center
                    extractNet->setctrl("Annotator/ann/mrs_natural/label", 0);
                }

                cout << "drumit   "<< count++ << endl;
                cout << "label    " << extractNet->getctrl("Annotator/ann/mrs_natural/label")->to<mrs_natural>() << endl;
                cout << "radiodrum    "<< radialposition << endl;

                extractNet->process(analysisvec, out2);
            
                break;
            }    
        }   
    }   
    delete playbacknet;
}

void drumExtract2() {
    MarSystemManager mng;

    mrs_natural windowsize = 512;

    mrs_string sfname = "input.wav";
    MarSystem* playbacknet = mng.create("Series", "playbacknet");
    playbacknet->addMarSystem(mng.create("SoundFileSource", "src"));
    playbacknet->addMarSystem(mng.create("PeakerAdaptive", "peaker"));
    playbacknet->addMarSystem(mng.create("SoundFileSink", "dest"));

    playbacknet->updctrl("mrs_natural/inSamples", windowsize);
    playbacknet->updctrl("mrs_real/israte", 44100.0);
    playbacknet->updctrl("mrs_real/osrate", 44100.0);

    playbacknet->updctrl("SoundFileSource/src/mrs_string/filename", sfname);
    playbacknet->updctrl("SoundFileSink/dest/mrs_string/filename", "output.wav");
    int srate = playbacknet->getctrl("mrs_natural/inSamples")->to<mrs_natural>();

    // values optimized for window size of 512
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/inSamples", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakSpacing", 4.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakStrength", 0.5);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStart", 0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakEnd", srate);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakGain", 1.0);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_natural/peakStrengthReset", 2);
    playbacknet->updctrl("PeakerAdaptive/peaker/mrs_real/peakDecay", 0.999);

    MarSystem* extractNet= mng.create("Series", "extractNet");
    extractNet->addMarSystem(mng.create("SoundFileSource","src"));
    //extractNet->addMarSystem(mng.create("SoundFileSink","dest"));

    extractNet->addMarSystem(mng.create("Windowing", "ham"));
    extractNet->addMarSystem(mng.create("Spectrum", "spk"));
    extractNet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
    MarSystem* featureFanout = mng.create("Fanout", "featureFanout");
    featureFanout->addMarSystem(mng.create("Centroid", "centroid"));
    featureFanout->addMarSystem(mng.create("Rolloff", "rolloff"));
    featureFanout->addMarSystem(mng.create("MFCC", "mfcc"));
    featureFanout->addMarSystem(mng.create("Kurtosis", "kurtosis"));
    featureFanout->addMarSystem(mng.create("Skewness", "skewness"));
    extractNet->addMarSystem(featureFanout);

    extractNet->addMarSystem(mng.create("Annotator", "ann"));
    extractNet->addMarSystem(mng.create("WekaSink",  "wsink"));

    // label everything as one class for now to get it working
    // later will use radio drum to generate these labels
    extractNet->updctrl("Annotator/ann/mrs_natural/label", 0);
    extractNet->updctrl("WekaSink/wsink/mrs_natural/nLabels", 1);
    extractNet->updctrl("WekaSink/wsink/mrs_string/labelNames","testhits");  
    extractNet->updctrl("WekaSink/wsink/mrs_string/filename", "art.arff");

    extractNet->updctrl("mrs_natural/inSamples",windowsize);
    extractNet->updctrl("mrs_natural/onSamples",windowsize);
    extractNet->updctrl("mrs_real/israte", 44100.0);
    extractNet->updctrl("mrs_real/osrate", 44100.0);
    extractNet->updctrl("SoundFileSource/src/mrs_string/filename", sfname);

    cout << *extractNet << endl;
    /*
       while(playbacknet->getctrl("SoundFileSource/src/mrs_bool/notEmpty")->to<mrs_bool>())
       playbacknet->tick();
     */
    //cout << *playbacknet << endl;

    realvec in1, out1;
    in1.create(playbacknet->getctrl("mrs_natural/inObservations")->to<mrs_natural>(), 
            playbacknet->getctrl("mrs_natural/inSamples")->to<mrs_natural>());
    out1.create(playbacknet->getctrl("mrs_natural/onObservations")->to<mrs_natural>(), 
            playbacknet->getctrl("mrs_natural/onSamples")->to<mrs_natural>());
    
    mrs_string filename;
    mrs_natural count = 1;
    
    while ( playbacknet->getctrl("SoundFileSource/src/mrs_bool/notEmpty")->to<mrs_bool>() ) 
    { 
        playbacknet->process(in1,out1);

        for (int i = 0; i < windowsize;i++)
        {   
            if ( out1(i) > 0)
            {
                int currentindex = playbacknet->getctrl("SoundFileSource/src/mrs_natural/pos")->to<mrs_natural>();
                currentindex = currentindex - windowsize + i;
                extractNet->updctrl("SoundFileSource/src/mrs_natural/pos", currentindex);
        
                cout << "drumit   "<< count++ << endl;
                cout << "time     "<< (mrs_real)(currentindex/44100.0) << endl;
                cout << "value    "<< out1(i) << endl;

                /*
                // this block is used for printing each extracted window to a soundfile
                ostringstream oss;
                oss << "drumhit" << setfill('0') << setw(4)  << count++ << ".wav";
                filename = oss.str();
                extractNet->updctrl("SoundFileSink/dest/mrs_string/filename",filename);
                cout << extractNet->getctrl("SoundFileSink/dest/mrs_string/filename")->to<mrs_string>() <<endl;
                */

                //cout << *extractNet << endl;
                extractNet->tick();
            
                break;
            }    
        }   
    }   

    delete playbacknet;
}


void readRMSmake(mrs_real length, string AudioFile) 
{
    cout << "Read AudioFiles and make RMS files" << endl;
    cout << AudioFile << endl;

    MarSystemManager mng;

    MarSystem* pnet = mng.create("Series", "pnet");    
    MarSystem* asrc = mng.create("SoundFileSource", "asrc");
    MarSystem* rms = mng.create("Rms", "rms");
    
    pnet->addMarSystem(asrc);
    pnet->addMarSystem(rms);
  
    pnet->updctrl("SoundFileSource/asrc/mrs_string/filename", AudioFile);   
    pnet->updctrl("AudioSink/dest/mrs_bool/initAudio", true);

        
    realvec rmsvec; 
    int len = 1000;
    rmsvec.create(len);
    int linecount = 0;

    // This is a hack to get size! change this
    realvec fret;
    fret.read("A_120_thumb_01.txt");

    for (mrs_natural t = 0; t < fret.getSize(); t++)
      {
	if (linecount > len)
	  {
	    len = len*2;
	    rmsvec.stretch(len);    
	  }
	pnet->tick();
	realvec out = pnet->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
	//cout << out(0,0) << endl;
	rmsvec(t) = out(0,0);
	linecount++;
	
      }
    
    
    rmsvec.stretch(linecount);
    AudioFile += "_rms.txt";
    rmsvec.write(AudioFile);

}

void readSitarSensors(mrs_real length) 
{
    cout << "Read Sitar Sensors" << endl;

    MarSystemManager mng;

    MarSystem* pnet = mng.create("Series", "pnet");    
    MarSystem* asrc = mng.create("SoundFileSource", "asrc");
    MarSystem* dest = mng.create("AudioSink", "dest");
    MarSystem* rms = mng.create("Rms", "rms");
    MarSystem* devibot = mng.create("DeviBot", "devibot");

    pnet->addMarSystem(asrc);
    pnet->addMarSystem(dest);
    pnet->addMarSystem(rms);
    pnet->addMarSystem(devibot);

    pnet->updctrl("SoundFileSource/asrc/mrs_string/filename", "A_120_01.au");   
    pnet->updctrl("AudioSink/dest/mrs_bool/initAudio", true);

    // Read in Thumb and Fret data into realvectors
    realvec thumb;
    realvec fret;
    thumb.read("A_120_thumb_01.txt");
    fret.read("A_120_fret_01.txt");
    realvec rmsvec; 
    rmsvec.create(fret.getSize());

    // Gather WISP data from Text files
    // realvec timer;
    realvec WispX;    
    realvec WispY;    
    realvec WispZ;    
    int len = fret.getSize();
    //timer.create(len);    
    WispX.create(len);    
    WispY.create(len);    
    WispZ.create(len);    
    
    //    float timestamp;
    float x;
    float y;
    float z;
    int linecount = 0;

    ifstream myfile;
    myfile.open("WISPTestData2.txt");
    while ( !myfile.eof () )
      {
	myfile >> x >> y >> z;	
	cout << x << y << z  << endl;
	if (linecount > len)
	  {
	    len = len*2;
	    //    timer.stretch(len);    
	    WispX.stretch(len);    
	    WispY.stretch(len);    
	    WispZ.stretch(len);    
	  }
	// timer(linecount) = timestamp;
	WispX(linecount) = x;
	WispY(linecount) = y;
	WispZ(linecount) = z;
	linecount++;
      }
    myfile.close();
    //timer.stretch(linecount);    
    WispX.stretch(linecount);    
    WispY.stretch(linecount);    
    WispZ.stretch(linecount);    
    


    // Set Robot Variables
    MarControlPtr arm = pnet->getctrl("DeviBot/devibot/mrs_natural/arm");
    MarControlPtr velocity = pnet->getctrl("DeviBot/devibot/mrs_natural/velocity");
    MarControlPtr strike = pnet->getctrl("DeviBot/devibot/mrs_bool/strike");
    
    pnet->updctrl(arm, DEVIBOT_NA);
    pnet->updctrl(velocity, 40);
    pnet->updctrl(strike, true);
    
    // Thumb Onset Detection Variables
    int deriv;
    int pderiv;
    int prevthumb = thumb(0);
    int flag = 0;
    int upThresh = 18;
    int downThresh = 15;
    int onsetWaitTime = 10; // 10::ms each note on
    int temp = 0;
    
    for (mrs_natural t = 0; t < fret.getSize(); t++)
    {
      //      cout << thumb(t) << "---";
      
      // Thumb Onset Detection
      deriv = thumb(t) - prevthumb; 
      pderiv = deriv*(-2) + deriv;
      
      if ((deriv > 0) && (deriv > downThresh) && (flag == 0))
	{
	  // down Stroke
	  pnet->updctrl(arm, DEVIBOT_NA);
	  pnet->updctrl(velocity, 40);
	  pnet->updctrl(strike, true);
	  flag = 1;
	  temp = t;
	}
      else if ((deriv < 0) && (pderiv > upThresh) && (flag == 0))
	{
	  // up Stroke
	  pnet->updctrl(arm, DEVIBOT_NA);
	  pnet->updctrl(velocity, 40);
	  pnet->updctrl(strike, true);
	  flag = 1;
	  temp = t;
	}
      else 
	{
	  // remove quick onsets
	  if (t-temp > onsetWaitTime)
	    {
	      flag = 0;
	    }
	}

      // set prev Thumb
      prevthumb = thumb(t);
      pnet->tick();
      realvec out = pnet->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
      // cout << out(0,0) << endl;
      rmsvec(t) = out(0,0);
    }
    rmsvec.write("A_80_rms_01.txt");

}

void readFrettoPitch(mrs_real length) 
{
    cout << "Read Fret to Pitch" << endl;
    MarSystemManager mng;

    MarSystem* pnet = mng.create("Series", "pnet");    
 
    // Mix Fan out 
    MarSystem* mix = mng.create("Fanout", "mix");
 
   //    pnet->addMarSystem(mng.create("Fanout", "mix"));
    
      // Direct Chain: Sitar to Speaker 
    MarSystem *DirectChain = mng.create("Series", "DirectChain");
    DirectChain->addMarSystem(mng.create("SoundFileSource", "asrc"));
    DirectChain->addMarSystem(mng.create("Gain", "dgain"));
   
    // Sine Chain: Fret data converted to Sine waves
    MarSystem* SineChain = mng.create("Series", "SineChain"); 
    SineChain->addMarSystem(mng.create("SineSource", "ssrc"));
    SineChain->addMarSystem(mng.create("Gain", "sgain"));
    SineChain->addMarSystem(mng.create("SoundFileSink", "fretpitch"));
    
    // Add Direct Chain and Sine Chain to  to Mix
    mix->addMarSystem(DirectChain);
    mix->addMarSystem(SineChain);

    // Add Mix to pnet
    pnet->addMarSystem(mix);
    
    // Send to speakers
    pnet->addMarSystem(mng.create("Sum", "mixsum"));
    pnet->addMarSystem(mng.create("AudioSink", "dest"));
   
    // File name of Sitar Sound File
    pnet->updctrl("Fanout/mix/Series/DirectChain/SoundFileSource/asrc/mrs_string/filename", "A_120_20.au");   

    // File name for SineWave Sound File
    pnet->updctrl("Fanout/mix/Series/SineChain/SoundFileSink/fretpitch/mrs_string/filename", "fretpitch.au");   
    
    // Link Controls to Relevant Variables    
    MarControlPtr freqptr = pnet->getctrl("Fanout/mix/Series/SineChain/SineSource/ssrc/mrs_real/frequency");
    MarControlPtr DirVolptr = pnet->getctrl("Fanout/mix/Series/DirectChain/Gain/dgain/mrs_real/gain");
    MarControlPtr SineVolptr = pnet->getctrl("Fanout/mix/Series/SineChain/Gain/sgain/mrs_real/gain");

    pnet->updctrl(DirVolptr, 2.0);
    pnet->updctrl(SineVolptr, 0.1);
    
    
    pnet->updctrl("AudioSink/dest/mrs_bool/initAudio", true);
    

    // Read Fret data into realvectors
    realvec fret;
    fret.read("A_120_fret_20.txt");

    for (mrs_natural t = 0; t < fret.getSize(); t++)
      {
	// Ma - F# 
	if ((fret(t) > 0) && (fret(t) < 5))
	  {
	    //Silence
	    //pnet->updctrl(freqptr, 184.997);  
	  }

	// Tivra Ma - G
	else if ((fret(t) > 5) && (fret(t) < 10))
	  {
	    pnet->updctrl(freqptr, 195.998);  
	  }
	
	// Pa - G# 
	else if ((fret(t) > 10) && (fret(t) < 15))
	  {
	    pnet->updctrl(freqptr, 207.652);  
	  }

	// Komal Dha - A
	else if ((fret(t) > 15) && (fret(t) < 20))
	  {
	    pnet->updctrl(freqptr, 220.000);  
	  }
	
	// Dha - A# 
	else if ((fret(t) > 20) && (fret(t) < 24))
	  {
	    pnet->updctrl(freqptr, 233.082);  
	  }

	// Komal Ni - B
	else if ((fret(t) > 24) && (fret(t) < 28))
	  {
	    pnet->updctrl(freqptr, 246.942);  
	  }
	
	// Ni - C 
	else if ((fret(t) > 28) && (fret(t) < 36))
	  {
	    pnet->updctrl(freqptr, 261.626);  
	  }
	
	// Sa - C#
	else if ((fret(t) > 35) && (fret(t) < 42))
	  {
	    pnet->updctrl(freqptr, 277.183);    
	  }

	// Re - D# 
	else if ((fret(t) > 41) && (fret(t) < 50))
	  {
	    pnet->updctrl(freqptr, 311.127);    
	  }

	// Komal Ga - E 
	else if ((fret(t) > 49) && (fret(t) < 58))
	  {
	    pnet->updctrl(freqptr, 329.628);     
	  }
	
	// Ga - F 
	else if ((fret(t) > 57) && (fret(t) < 66))
	  {
	    pnet->updctrl(freqptr, 349.228);    
	  }

	// Ma - F# 
	else if ((fret(t) > 65) && (fret(t) < 75))
	  {
	    pnet->updctrl(freqptr, 369.994);    
	  }

	// Tivra Ma - G 
	else if ((fret(t) > 74) && (fret(t) < 83))
	  {
	    pnet->updctrl(freqptr, 391.995);    
	  }
	
	// Pa - G# 
	else if ((fret(t) > 82) && (fret(t) < 91))
	  {
	    pnet->updctrl(freqptr, 415.305);    
	  }
	
	// Dha - A# 
	else if ((fret(t) > 90) && (fret(t) < 99))
	  {
	    pnet->updctrl(freqptr, 466.164);    
	  }

	// Komal Ni - B 
	else if ((fret(t) > 98) && (fret(t) < 106))
	  {
	    pnet->updctrl(freqptr, 493.883);    
	  }
	
	// Ni - C
	else if ((fret(t) > 105) && (fret(t) < 112))
	  {
	    pnet->updctrl(freqptr, 523.251);    
	  }

	// Sa - C#
	else if ((fret(t) > 111) && (fret(t) < 117))
	  {
	    pnet->updctrl(freqptr, 554.4);    
	  }
	
	// Re - D#
	else if ((fret(t) > 116) && (fret(t) < 122))
	  {
	    pnet->updctrl(freqptr, 622.3);    
	  }

	// Ga - F
	else if ((fret(t) > 121) && (fret(t) < 125))
	  {
	    pnet->updctrl(freqptr, 698.5);    
	  }
	
	// Ma - F#
	else if ((fret(t) > 124) && (fret(t) < 128))
	  {
	    pnet->updctrl(freqptr, 740.0);    
	  }
	  
      pnet->tick();
    }


}

void recordSitarSensors(mrs_real length) 
{
  // This function records syncronized audio data, thumb pressure 
  // and fret data. Audio is recorded at 44100 HZ. Thumb and Fret
  // are recorded at 44100/512 Hz. Function is also set up to send 
  // Midi Output signal to start and stop other processes, such as 
  // using MAX/MSP to record Open Sound Control Data. 

    cout << "Record Sitar Sensor Data" << endl;
    MarSystemManager mng;

    MarSystem* pnet = mng.create("Series", "pnet");

    MarSystem* recordNet = mng.create("Series", "recordNet");
    MarSystem* asrc = mng.create("AudioSource", "asrc");
    MarSystem* dest = mng.create("SoundFileSink", "dest");
    MidiInput* vmidi = new MidiInput("vmidi");
    Esitar* esitar = new Esitar("esitar");
    MarSystem* devibot = mng.create("DeviBot", "devibot");

    recordNet->addMarSystem(asrc);
    recordNet->addMarSystem(vmidi);
    recordNet->addMarSystem(esitar);
    recordNet->addMarSystem(devibot);

    // recordNet->addMarSystem(dest); 
    recordNet->updctrl("mrs_real/israte", 44100.0); 
    recordNet->updctrl("mrs_real/osrate", 44100.0); 
    recordNet->linkctrl("mrs_bool/notEmpty", "AudioSource/asrc/mrs_bool/notEmpty");
    // this buffer size is needed for the Tascam FW 1804
    //recordNet->updctrl("AudioSource/asrc/mrs_natural/bufferSize",6144);
    recordNet->updctrl("AudioSource/asrc/mrs_bool/initAudio", true);

    pnet->addMarSystem(recordNet);
    
    pnet->addMarSystem(dest);
    
    // pnet->addMarSystem(mng.create("PlotSink", "psink"));
    pnet->addMarSystem(mng.create("Windowing", "ham"));
    pnet->addMarSystem(mng.create("Spectrum", "spk"));
    pnet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
    MarSystem* features = mng.create("Fanout", "features");
    features->addMarSystem(mng.create("Centroid", "cntrd"));
    features->addMarSystem(mng.create("Rolloff", "rolloff"));
    features->addMarSystem(mng.create("MFCC", "mfcc"));
    pnet->addMarSystem(features);

    pnet->addMarSystem(mng.create("Annotator", "ann"));
    pnet->addMarSystem(mng.create("WekaSink", "wsink"));

    pnet->updctrl("WekaSink/wsink/mrs_bool/regression", true);
    pnet->updctrl("WekaSink/wsink/mrs_bool/putHeader", true);    
    pnet->updctrl("WekaSink/wsink/mrs_string/filename", "vsensor.arff");

    pnet->updctrl("SoundFileSink/dest/mrs_real/israte", 44100.0); 
    pnet->updctrl("SoundFileSink/dest/mrs_real/osrate", 44100.0); 
    pnet->updctrl("SoundFileSink/dest/mrs_string/filename", "vsens.au");   

    mrs_real srate = recordNet->getctrl("AudioSource/asrc/mrs_real/israte")->to<mrs_real>();
    mrs_natural inSamples = recordNet->getctrl("AudioSource/asrc/mrs_natural/inSamples")->to<mrs_natural>();
    mrs_natural iterations = (mrs_natural)((srate * length) / inSamples);

    int r,f;

    //    cout << *recordNet << endl; 

    int len; 
    len = 2600;
    realvec thumb(len);
    realvec fret(len);

    MarControlPtr arm = pnet->getctrl("Series/recordNet/DeviBot/devibot/mrs_natural/arm");
    MarControlPtr velocity = pnet->getctrl("Series/recordNet/DeviBot/devibot/mrs_natural/velocity");
    MarControlPtr strike = pnet->getctrl("Series/recordNet/DeviBot/devibot/mrs_bool/strike");

    
    // Start Signal 
    pnet->updctrl(arm, DEVIBOT_NA);
    pnet->updctrl(velocity, 50);
    pnet->updctrl(strike, true);

    // Wait
    sleep(1);

    pnet->updctrl(arm, DEVIBOT_NA);
    pnet->updctrl(velocity, 50);
    pnet->updctrl(strike, true);
    
    //    for (mrs_natural t = 0; t < iterations; t++)
    for (mrs_natural t = 0; t < len; t++)
    {

      /*  if (t % 100 == 0) 
	{
	  pnet->updctrl(arm, DEVIBOT_GE);
	  pnet->updctrl(velocity, 50);
	  pnet->updctrl(strike, true);
	}


      if (t % 100 == 50) 
	{
	  pnet->updctrl(arm, DEVIBOT_NA);
	  pnet->updctrl(velocity, 50);
	  pnet->updctrl(strike, true);
	  
	}
      
      */
      
      f = esitar->fret;
      r = esitar->thumb; 
      cout << "thumb: " << r << endl;       
      
      fret(t) = f;
      thumb(t) = r;
      
      pnet->setctrl("Annotator/ann/mrs_natural/label", r);
      pnet->tick();
    }

    // Stop Signal
    pnet->updctrl(arm, DEVIBOT_GE);
    pnet->updctrl(velocity, 50);
    pnet->updctrl(strike, true);
    
    thumb.write("thumb.plot");
    fret.write("fret.plot");
}


int main(int argc, const char **argv)
{
    MRSDIAG("VirtualSensor.cpp - main");

    string progName = argv[0];  

    // read command-line options
    initOptions();
    cmd_options.readOptions(argc, argv);
    loadOptions();

    vector<string> soundfiles = cmd_options.getRemaining();
    vector<string>::iterator sfi;

    vector<Collection> cls;
    string classNames = "";

    for (sfi = soundfiles.begin(); sfi != soundfiles.end(); ++sfi) 
    {	
        string sfname = *sfi;

        Collection l;
        readCollection(l,sfname);
        l.write("patata.mf");

        classNames += (l.name()+',');
        cls.push_back(l);

    }

    if (helpopt) 
        printHelp(progName);

    if (usageopt)
        printUsage(progName);
    
    /*    if (trainopt)
        drumExtract(cls, classNames);
    else
        recordVirtualThumbSensor( lengthopt );
    */

    //    cout << "INSTRUMENTO OPT"<< instrumentopt << endl;

    if (instrumentopt == 0)
        recordVirtualSensor( lengthopt );
    else if (instrumentopt == 1)
        readSitarSensors( lengthopt );
    else if (instrumentopt == 2)
        recordSitarSensors( lengthopt );
    else if (instrumentopt == 3)
        readFrettoPitch( lengthopt );
    else if (instrumentopt == 4)
    {
        Collection l;
        l.read("rms.mf");

        for (int i=0; i < l.size(); i++)
        {
            readRMSmake( lengthopt, l.entry(i) );
        }
    }
    else if (instrumentopt == 5)    
        drumExtract2();
    else if (instrumentopt == 6)    
        extractHits();
    else if (instrumentopt == 7)    
        drumExtract3();
    else if (instrumentopt == 8)    
        testrmspreprocess();

    exit(0);
}


// old version of extracting drum 
// written before silenceremove marsystem was written
void drumExtract(vector<Collection> cls, string classNames)
{
    MarSystemManager mng;
    MarSystem* src = mng.create("SoundFileSource", "src");
    src->updctrl("mrs_natural/inSamples", 4096);


    mrs_natural inObservations = src->getctrl("mrs_natural/inObservations")->to<mrs_natural>();
    mrs_natural inSamples = src->getctrl("mrs_natural/inSamples")->to<mrs_natural>();  

    realvec in(inObservations, inSamples);
    realvec out(inObservations, inSamples);

    mrs_natural cj,i;
    mrs_natural win = 0;
    mrs_natural startPos = 0;
    mrs_natural endPos = 0;
    mrs_natural startWin = 0;
    mrs_natural endWin = 0;

    MarSystem* extractNet = mng.create("Series", "extractNet");
    extractNet->addMarSystem(src);

    MarSystem* spectimeFanout = mng.create("Fanout", "spectimeFanout");
    spectimeFanout->addMarSystem(mng.create("ZeroCrossings", "zcrs"));
    spectimeFanout->addMarSystem(mng.create("Rms", "rms"));

    MarSystem* spectralNet = mng.create("Series", "spectralNet");
    spectralNet->addMarSystem(mng.create("Windowing", "ham"));
    spectralNet->addMarSystem(mng.create("Spectrum", "spk"));
    spectralNet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
    MarSystem* featureFanout = mng.create("Fanout", "featureFanout");
    featureFanout->addMarSystem(mng.create("Centroid", "centroid"));
    featureFanout->addMarSystem(mng.create("Rolloff", "rolloff"));
    // featureFanout->addMarSystem(mng.create("MFCC", "mfcc"));
    // featureFanout->addMarSystem(mng.create("Kurtosis", "kurtosis"));
    // featureFanout->addMarSystem(mng.create("Skewness", "skewness"));
    spectralNet->addMarSystem(featureFanout);

    spectimeFanout->addMarSystem(spectralNet);

    extractNet->addMarSystem(spectimeFanout);

    extractNet->addMarSystem(mng.create("Annotator", "ann"));
    extractNet->addMarSystem(mng.create("WekaSink",  "wsink"));
    extractNet->addMarSystem(mng.create("GaussianClassifier", "classifier"));  

    extractNet->updctrl("WekaSink/wsink/mrs_natural/nLabels", (mrs_natural)cls.size());
    extractNet->updctrl("WekaSink/wsink/mrs_string/labelNames",classNames);  
    extractNet->updctrl("WekaSink/wsink/mrs_string/filename", "art.arff");


    extractNet->updctrl("GaussianClassifier/classifier/mrs_natural/nLabels", (mrs_natural)cls.size());
    extractNet->updctrl("GaussianClassifier/classifier/mrs_string/mode","train");     


    for (cj=0; cj < (mrs_natural)cls.size(); cj++)
    {
        Collection l = cls[cj];
        extractNet->updctrl("Annotator/ann/mrs_natural/label", cj);

        for (i=0; i < l.size(); i++)
        { 
            win = 0;
            startPos = 0;
            endPos = 0;
            startWin = 0;
            endWin = 0;
            src->updctrl("mrs_string/filename", l.entry(i));
            cout << "Processing " << l.entry(i) << endl;

            src->updctrl("mrs_natural/inSamples", 4096);

            while(src->getctrl("mrs_bool/notEmpty")->to<mrs_bool>()) 
            {
                src->process(in,out);

                for (mrs_natural t = 0; t < inSamples; t++)
                {
                    if ((fabs(out(0,t)) > 0.1)&&(startPos == 0))
                    {

                        startPos = t;
                        startWin = win;
                    }
                    if ((fabs(out(0,t)) > 0.999)&&(endPos == 0))
                    {
                        endPos = t;
                        endWin = win;
                    }

                }      
                win++;
            }
            endPos = startPos + 512;

            extractNet->updctrl("mrs_natural/inSamples", 
                    endPos - startPos);
            extractNet->updctrl("SoundFileSource/src/mrs_natural/pos", startPos);
            extractNet->tick();



        }
    }


    extractNet->updctrl("GaussianClassifier/classifier/mrs_bool/done", true);
    extractNet->updctrl("GaussianClassifier/classifier/mrs_string/mode","predict");   	  
    extractNet->tick();  

    cout << (*extractNet) << endl;


    cout << "Wrote " << extractNet->getctrl("WekaSink/wsink/mrs_string/filename")->to<mrs_string>() << endl;


    return;

    /* src->updctrl("mrs_string/filename", filename);
       src->updctrl("mrs_natural/inSamples", 4096);


       extractNet->tick();
     */ 
}


