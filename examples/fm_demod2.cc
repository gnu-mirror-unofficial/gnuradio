/* -*- Mode: c++ -*-
*******************************************************************************
*
* File:         fm_demod2.cc
* Description:  
*
*******************************************************************************
*/

/*
 * Copyright 2001 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

//
// This demodulates two broadcast FM stations at the same time
// using the microtune 4937 cable modem tuner module as the front end.
//


#include <GrMC4020Source.h>
#include <GrFFTSink.h>
#include <VrFixOffset.h>
#include <VrComplexFIRfilter.h>
#include <VrQuadratureDemod.h>
#include <VrRealFIRfilter.h>
#include <VrAudioSink.h>
#include <VrFileSink.h>
#include <VrFileSource.h>
#include <VrAudioSink.h>
#include <VrConnect.h>
#include <VrMultiTask.h>
#include <VrGUI.h>
#include <microtune_eval_board.h>


const int inputRate = 20000000;		// input sample rate from PCI-DAS4020/12

// const int chanTaps = 150;
const int chanTaps = 75;
const int CFIRdecimate = 125;
const float chanGain = 2.0;


//const float FMdemodGain = 8800;
//const float FMdemodGain = 4400;	// works well
const float FMdemodGain = 2200;
//const float FMdemodGain = 1100;

const int RFIRdecimate = 5;
//const int RFIRdecimate = 1;	// for now
//const int ifTaps = 22;
const int ifTaps = 50;
const float ifGain = 1.0;

const int quadRate = inputRate / CFIRdecimate;
const int audioRate = quadRate / RFIRdecimate;


#define	ENABLE_CHANNEL_FILTER_FFT	0


static void
usage ()
{
  cerr << "Usage: fm_demo2 <freq1-in-MHz> <freq2-in-MHz>\n";
  exit (1);
}


int main(int argc, char **argv)
{
  float volume = 1.0;

  VrGUI *guimain = new VrGUI(argc, argv);
  VrGUILayout *horiz = guimain->top->horizontal();
  VrGUILayout *vert = horiz->vertical();

  int	which = 1;	// which lp port eval board is connected to
  bool	ok = true;

  if (argc != 3)
    usage ();

  double freq1 = strtod (argv[1], 0);
  if (freq1 == 0.0)
    usage ();

  double freq2 = strtod (argv[2], 0);
  if (freq2 == 0.0)
    usage ();

  freq1 *= 1e6;
  freq2 *= 1e6;
  
  if (fabs (freq1 - freq2) > 5.5e6){
    cerr << "Sorry, freq1 and freq2 must be within 5.5 MHz of each other\n";
    exit (1);
  }

  // tune the front end to the mean frequency
  
  double target_freq = (freq1 + freq2) / 2;

  microtune_eval_board *eb = new microtune_eval_board (which);

  if (!eb->board_present_p ()){
    ok = false;
    fprintf (stderr, "microtune: eval board is NOT present\n");
  }

  double actual_freq;
  if (!eb->set_RF_freq (target_freq, &actual_freq)){
    ok = false;
    fprintf (stderr, "microtune: set_RF_freq (%g) failed\n", target_freq / 1e6);
  }

  if (!ok)
    exit (1);

#if 0
  cerr << "Actual freq: " << actual_freq * 1e-6
       << " delta: " << (actual_freq - target_freq) * 1e-6 << endl << endl;
#endif
  
  double IF_center_freq = eb->get_output_freq ();
  
  cerr << "Input Sampling Rate: " << inputRate << endl;
  cerr << "Complex FIR decimation factor: " << CFIRdecimate << endl;
  cerr << "QuadDemod Sampling Rate: " << quadRate << endl;
  cerr << "Real FIR decimation factor: " << RFIRdecimate << endl;
  cerr << "Audio Sampling Rate: " << audioRate << endl;


  VrMultiTask *m = new VrMultiTask ();

  //
  // setup common prefix of signal processing chain
  //

  // --> short
  VrSource<short> *source = 
    new GrMC4020Source<short>(inputRate, MCC_CH3_EN | MCC_ALL_1V);

  // short --> short 
  VrFixOffset<short,short> *offset_fixer =
    new VrFixOffset<short,short>();

  //
  // setup first FM demodulator chain
  //
  
  // short --> VrComplex
  VrComplexFIRfilter<short>* chanFilter_1 = 
    new VrComplexFIRfilter<short>(CFIRdecimate, chanTaps,
				  IF_center_freq + freq1 - actual_freq,
				  chanGain);

  // VrComplex --> float
  VrQuadratureDemod<float> *FMdemod_1 =
    new VrQuadratureDemod<float>(volume * FMdemodGain);

  // float --> short
  VrRealFIRfilter<float,short> *ifFilter_1 = 
    new VrRealFIRfilter<float,short>(RFIRdecimate, audioRate/2,
				     ifTaps, ifGain);

  //
  // setup second FM demodulator chain
  //
  
  // short --> VrComplex
  VrComplexFIRfilter<short>* chanFilter_2 = 
    new VrComplexFIRfilter<short>(CFIRdecimate, chanTaps,
				  IF_center_freq + freq2 - actual_freq,
				  chanGain);

  // VrComplex --> float
  VrQuadratureDemod<float> *FMdemod_2 =
    new VrQuadratureDemod<float>(volume * FMdemodGain);

  // float --> short
  VrRealFIRfilter<float,short> *ifFilter_2 = 
    new VrRealFIRfilter<float,short>(RFIRdecimate, audioRate/2,
				     ifTaps, ifGain);


  // order on the display depends on order of initialization, not construction

  // sink1 is channel filter output
  VrSink<VrComplex> *fft_sink1 = new GrFFTSink<VrComplex>(vert, 0, 200, 1024);

  // sink2 is fm demod output
  VrSink<float> *fft_sink2 = new GrFFTSink<float>(vert, 0, 200, 512);

  // sink3 is audio output
  VrSink<short> *fft_sink3 = new GrFFTSink<short>(horiz, 0, 200, 512);

  //connect the modules together

  // m->add (fft_sink3);
  // CONNECT (fft_sink3, ifFilter_1, audioRate, 0);
  CONNECT (ifFilter_1,  FMdemod_1, quadRate, 0);

  // m->add (fft_sink2);
  // CONNECT (fft_sink2, FMdemod_1, quadRate, 0);
  CONNECT (FMdemod_1, chanFilter_1, quadRate, 0);


  if (ENABLE_CHANNEL_FILTER_FFT){
    m->add (fft_sink1);
    CONNECT (fft_sink1, chanFilter_1, quadRate, 0);
  }

  CONNECT (chanFilter_1, offset_fixer, inputRate, 0);
  CONNECT (offset_fixer, source, inputRate, 0);

#if 1
  VrSink<short> *audio_sink = new VrAudioSink<short>();
#else
  VrSink<short> *audio_sink = new VrFileSink<short>("microtune_audio.sw");
#endif

  m->add (audio_sink);
  CONNECT (audio_sink, ifFilter_1, audioRate, 0);

  // connect up second path
  
#if 1
  CONNECT (ifFilter_2,  FMdemod_2, quadRate, 0);
  CONNECT (FMdemod_2, chanFilter_2, quadRate, 0);
  CONNECT (audio_sink, ifFilter_2, audioRate, 0);
  CONNECT (chanFilter_2, offset_fixer, inputRate, 0);
#endif


  m->start ();
  guimain->start ();

  while (1){
    guimain->processEvents(10 /*ms*/);
    m->process();
  }  
}
