/* -*-C++-*-
*******************************************************************************
*
* File:         mono_audio_out.cc
* Description:  play 350 Hz sine wave out sound card output
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
#include <VrAudioSink.h>
#include <VrSigSource.h>
#include <VrConnect.h>
#include <VrMultiTask.h>

const int SAMPLING_FREQ	= 8000;

int 
main (int argc, char **argv)
{
  // create the modules
  
  VrSigSource<short>* source =
    new VrSigSource<short>(SAMPLING_FREQ, VR_SIN_WAVE, 350, 8192);
  VrAudioSink<short>* sink = new VrAudioSink<short>();

  VrMultiTask *m = new VrMultiTask ();
  m->add (sink);
  
  NWO_CONNECT (source, sink);

  m->start ();
  while (1){
    m->process ();
  }
  m->stop ();

  return 0;
}
