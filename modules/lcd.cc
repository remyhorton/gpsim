/*
   Copyright (C) 2025 Roy R. Rankin
   Copyright (C) 2000 T. Scott Dattalo

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

  lcd.cc

  This is an example module illustrating how gpsim modules may be created.
  Additional examples may also be found with the gpsim.

  This particular example creates a 7-segment, common cathode LCD display.

  Pin Numbering of LCD:
  --------------------
       a
      ---
   f | g | b
      ---
   e |   | c
      ---
       d
  cc = common cathode


  Electrical:
  ----------

   Static Drive
   seg0 (a)  ---|>|--- com0
   seg1 (b)  ---|>|--- com0
   seg2 (c)  ---|>|---
   seg3 (d)  ---|>|---+
   seg4 (e)  ---|>|---+
   seg5 (f)  ---|>|---+
   seg6 (g)  ---|>|---+
               |
              cc

	      Static (Lmux = 0)
	seg  0 1 2 3 4 5 6
	com0 a b c d e f g

	      Lmux = 1
       seg  0  1  2  3
       com0 b  c  d
       com1 a  e  f  g

       		Lmux = 2
	seg  0  1  2
	com0 c  d
	com1 b  g  e
	com2    a  f

		Lmux = 3
	seg  0  1
	com0 d
	com1 c  e
	com2 g  f
	com3 b  a

  How It Works:
  ------------

  Once the Lcd module has been built (and optionally installcd), you
  can include it in your .stc file. See the examples subdirectory.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <typeinfo>
#include "../cli/input.h"

#define DEBUG 0
#if DEBUG != 0
#define Dprintf(arg) {printf("%s:%d %s ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include <cmath>

#include "../src/stimuli.h"
#include "../src/gpsim_interface.h"

#include "lcd.h"
#include "../src/packages.h"
#include <cassert>



namespace Lcds
{

class Lcd_7Segments;
//------------------------------------------------------------------------
class LmuxAttribute : public Integer
{
public:
    LmuxAttribute(Lcd_7Segments *_lcd)
        : Integer("Lmux", 0, "LCD Multiplex Level"), m_lcd7s(_lcd)
    {
        ;
    }

    using Integer::set;
    virtual void set(gint64 r)
    {

        Integer::set(r);
        m_lcd7s->set_lmux(r);
    }

    std::string toString() override
    {
        return Integer::toString("%" PRINTF_INT64_MODIFIER "d");
    }
    Lcd_7Segments *m_lcd7s;
};
class TypeBAttribute : public Boolean
{
public:
    TypeBAttribute(Lcd_7Segments *_lcd)
        : Boolean("TypeB", false, "LCD Type B"), m_lcd7s(_lcd)
    {
        ;
    }

    using Boolean::set;
    virtual void set(bool r)
    {

        Boolean::set(r);
        m_lcd7s->set_typeB(r);
    }

    std::string toString() override
    {
        return Boolean::toString("%" PRINTF_INT64_MODIFIER "d");
    }
    Lcd_7Segments *m_lcd7s;
};

Lcd_base::Lcd_base(const char *_name, const char *desc)
    : Module(_name, desc)
{
}
//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class LCD_Interface : public Interface
{
private:
    Lcd_base *lcd;

public:
    virtual void SimulationHasStopped(gpointer object)
    {
        Update(object);
    }
    virtual void Update(gpointer /* object */ )
    {
#ifdef HAVE_GUI

        if (lcd)
        {
            lcd->update();
        }

#endif
    }

    explicit LCD_Interface(Lcd_base *_lcd)
        : Interface((gpointer *) _lcd), lcd(_lcd)
    {
    }
};




//------------------------------------------------------------------------
//
Lcd_Input::Lcd_Input(const std::string &name, Lcd_base *pParent)
    : IOPIN(name.c_str(), 0.), m_pParent(pParent)
{
}


