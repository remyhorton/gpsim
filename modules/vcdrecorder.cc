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


/*
  vcdrecorder.cc

  This module records signals to a file.

  VcdRecorder - time and values are written to a file.
    .file         - name of file or pipe to write data to
    .ch?.name     - name of the signal
    .ch?.digital  - is the signal digital (true) or analog (false)
    .ch?          - Input pin
    .xpos         - module x position on breadboard
    .ypos         - module y position on breadboard
*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <fstream>
#include <iostream>
#include <cmath>

#include "vcdrecorder.h"
#include "module_attribute.h"

#include "../src/stimuli.h"
#include "../src/processor.h"
#include "../src/packages.h"

//----------------------------------------------------------------------
// Attributes

class VcdRecorder::FileNameAttribute : public String {
public:
  explicit FileNameAttribute(VcdRecorder *parent);
  ~FileNameAttribute();

  virtual void update();

private:
  VcdRecorder *m_Parent;
};

VcdRecorder::FileNameAttribute::FileNameAttribute(VcdRecorder *parent)
  : String("file", "", "Name of a file or pipe"), m_Parent(parent)
{
  parent->addSymbol(this);
}

VcdRecorder::FileNameAttribute::~FileNameAttribute()
{
  m_Parent->removeSymbol(this);
}

void VcdRecorder::FileNameAttribute::update()
{
  m_Parent->newFile();
}

class VcdRecorder::Input : public IOPIN {
public:
  Input(const char *, VcdRecorder *, char);
  ~Input();
  virtual void setDrivenState(bool);
  virtual void set_nodeVoltage(double);
  bool is_recorded();
  bool is_digital();
  void long_name(char *, int);
  char short_name();

private:

  VcdRecorder *m_pParent;
  char m_short_name;
  double m_lastv;
  BooleanAttribute *m_digitalattribute;
  StringAttribute *m_nameattribute;
};


VcdRecorder::Input::Input(const char *n, VcdRecorder *pParent, char short_name)
  : IOPIN(n), m_pParent(pParent), m_short_name(short_name), m_lastv(-HUGE_VAL)
{
  char name[40];
  snprintf(name, sizeof(name), "%s.digital", n);
  m_digitalattribute = new BooleanAttribute(name, true,
      "Is the signal digital (true) or analog (false)");
  pParent->addSymbol(m_digitalattribute);
  snprintf(name, sizeof(name), "%s.name", n);
  m_nameattribute = new StringAttribute(name, "",
      "Name of the signal. Must be filled for recording.");
  pParent->addSymbol(m_nameattribute);
  pParent->addSymbol(this);
}

VcdRecorder::Input::~Input()
{
  m_pParent->removeSymbol(m_digitalattribute);
  delete m_digitalattribute;
  m_pParent->removeSymbol(m_nameattribute);
  delete m_nameattribute;
  m_pParent->removeSymbol(this);
}

void VcdRecorder::Input::setDrivenState(bool new_dstate)
{
  IOPIN::setDrivenState(new_dstate);

  if (is_digital()) {
    if ((m_lastv != 0.0) == new_dstate && m_lastv >= 0.0) {
      return;
    }
    m_lastv = new_dstate ? HUGE_VAL : 0.0;
    m_pParent->record(new_dstate, m_short_name);
  }
}


void VcdRecorder::Input::set_nodeVoltage(double v)
{
  IOPIN::set_nodeVoltage(v);

  if (!is_digital()) {
    if (m_lastv == v) {
      return;
    }
    m_lastv = v;
    m_pParent->record(v, m_short_name);
  }
}

bool VcdRecorder::Input::is_recorded()
{
  return m_nameattribute->getVal()[0] != 0;
}

bool VcdRecorder::Input::is_digital()
{
  bool value;
  m_digitalattribute->get(value);
  return value;
}

void VcdRecorder::Input::long_name(char *value, int len)
{
  m_nameattribute->get(value, len);
}

char VcdRecorder::Input::short_name()
{
  return m_short_name;
}

//------------------------------------------------------------------------
Module *VcdRecorder::construct(const char *new_name)
{
  VcdRecorder *pVcdRecorder = new VcdRecorder(new_name);
  return pVcdRecorder;
}


VcdRecorder::VcdRecorder(const char *_name)
  : Module(_name,
      "File Recorder\n"
      " Attributes:\n"
      " .ch?.digital - is the signal digital (true) or analog (false)\n"
      " .ch?.name - name of the signal. Must be filled for recording\n"
      " .file - name of file or pipe to write data to\n"),
    m_fp(nullptr), m_lastcycle(~0u)
{
  create_pkg(CHANNELS);
  for (int i = 0; i < CHANNELS; i++)
  {
    char name[40];
    snprintf(name, sizeof(name), "ch%d", i);
    m_pin[i] = new Input(name, this, '!' + i);
    assign_pin(i + 1, m_pin[i]);
  }
  m_filename = new FileNameAttribute(this);

  if (verbose) {
    std::cout << description() << '\n';
  }
}


VcdRecorder::~VcdRecorder()
{
  for (int i = 0; i < CHANNELS; i++)
  {
    delete m_pin[i];
  }
  delete m_filename;
  delete m_fp;
}


void VcdRecorder::newFile()
{
  delete m_fp;
  m_fp = nullptr;
}

std::ofstream *VcdRecorder::file()
{
  if (m_fp)
  {
    return m_fp->rdstate() ? nullptr : m_fp;
  }

  if (m_filename->getVal())
  {
    m_fp = new std::ofstream(m_filename->getVal());
    if (m_fp->rdstate())
    {
        std::cerr << "Warning " << name() << " cannot open " << m_filename->getVal() << std::endl;
        return nullptr;
    } else {
      m_fp->precision(16);
      *m_fp << "$version" << std::endl << "\tgpsim VcdRecorder" << std::endl << "$end" << std::endl;
      *m_fp << "$timescale 1 ns $end" << std::endl;
      for (int i = 0; i < CHANNELS; i++)
      {
        Input *input = m_pin[i];
        if (input->is_recorded())
        {
          char long_name[100];
          input->long_name(long_name, sizeof(long_name));
          *m_fp << "$var " << (input->is_digital() ? "wire 1" : "real 64") << ' ' << input->short_name() << ' ' << long_name << " $end" << std::endl;
        }
      }
      *m_fp << "$enddefinitions $end" << std::endl;
      *m_fp << "$dumpvars" << std::endl;
    }
  }
  return m_fp;
}

void VcdRecorder::record_timestamp(std::ofstream *fp)
{
  Cycle_Counter &cycles = get_cycles();
  guint64 cur_cycle = cycles.get();
  if (m_lastcycle == cur_cycle) {
    return;
  }
  m_lastcycle = cur_cycle;
  guint64 cps = cycles.instruction_cps();
  *fp << '#' << (guint64)1000000000 * cur_cycle / cps << std::endl;
}

void VcdRecorder::record(bool NewVal, char short_name)
{
  std::ofstream *fp = file();

  if (fp) {
    char newvalue = (NewVal ? '1' : '0');
    record_timestamp(fp);
    *fp << newvalue << short_name << std::endl;

    if (verbose) {
      std::cout << this->name() << " recording " << newvalue << " @ 0x" << std::hex
           << m_lastcycle << std::endl;
    }
  }
}


void VcdRecorder::record(double NewVal, char short_name)
{
  std::ofstream *fp = file();

  if (fp) {
    record_timestamp(fp);
    *fp << 'r' << NewVal << ' ' << short_name << std::endl;

    if (verbose) {
      std::cout << this->name() << " recording " << NewVal << " @ 0x" << std::hex
           << m_lastcycle << std::endl;
    }
  }
}

