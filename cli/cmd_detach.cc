/*
   Copyright (C) 1999 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <glib.h>

#include <iostream>
#include <string>

#include "command.h"
#include "misc.h"
#include "../src/gpsim_object.h"
#include "../src/processor.h"
#include "../src/stimuli.h"
#include "../src/value.h"
#include "cmd_detach.h"

cmd_detach detach;

static cmd_options cmd_detach_options[] =
{
  {nullptr, 0, 0}
};


cmd_detach::cmd_detach()
  : command("detach", nullptr)
{ 
  brief_doc = "Detach stimuli";

  long_doc = "detach stimulus_1 [stimulus_2 stimulus_N]\n"
    "\tDetach is used to disconnect stimuli from nodes. At least one\n"
    "\tstimulus must be specified. A stimulus is either a CPU or module\n"
    "\tI/O pin or a stimulus name.\n\n"
    "\tstimulus_n                 May be one of four forms:\n"
    "\tpin(<number>) or pin(<symbol>)\n"
    "\t    This refers to a pin of the current active CPU.\n"
    "\t    <number> is the pin number\n"
    "\t    <symbol> is an integer symbol whose value is a pin number\n"
    "\n"
    "\t<connection> or pin(<connection>)\n"
    "\t    These two forms are treated exactly the same\n"
    "\t            ( i.e. the pin() has no meaning).\n"
    "\t    <connection> is a stimulus name or an I/O pin name.\n"
    "\t            I/O pin name can be just the pin name for the CPU or\n"
    "\t                <module_name>.pin_name for a module\n";

  op = cmd_detach_options; 
}

extern void stimuli_detach(gpsimObjectList_t *pPinList);

void cmd_detach::detach(gpsimObjectList_t *pPinList)
{
  stimuli_detach(pPinList);

  //cout <<"deleting stimulus list\n";
  pPinList->clear(); 
  delete pPinList;
}