void Lcd_Input::set_nodeVoltage(double new_nodeVoltage)
{
    if (new_nodeVoltage != old_volt)
    {
        guint64 now = get_cycles().get();
        if (strcmp("com0", name().c_str()) == 0)
        {
            Lcd_7Segments *m_lcd7s = (Lcd_7Segments *)m_pParent;
            unsigned int dt = 0;
            if( new_nodeVoltage < 0.2 && old_volt > 0.68)
            {
                if (last_time == 0)
                {
                    last_time = now;
                }
                else
                {
                    dt = now-last_time;	// length of time for complete phase
                    last_time = now;
                    if (dt != m_lcd7s->delta_t)
                    {
                        Dprintf(("%s cycle time change from %d to %d\n", m_lcd7s->name().c_str(), m_lcd7s->delta_t, dt));
                        m_lcd7s->set_clock(dt);
                    }
                }
            }
        }
    }
    IOPIN::set_nodeVoltage(new_nodeVoltage);
    old_volt = new_nodeVoltage;
}
void Lcd_Input::setDrivenState(bool bNewState)
{
    IOPIN::setDrivenState(bNewState);
}


void Lcd_Input::get(char *return_str, int len)
{
    if (return_str)
    {
        strncpy(return_str, IOPIN::getState() ? "1" : "0", len);
    }
}


#ifdef HAVE_GUI

void Lcd_7Segments::update()
{
    if (get_interface().bUsingGUI())
    {
        gtk_widget_queue_draw(darea);
    }
}


gboolean Lcd_7Segments::lcd7_expose_event(GtkWidget *widget, GdkEvent *,
        gpointer user_data)
{
    Lcd_7Segments *lcd = static_cast<Lcd_7Segments *>(user_data);
    g_return_val_if_fail(widget != nullptr, TRUE);
    g_return_val_if_fail(GTK_IS_DRAWING_AREA(widget), TRUE);
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    guint max_width = allocation.width;
    guint max_height = allocation.height;
    // not a very O-O way of doing it... but here we go directly
    // to the I/O port and get the values of the segments
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
    // background color
    cairo_rectangle(cr, 0.0, 0.0, max_width, max_height);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);

    // box around display
    cairo_set_line_width (cr, 2.9);
    cairo_rectangle(cr, 0.0, 0.0, max_width, max_height);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_stroke (cr);

    double Vmin;
    double Vmax;
    if (lcd->Lmux == 0)
    {
        Vmin = 0.;
        Vmax = lcd->get_Vdd();
    }
    else
    {
        Vmin = lcd->get_Vdd() / 3.;
        Vmax = lcd->phase_maxV;
        if (Vmax < 0.1) Vmax = lcd->get_Vdd();
    }
    for (int i = 0; i < NUM_ELEMENTS; ++i)
    {
        //bright should be a number between 0. and 1. with 0. being black.
        //The following calculation does give bright > 1.0 but cairo_set_source_rgb
        //clamps the number to 1.0 in that case.
        double bright = (Vmax - lcd->deltaV[i]) / (Vmax - Vmin);
//	bright = (1. + std::erf(2. * bright -1.))/2.;
        cairo_set_source_rgb(cr, bright, bright, bright);

        XfPoint *pts = &(lcd->seg_pts[i][0]);
        cairo_move_to(cr, pts[0].x, pts[0].y);

        for (int j = 1; j < MAX_PTS; ++j)
        {
            cairo_line_to(cr, pts[j].x, pts[j].y);
        }

        cairo_line_to(cr, pts[0].x, pts[0].y);
        cairo_fill(cr);
    }

    cairo_destroy(cr);
    return true;
}


//-------------------------------------------------------------------
// build_segments
//
// from Dclock.c (v.2.0) -- a digital clock widget.
// Copyright (c) 1988 Dan Heller <argv@sun.com>
// Modifications 2/93 by Tim Edwards <tim@sinh.stanford.edu>
// And further modifications by Scott Dattalo <scott@dattalo.com>
//
// Each segment on the LCD is comprised of a 6 point polygon.
// This routine will calculate what those points should be and
// store them an arrary.

