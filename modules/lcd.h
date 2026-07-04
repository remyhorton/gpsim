/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef MODULES_LCD_H_
#define MODULES_LCD_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE
#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "../src/modules.h"
#include "../src/trigger.h"
#include "../src/gpsim_time.h"

namespace Lcds {

class LmuxAttribute;
class TypeBAttribute;
class LCD_Interface;  // Defined in lcd.cc

/*
enum ActiveStates {
  HIGH,
  LOW
};
*/


//------------------------------------------------------------------------
// LCD base class

class Lcd_base : public Module {
public:
  Lcd_base(const char *_name, const char *desc);
  ~Lcd_base() {}

#ifdef HAVE_GUI
  virtual void build_window() = 0;
  virtual void update() = 0;
#endif

  unsigned int interface_seq_no;
};

class Lcd_Input : public IOPIN
{
public:
    Lcd_Input(const std::string &name, Lcd_base *pParent);

    virtual void setDrivenState(bool);
    virtual void get(char *return_str, int len);
    void set_nodeVoltage(double new_nodeVoltage) override;
    void clear_last_time() { last_time = 0;}

private:
    Lcd_base *m_pParent;
    double old_volt = 0.;
    guint64 last_time = 0;
};

//------------------------------------------------------------------------
// 7-segment lcds

// define a point
typedef struct {
  double x;
  double y;
} XfPoint;


#define MAX_PTS         6       /* max # of pts per segment polygon */
#define NUM_SEGS        7       /* max number segment pins */
#define NUM_COMS	4	/* max number is com pins */
#define NUM_ELEMENTS    7	/* number of display segments */
/*
 * These constants give the bit positions for the segmask[]
 * digit masks.
 */
#define TOP             0
#define TOP_RIGHT       1
#define BOT_RIGHT       2
#define BOTTOM          3
#define BOT_LEFT        4
#define TOP_LEFT        5
#define MIDDLE          6
//#define DECIMAL_POINT   7

typedef XfPoint segment_pts[NUM_SEGS][MAX_PTS];


class Lcd_7Segments : public Lcd_base, public TriggerObject {
public:
  guint w_width;
  guint w_height;

  segment_pts seg_pts;

#ifdef HAVE_GUI
  GtkWidget *darea;
#endif

  explicit Lcd_7Segments(const char *);
  ~Lcd_7Segments();

#ifdef HAVE_GUI
  void build_segments(int w, int h);

  virtual void build_window();
  virtual void update();
#endif
//  unsigned int getPinState();
//  double SegDrive(int i);
  void set_lmux(gint64 _lmux);
  void set_typeB(bool r) {TypeB = r;}
  void set_clock(unsigned int dt);
  void lmux_compute();

  // Inheritances from the Package class
  virtual void create_iopin_map();

  // Inheritance from Module class
  const virtual char *type() { return "lcd_7segments"; }
  static Module *construct(const char *new_name);
  void callback() override;
  void callback_print() override;

  gint64   	Lmux = 0;
  bool  	TypeB = false;
  guint64 	future_cycle = 0;
  unsigned int	delta_t = 0;
  unsigned int  phase = 0;
  unsigned int  num_phases = 2 * (Lmux + 1);
  unsigned int  deadman;
  double	phase_maxV;
  double	temp_maxV;

  LmuxAttribute *m_Lmux;
  TypeBAttribute *m_TypeB;
private:
#ifdef HAVE_GUI
  static gboolean lcd7_expose_event(GtkWidget *widget, GdkEvent *event,
                                    gpointer user_data);
#endif
  Lcd_Input *m_com_pins[NUM_COMS];
  Lcd_Input *m_seg_pins[NUM_SEGS];
  double    deltaV[NUM_SEGS];
  double    dc_V[NUM_SEGS];
  double    temp_deltaV[NUM_SEGS];
};


//------------------------------------------------------------------------
// Simple LCD
//
class ColorAttribute;
class ActiveStateAttribute;

} // end of namespace Lcd
#endif //  MODULES_LCD_H_
