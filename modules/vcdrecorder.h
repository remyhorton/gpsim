/*
   Copyright (C) 2023 Zsolt Kajtar

This file is part of the libgpsim_modules library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


#ifndef MODULES_VCDRECORDER_H_
#define MODULES_VCDRECORDER_H_

#include <glib.h>

#include "../src/modules.h"

//----------------------------------------------------------------------

class VcdRecorder : public Module {
public:
  explicit VcdRecorder(const char *_name);
  ~VcdRecorder();

  static Module *construct(const char *new_name);

  void newFile();
  virtual void record(bool, char);
  virtual void record(double, char);

private:
  enum { CHANNELS = 16 };
  class Input;
  class FileNameAttribute;
  void record_timestamp(std::ofstream *);
  std::ofstream *file();

  FileNameAttribute *m_filename;
  Input *m_pin[CHANNELS];
  std::ofstream *m_fp;
  guint64 m_lastcycle;
};

#endif // MODULES_VCDRECORDER_H_