void Lcd_7Segments::build_segments(int w, int h)
{
    XfPoint *pts;
    float spacer, hskip, fslope, bslope, midpt, seg_width, segxw;
    float invcosphi, invsinphi, invcospsi, invsinpsi, slope;
    float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy5, dy6;
    float xfactor, temp_xpts[4];
    h -= 4;
    w_width = w;
    w_height = h;
    // Hard code the display parameters...
    float space_factor = 0.13;
    float width_factor = 0.13;
    float sxw = 0.13;
    float angle = 6;
    /* define various useful constants */
    segxw = sxw * w;
    slope = angle;
    seg_width = width_factor * w;
    spacer = w * space_factor;
    hskip = seg_width * 0.125;
    fslope = 1 / (segxw / seg_width + 1 / slope);
    bslope = -1 / (segxw / seg_width - 1 / slope);
    midpt = h / 2;
    /* define some trigonometric values */
    /*  phi is the forward angle separating two segments;
        psi is the reverse angle separating two segments. */
    invsinphi = sqrt(1 + fslope * fslope) / fslope;
    invcosphi = sqrt(1 + 1 / (fslope * fslope)) * fslope;
    invsinpsi = sqrt(1 + bslope * bslope) / -bslope;
    invcospsi = sqrt(1 + 1 / (bslope * bslope)) * bslope;
    /* define offsets from easily-calculated points for 6 situations */
    dx1 = hskip * invsinphi / (slope / fslope - 1);
    dy1 = hskip * invcosphi / (1 - fslope / slope);
    dx2 = hskip * invsinpsi / (1 - slope / bslope);
    dy2 = hskip * invcospsi / (bslope / slope - 1);
    dx3 = hskip * invsinphi;
    dx4 = hskip * invsinpsi;
    dx5 = hskip * invsinpsi / (1 - fslope / bslope);
    dy5 = hskip * invcospsi / (bslope / fslope - 1);
    dx6 = dy5;
    dy6 = dx5;
    /* calculate some simple reference points */
    temp_xpts[0] = spacer + (h - seg_width) / slope;
    temp_xpts[1] = spacer + (h - seg_width / 2) / slope + segxw / 2;
    temp_xpts[2] = spacer + h / slope + segxw;
    temp_xpts[3] = temp_xpts[0] + segxw;
    xfactor = w - 2 * spacer - h / slope - segxw;
    // calculate the digit positions
    pts = seg_pts[TOP];
    pts[0].y = pts[1].y = 2;
    pts[0].x = temp_xpts[2] - dx3;
    pts[1].x = w - spacer - segxw + dx4;
    pts[2].y = pts[5].y = (seg_width / 2) - dy5 - dy6;
    pts[5].x = temp_xpts[1] + dx5 - dx6;
    pts[2].x = pts[5].x + xfactor;
    pts[3].y = pts[4].y = seg_width;
    pts[4].x = temp_xpts[3] + dx4;
    pts[3].x = temp_xpts[0] + xfactor - dx3;
    pts = &(seg_pts[MIDDLE][0]);
    pts[0].y = pts[1].y = midpt - seg_width / 2;
    pts[0].x = spacer + (h - pts[0].y) / slope + segxw;
    pts[1].x = pts[0].x - segxw + xfactor;
    pts[2].y = pts[5].y = midpt;
    pts[3].y = pts[4].y = midpt + seg_width / 2;
    pts[5].x = spacer + (h - pts[5].y) / slope + segxw / 2;
    pts[2].x = pts[5].x + xfactor;
    pts[4].x = pts[0].x - seg_width / slope;
    pts[3].x = spacer + (h - pts[3].y) / slope + xfactor;
    pts = &(seg_pts[BOTTOM][0]);
    pts[3].y = pts[4].y = (float)h;
    pts[2].y = pts[5].y = h - (seg_width / 2) + dy5 + dy6;
    pts[0].y = pts[1].y = h - seg_width;
    pts[0].x = spacer + segxw + seg_width / slope + dx3;
    pts[1].x = spacer + (h - pts[1].y) / slope + xfactor - dx4;
    pts[4].x = spacer + segxw - dx4;
    pts[5].x = spacer + segxw / 2 + (h - pts[5].y) / slope + dx6 - dx5;
    pts[2].x = pts[5].x + xfactor;
    pts[3].x = spacer + xfactor + dx3;
    pts = &(seg_pts[TOP_LEFT][0]);
    pts[0].y = seg_width / 2 - dy6 + dy5;
    pts[1].y = seg_width + dy2;
    pts[2].y = seg_pts[MIDDLE][0].y - 2 * dy1;
    pts[3].y = seg_pts[MIDDLE][5].y - 2 * dy6;
    pts[4].y = seg_pts[MIDDLE][0].y;
    pts[5].y = seg_width - dy1;
    pts[0].x = temp_xpts[1] - dx5 - dx6;
    pts[1].x = temp_xpts[3] - dx2;
    pts[2].x = seg_pts[MIDDLE][0].x + 2 * dx1;
    pts[3].x = seg_pts[MIDDLE][5].x - 2 * dx6;
    pts[4].x = spacer + (h - pts[4].y) / slope;
    pts[5].x = temp_xpts[0] + dx1;
    pts = &(seg_pts[BOT_LEFT][0]);
    pts[0].y = seg_pts[MIDDLE][5].y + 2 * dy5;
    pts[1].y = seg_pts[MIDDLE][4].y + 2 * dy2;
    pts[2].y = seg_pts[BOTTOM][0].y - dy1;
    pts[3].y = seg_pts[BOTTOM][5].y - 2 * dy6;
    pts[4].y = h - seg_width + dy2;
    pts[5].y = midpt + seg_width / 2;
    pts[0].x = seg_pts[MIDDLE][5].x - 2 * dx5;
    pts[1].x = seg_pts[MIDDLE][4].x - 2 * dx2;
    pts[2].x = seg_pts[BOTTOM][0].x - dx3 + dx1;
    pts[3].x = seg_pts[BOTTOM][5].x - 2 * dx6;
    pts[4].x = spacer + seg_width / slope - dx2;
    pts[5].x = spacer + (midpt - seg_width / 2) / slope;
    pts = &(seg_pts[TOP_RIGHT][0]);
    pts[0].y = seg_width / 2 - dy5 + dy6;
    pts[1].y = seg_width - dy2;
    pts[2].y = midpt - seg_width / 2;
    pts[3].y = midpt - 2 * dy5;
    pts[4].y = pts[2].y - 2 * dy2;
    pts[5].y = seg_width + dy1;
    pts[0].x = temp_xpts[1] + xfactor + dx5 + dx6;
    pts[1].x = temp_xpts[3] + xfactor + dx1;
    pts[2].x = seg_pts[MIDDLE][0].x + xfactor;
    pts[3].x = seg_pts[MIDDLE][5].x + xfactor + dx5 * 2;
    pts[4].x = seg_pts[TOP_LEFT][4].x + xfactor + dx2 * 2;
    pts[5].x = temp_xpts[0] + xfactor - dx1;
    pts = &(seg_pts[BOT_RIGHT][0]);
    pts[0].y = seg_pts[MIDDLE][2].y + 2 * dy6;
    pts[1].y = midpt + seg_width / 2;
    pts[2].y = h - seg_width + dy1;
    pts[3].y = h - (seg_width / 2) + dy6 - dy5;
    pts[4].y = h - seg_width - dy2;
    pts[5].y = seg_pts[MIDDLE][3].y + 2 * dy1;
    pts[0].x = seg_pts[MIDDLE][2].x + 2 * dx6;
    pts[1].x = seg_pts[MIDDLE][3].x + segxw;
    pts[2].x = seg_pts[BOTTOM][1].x + dx4 + segxw - dx1;
    pts[3].x = seg_pts[BOTTOM][2].x + 2 * dx5;
    pts[4].x = seg_pts[BOTTOM][1].x + dx4 + dx2;
    pts[5].x = seg_pts[MIDDLE][3].x - 2 * dx1;
}


