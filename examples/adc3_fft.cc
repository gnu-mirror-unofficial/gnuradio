/* -*- Mode: c++ -*- */
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

/*
 * Sample application to illustrate the use of a GrSimpleScopeSink.
 * The input here is provided by the VrSigSource signal generator.
 */

#include <GrMC4020Source.h>
#include <GrFFTSink.h>
#include <VrFixOffset.h>
#include <VrNullSink.h>
#include <VrConnect.h>
#include <VrMultiTask.h>
#include "VrGUI.h" 

#define SAMPLING_FREQUENCY                         20e6
#define	MAX_FREQUENCY		(SAMPLING_FREQUENCY / 2)


#define IOTYPE short

VrSource<short> *source;
GrFFTSink<IOTYPE> *scope_sink;

int main(int argc, char **argv) {
  VrGUI *guimain = new VrGUI(argc, argv);
  VrGUILayout *horiz = guimain->top->horizontal();
  VrGUILayout *vert = horiz->vertical();
  VrSink<IOTYPE> *sink;
  VrFixOffset<short,IOTYPE> *offset_fixer;

  source = new GrMC4020Source<short>(SAMPLING_FREQUENCY, MCC_CH3_EN | MCC_ALL_5V);
  offset_fixer = new VrFixOffset<short,IOTYPE>();
  scope_sink = new GrFFTSink<IOTYPE>(vert, 20, 100, 1024);

  sink = scope_sink;

  NWO_CONNECT (source, offset_fixer);
  NWO_CONNECT (offset_fixer, sink);

  VrMultiTask *m = new VrMultiTask ();
  m->add (sink);
  m->start();
  guimain->start();

  while (1) {
    guimain->processEvents(10 /*ms*/);
    m->process();
  }
}