void Lcd_7Segments::build_window()
{
    darea = gtk_drawing_area_new();
    gtk_widget_set_size_request(darea, 140, 150);
    g_signal_connect(darea, "expose_event", G_CALLBACK(lcd7_expose_event), this);
    gtk_widget_set_events(darea, GDK_EXPOSURE_MASK);
    gtk_widget_show(darea);
    set_widget(darea);
}

#endif //HAVE_GUI

void Lcd_7Segments::callback()
{
    guint64 now = get_cycles().get();
#if DEBUG != 0
    int com_numb = phase/2;
#endif
    Dprintf(("%s now=%" PRINTF_GINT64_MODIFIER "d delta_t=%d phase=%d com%d=%.2f\n",
             name().c_str(), now, delta_t, phase, com_numb,
             m_com_pins[com_numb]->get_nodeVoltage()));
    lmux_compute();
    future_cycle = now + delta_t / num_phases;
    get_cycles().set_break(future_cycle, this);
}

void Lcd_7Segments::callback_print()
{
    std::cout << "Lcd_7Segments " << name() << " CallBack ID 0x"
              << std::hex << CallBackID << '\n';
}

//--------------------------------------------------------------

Lcd_7Segments::Lcd_7Segments(const char *name)
    : Lcd_base(name, "7 Segment LCD")
{

#ifdef HAVE_GUI

    if (get_interface().bUsingGUI())
    {
        build_segments(140, 150);
        build_window();
    }

#endif
    m_Lmux = new LmuxAttribute(this);
    addSymbol(m_Lmux);
    m_TypeB = new TypeBAttribute(this);
    addSymbol(m_TypeB);
    interface_seq_no = get_interface().add_interface(new LCD_Interface(this));
    create_iopin_map();
}


Lcd_7Segments::~Lcd_7Segments()
{
    for (int i = 0; i < NUM_SEGS; i++)
    {
        removeSymbol(m_seg_pins[i]);
        if (i < NUM_COMS)
            removeSymbol(m_com_pins[i]);
    }

    get_interface().remove_interface(interface_seq_no);
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Lcd_7Segments::create_iopin_map()
{
    // Define the physical package.
    //   The Package class, which is a parent of all of the modules,
    //   is responsible for allocating memory for the I/O pins.
    //
    //   The 7-segment LCD has 8 pins
    create_pkg(NUM_SEGS + NUM_COMS);
    float ypos = 6.0;

    for (int i = 1; i <= NUM_SEGS + NUM_COMS; i++)
    {
        package->setPinGeometry(i, 0.0, ypos, 0, true);
        ypos += 12.0;
    }

    // Here, we create and name the I/O pins. In gpsim, we will reference
    //   the bit positions as LCD.seg0, LCD.seg1, ..., where LCD is the
    //   user-assigned name of the 7-segment LCD

    std::string seg = "seg";
    std::string com = "com";
    int i;
    char ch;
    for (ch = '0', i = 0; i < NUM_SEGS; i++, ch++)
    {
        m_seg_pins[i] = new Lcd_Input(seg + ch, this);
        addSymbol(m_seg_pins[i]);
        assign_pin(i + NUM_COMS+1, m_seg_pins[i]);
        if (i < NUM_COMS)
        {
            m_com_pins[i] = new Lcd_Input(com + ch, this);
            addSymbol(m_com_pins[i]);
            assign_pin(i + 1, m_com_pins[i]);
        }
    }
}

void Lcd_7Segments::set_lmux(gint64 _lmux)
{
    if (Lmux != _lmux)
    {
        Dprintf(("%s Lmux=%ld _lmux=%ld future_cycle=%ld\n", name().c_str(), Lmux, _lmux, future_cycle));
        if (future_cycle)
        {
            get_cycles().clear_break(future_cycle);
            future_cycle = 0;
        }
        if (m_com_pins[0])
            m_com_pins[0]->clear_last_time();
        delta_t = 0;
    }
    Lmux = _lmux;
    num_phases = 2 * (Lmux + 1);
}

void Lcd_7Segments::set_clock(unsigned int dt)
{
    delta_t = dt;
    Dprintf(("%s dt=%d old_phase=%d old_future_cycle=%ld\n", name().c_str(), dt, phase, future_cycle));
    if (future_cycle)
        get_cycles().clear_break(future_cycle);

    future_cycle = get_cycles().get() + delta_t/(2 * num_phases);
    get_cycles().set_break(future_cycle, this);
    phase = 0;
}
// Each display segment is controlled by a SEG pin and a COM pin
// which depends on the number of COM pins as determined from LMUX.
// This table gives the SEG pin and COM pin for each display segment
// for each value of LMUX.
struct {int seg; unsigned int com;} muxes[4][NUM_ELEMENTS] =
{
    {{0, 0}, {1, 0}, {2, 0}, {3,0}, {4,0}, {5,0}, {6,0}},   //Lmux=0 1/1
    {{0, 1}, {0, 0}, {1, 0}, {2,0}, {2,1}, {3,1}, {1,1}},   //Lmux=1 1/2
    {{1, 2}, {0, 1}, {0, 0}, {1,0}, {2,1}, {2,2}, {1,1}},   //Lmux=2 1/3
    {{1, 3}, {0, 3}, {0, 1}, {0,0}, {1,1}, {1,2}, {0,2}},   //Lmux=3 1/4
};
// called from callback to process all devices IO pins
void Lcd_7Segments::lmux_compute()
{

    // if TypeB is false or Lmux is zero we have Type A then
    //     phase 0,1 is com0, 2,3 com1 so n,n+1 is com int(n/2)
    // otherwise
    //     the first Lmux+1 phases are com0 the next Lmux+1 phases are com1 etc
    unsigned int com_slot = (TypeB && Lmux)? (phase % (Lmux+1)) :phase/2;

    for(int i = 0; i < NUM_ELEMENTS; i++)
    {
        if (muxes[Lmux][i].com == com_slot)
        {
            double seg_v = m_seg_pins[muxes[Lmux][i].seg]->get_nodeVoltage();
            double com_v = m_com_pins[muxes[Lmux][i].com]->get_nodeVoltage();
            double delta_v = com_v - seg_v;
            Dprintf(("%s el%c seg%d com%d phase=%d delta_v=%5.2f %5.2f-%5.2f\n",
                     name().c_str(), 'a' + i, muxes[Lmux][i].seg,
                     muxes[Lmux][i].com, phase, delta_v, com_v, seg_v));
            if (TypeB && Lmux)
            {
                if (phase < num_phases/2)
                {
                    dc_V[i] = delta_v;
                }
                else
                {
                    if ((dc_V[i] + delta_v) > 0.2)
                    {
                        fprintf(stdout, "*** FAILED LCD DC Voltage %s el%c seg%d com%d Phase%d=%.2fV Phase%d=%.2fV num_phase=%d\n",
                            name().c_str(), 'a'+i, muxes[Lmux][i].seg, muxes[Lmux][i].com,
                            phase, delta_v, phase-num_phases/2, dc_V[i], num_phases);
                        fprintf(stdout, "\t Check CPU-LCD Module for missing connection of LCD registers such as LCDSE\n");

                        fflush(stdout);
                        // If debugging halt execution at next GUI update,
                        // otherwise halt program. This issue is likely missing connection
                        // between CPU and module or bad LCD register setup such as LCDSE.
#if DEBUG > 0
                        bp.halt();
#else
                        exit_gpsim(1);
#endif
                    }
                    deltaV[i] = fabs(dc_V[i]);
                    if (deltaV[i] > temp_maxV) temp_maxV = deltaV[i];
                }
            }
            else
            {
                if (phase % 2 == 0)
                {
                    dc_V[i] = delta_v;
                }
                else if (phase % 2  == 1)
                {
                    if ((dc_V[i] + delta_v) > 0.2)
                    {
                        fprintf(stdout, "*** FAILED LCD DC Voltage %s el%c seg%d com%d Phase%d=%.2fV Phase%d=%.2fV num_phase=%d\n",
                                name().c_str(), 'a'+i, muxes[Lmux][i].seg, muxes[Lmux][i].com,
                                phase, delta_v, phase-1, dc_V[i], num_phases);
                        fprintf(stdout, "\t Check CPU-LCD Module for missing connection of LCD registers such as LCDSE\n");

                        fflush(stdout);
                        // If debugging halt execution at next GUI update,
                        // otherwise halt program. This issue is likely missing connection
                        // between CPU and module or bad LCD register setup such as LCDSE.
#if DEBUG > 0
                        bp.halt();
#else
                        exit_gpsim(1);
#endif
                    }
                    deltaV[i] = fabs(dc_V[i]);
                    if (deltaV[i] > temp_maxV) temp_maxV = deltaV[i];
                }
            }
        }
    }
    if (++phase == num_phases)
    {
        phase = 0;
        phase_maxV = temp_maxV;
        temp_maxV = 0.;
    }
}


//--------------------------------------------------------------
// construct

Module * Lcd_7Segments::construct(const char *_new_name = nullptr)
{
    return new Lcd_7Segments(_new_name);
}

}
