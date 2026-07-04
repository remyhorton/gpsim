/*
   Copyright (C) 2017,2025	Roy R Rankin

This file is part of the libgpsim library of gpsim

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

#include "lcd_module.h"

#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <string>

#include "14bit-tmrs.h"
#include "gpsim_time.h"
#include "ioports.h"
#include "pir.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


LCD_MODULE::LCD_MODULE(Processor *pCpu, bool full_reg_set)
   : cpu(pCpu)
{
    char lcdsex[] = "lcdsex";
    char lcddataX[10];
    int i;

    lcdcon = new LCDCON(pCpu, "lcdcon", "LCD control register", this);
    lcdps = new LCDPS(pCpu, "lcdps", "LCD prescaler select register", this, 0xcf);
    lcdrl = new LCDRL(pCpu, "lcdrl", "LCD REFERENCE LADDER CONTROL REGISTER", this);
    lcdref = new LCDREF(pCpu, "lcdref", "LCD REFERENCE VOLTAGE CONTROL REGISTER", this);
    lcdcst = new LCDCST(pCpu, "lcdcst", "LCD CONTRAST CONTROL REGISTER", this);
    for (i = 0; i < 3 ; i++)
    {
        lcdsex[5] = '0' + i;
        if ( i < 2 || full_reg_set)
            lcdSEn[i] = new LCDSEn(pCpu, lcdsex, "LCD Segment register", this, i);
        else
            lcdSEn[i] = nullptr;
    }
    printf("\n");
    for (i = 0; i < 12; i++)
    {
        snprintf(lcddataX, sizeof(lcddataX), "lcddata%d", i);
        if (((i + 1) % 3 != 0) || full_reg_set)
        {
            lcddatax[i] = new LCDDATAx(pCpu, lcddataX, "LCD Data register", this, i);
        }
        else
        {
            lcddatax[i] = nullptr;
        }
    }

    std::fill_n(LCDsegDirection, 3, '\0');
    std::fill_n(hold_data, 12, '\0');
    std::fill_n(LCDsegn, 24, nullptr);
    std::fill_n(LCDcom, 4, nullptr);
}
LCD_MODULE::~LCD_MODULE()
{
    delete lcdcon;
    delete lcdps;
    delete lcdcst;
    delete lcdref;
    delete lcdrl;
    for(int i = 0; i < 3; i++)
	if (lcdSEn[i]) delete lcdSEn[i];
    for(int i = 0; i < 12; i++)
	if (lcddatax[i]) delete lcddatax[i];
}
// set LCD bias pins
void LCD_MODULE::set_Vlcd(PinModule *_Vlcd1, PinModule *_Vlcd2, PinModule *_Vlcd3)
{
    Vlcd1 = _Vlcd1;
    Vlcd2 = _Vlcd2;
    Vlcd3 = _Vlcd3;
}


// set LCD common pins
void LCD_MODULE::set_LCDcom(PinModule *c0, PinModule *c1, PinModule *c2, PinModule *c3)
{
    LCDcom[0] = c0;
    LCDcom[1] = c1;
    LCDcom[2] = c2;
    LCDcom[3] = c3;
}

// set 4 LCD segment pins (at a time)
void LCD_MODULE::set_LCDsegn(unsigned int i, PinModule *c0, PinModule *c1, PinModule *c2, PinModule *c3)
{
    assert(i <= 20);
    LCDsegn[i]     = c0;
    LCDsegn[i + 1] = c1;
    LCDsegn[i + 2] = c2;
    LCDsegn[i + 3] = c3;
}

void LCD_MODULE::vlcd_pins_toggle(unsigned char mask)
{

    if (mask & 1<<0)
    {
	if(!Vlcd1_on)
	{
	    Vlcd1->AnalogReq(lcdps, true, "vlcd1");
            Vlcd1_on = true;
	}
    }
    else if (Vlcd1_on)
    {
        Vlcd1->AnalogReq(lcdps, false, Vlcd1->getPin().name().c_str());
        Vlcd1_on = false;
    }
    if (mask & 1<<1)
    {
	if(!Vlcd2_on)
	{
	    Vlcd2->AnalogReq(lcdps, true, "vlcd2");
            Vlcd2_on = true;
	}
    }
    else if (Vlcd2_on)
    {
        Vlcd2->AnalogReq(lcdps, false, Vlcd2->getPin().name().c_str());
        Vlcd2_on = false;
    }
    if (mask & 1<<2)
    {
	if(!Vlcd3_on)
	{
	    Vlcd3->AnalogReq(lcdps, true, "vlcd3");
            Vlcd3_on = true;
	}
    }
    else if (Vlcd3_on)
    {
        Vlcd3->AnalogReq(lcdps, false, Vlcd3->getPin().name().c_str());
        Vlcd3_on = false;
    }
}


void LCD_MODULE::clear_bias()
{
    Dprintf(("LCD_MODULE::clear_bias()\n"));
    bias_now = 0;

    vlcd_pins_toggle(0);

}

void LCD_MODULE::set_bias(unsigned int lmux)
{
    bool biasmode = (lcdps->value.get() & LCDPS::BIASMD);
    unsigned char bias = 0;

    switch (lmux)
    {
    case 0:
        bias = 1;
        break;

    case 1:
        bias = biasmode ? 2 : 3;
        break;

    case 2:
        bias = biasmode ? 2 : 3;
        break;

    case 3:
        bias = 3;
        break;
    }
    if (bias == bias_now)
        return;

    if (lcdcon->value.get() & LCDCON::VLCDEN)
    {
        unsigned char mask;
	if (bias == 1)
	    mask = 4;	//Vlcd3 only
	else
	    mask = 7;	//Vlcd3,2,1
	vlcd_pins_toggle(mask);
    }
    bias_now = bias;
}


bool LCD_MODULE::get_lcdps_wa() 
{ 
    bool ret;
    ret = (!(bool)(lcdcon->value.get() & LCDCON::LCDEN) && future_cycle == 0) ||
	    	((lcdps->value.get() & LCDPS::WA) );
    return ret;
}
void LCD_MODULE::lcd_on_off(bool lcdOn)
{
    if (lcdOn)
    {
        for (unsigned int i = 0; i < 3; i++)
        {
            if (lcdSEn[i])
                lcd_set_segPins(i, lcdSEn[i]->value.get(), lcdSEn[i]->value.get() ^ 0);
        }
        lcd_set_com(lcdOn, lcdcon->value.get() & (LCDCON::LMUX0| LCDCON::LMUX1));
        start_clock();
    }
}


void LCD_MODULE::lcd_set_com(bool lcdOn, unsigned int lmux)
{
    unsigned int i;

    Dprintf(("LCD_MODULE::lcd_set_com on %d lmux %u\n", lcdOn, lmux));
    if (lcdOn)
    {
        for(i = 0; i < 4; i++)
        {
            mux_now = lmux;
            if (i <= lmux)
            {
                char name[5];
                snprintf(name, sizeof(name), "COM%u", i);
                LCDcom[i]->getPin().newGUIname(name);
                if (LCDcom[i]->getPin().get_direction())
                    LCDcomDirection |= (1 << i);
                else
                    LCDcomDirection &= ~(1 << i);
                LCDcom[i]->getPin().update_direction(1,true);
            }
            else
            {
                LCDcom[i]->getPin().newGUIname(LCDcom[i]->getPin().name().c_str());
                LCDcom[i]->getPin().update_direction(LCDcomDirection & (1 << i), true);
            }
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            LCDcom[i]->getPin().newGUIname(LCDcom[i]->getPin().name().c_str());
            LCDcom[i]->getPin().update_direction(LCDcomDirection & (1 << i), true);
        }
    }
}

void LCD_MODULE::sleep()
{
    unsigned int con_reg = lcdcon->value.get();
    Dprintf(("LCD_MODULE::sleep()\n"));
    if (!(lcdps->value.get() & LCDPS::LCDA))
        return;

    // Stop during sleep
    if ((con_reg & LCDCON::SLPEN) || !(con_reg & (LCDCON::CS0 | LCDCON::CS1)))
    {
        Dprintf(("LCD_MODULE::sleep() stop during sleep fc=%" G_GINT64_MODIFIER "d now=%" G_GINT64_MODIFIER "d\n", future_cycle, get_cycles().get()));
        if (future_cycle >= get_cycles().get())
        {
            get_cycles().clear_break(future_cycle);
            future_cycle = 0;
            phase = 0;
        }
        is_sleeping = true;
        // Set all LCD outputs to zero
        for (int l = 0; l <= mux_now; l++) // scan across com related output
        {
            LCDcom[l]->getPin().putState(0.0);
        }
        for (int k = 0; (k < 3) && lcdSEn[k]; k++)
        {
            unsigned int enable = lcdSEn[k]->value.get();
            if (enable)
            {
                for (int i = 0; i < 8; i++)
                {
                    if (enable & (1 << i))
                        LCDsegn[i]->getPin().putState(0.0);
                }
            }
        }
    }
}

void LCD_MODULE::wake()
{
    unsigned int con_reg = lcdcon->value.get();
    if (!(lcdps->value.get() & LCDPS::LCDA) || !is_sleeping)
        return;

    is_sleeping = false;

    Dprintf(("LCD_MODULE::wake() fc=%" G_GINT64_MODIFIER "d\n", future_cycle));
    // Stop during sleep
    if ((con_reg & LCDCON::SLPEN) || !(con_reg & (LCDCON::CS0 | LCDCON::CS1)))
    {
        Dprintf(("LCD_MODULE::wake() restart after  sleep\n"));
        start_clock();
    }
}

void LCD_MODULE::lcd_set_segPins(unsigned int regno, unsigned int new_value, unsigned int diff)
{
    unsigned char *pt = &LCDsegDirection[regno];

    for (int i = 0; i < 8; i++)
    {
        unsigned int mask = 1 << i;
        PinModule *port = LCDsegn[regno * 8 + i];
        if (diff & mask)
        {
            if (new_value & mask)
            {
                char name[6];
                snprintf(name, sizeof(name), "SEG%u", regno * 8 + i);

                if (port->getPin().get_direction())
                    *pt |= mask;
                else
                    *pt &= ~mask;

                port->getPin().newGUIname((const char *)name);
                port->getPin().update_direction(1, true);
            }
            else
            {
                port->getPin().update_direction(*pt&mask, true);
                port->getPin().newGUIname(port->getPin().name().c_str());
            }
        }
    }
}


void LCD_MODULE::start_clock()
{
    unsigned int prescale = (lcdps->value.get() & (LCDPS::LPMASK)) + 1;
    unsigned int clock_source, frame_rate;
    double freq;

    clock_source = 0;
    Dprintf(("LCD_MODULE::start_clock() mux_now %x lmux %x\n", mux_now, lcdcon->value.get() & 0x3));

    switch((lcdcon->value.get() & (LCDCON::CS0 | LCDCON::CS1)) >> 2)
    {
    case 0:	// Fosc/8102 or instruction/sec / 2048;
        clock_source = 2048;
        break;

    case 1:	//T1OSC(32kHz) (Timer1)/32
        freq = t1con->t1osc();
        if (freq > 1.0)
            clock_source = get_cycles().instruction_cps() * 32 / freq;
        else
        {
            fprintf(stderr, "LCD_MODULE::start_clock() t1osc not enabled\n");
            return;
        }
        break;

    case 2:	//LFINTOSC (31 kHz) /32
    case 3:
        clock_source = get_cycles().instruction_cps() * 32 / 31e3;
        Dprintf(("LFINTOSC %u \n", clock_source));
        break;
    }
    if (mux_now != 3)
        frame_rate = clock_source * (4 * prescale);
    else
        frame_rate = clock_source * (3 * prescale);

    num_phases = 2 * (mux_now + 1);
    phase = 0;

    if (typeB()) // Type B wave form
    {
        clock_tick = frame_rate / (mux_now + 1);
        start_typeB();
    }
    else
    {
        clock_tick = frame_rate / num_phases;
        start_typeA();
    }
    Dprintf(("frame rate %u clock_tick %u %.1f\n", frame_rate, clock_tick, get_cycles().instruction_cps() / frame_rate));
    if (future_cycle >= get_cycles().get())
    {
        get_cycles().clear_break(future_cycle);
        future_cycle = 0;
    }
    save_hold_data();
    lcdps->value.put(lcdps->value.get() | LCDPS::LCDA);
    //if ((lcdps->value.get() & LCDPS::WFT) == 0)
    if (typeB() == 0)
    {
        lcdps->value.put(lcdps->value.get() | LCDPS::WA);
    }
    callback();
}

void LCD_MODULE::start_typeA()
{
    switch (mux_now)
    {
    case 0:		// static
        map_com[0] = 003;
        map_on     = 030;
        map_off    = 003;
        break;

    case 1:		// mux 1/2
        map_com[0] = 00321;
        map_com[1] = 02103;
        map_on     = 03030;
        map_off    = 01212;
        break;

    case 2:		// mux 1/3
        map_com[0] = 0032121;
        map_com[1] = 0210321;
        map_com[2] = 0212103;
        map_on     = 0303030;
        map_off    = 0121212;
        break;

    case 3:		// mux 1/4
        map_com[0] = 003212121;
        map_com[1] = 021032121;
        map_com[2] = 021210321;
        map_com[3] = 021212103;
        map_on     = 030303030;
        map_off    = 012121212;
        break;
    }
}

void LCD_MODULE::start_typeB()
{
    switch (mux_now)
    {
    case 0:		// static - use type A for this
        break;

    case 1:		// 1/2
        map_com[0] = 00231;
        map_com[1] = 02013;
        map_on     = 03300;
        map_off    = 01122;
        break;

    case 2:		// 1/3
        map_com[0] = 0022311;	
        map_com[1] = 0202131;
        map_com[2] = 0220113;
        map_on     = 0333000;
        map_off    = 0111222;
        break;

    case 3:		// 1/4
        map_com[0] = 002223111;
        map_com[1] = 020221311;
        map_com[2] = 022021131;
        map_com[3] = 022201113;
        map_on     = 033330000;
        map_off    = 011112222;
        break;
    }
}

// shutdown LCD
void LCD_MODULE::stop_clock()
{
    for (int i = 0; i < 3; i++)
    {
        if (lcdSEn[i])
            lcd_set_segPins(i, 0, lcdSEn[i]->value.get());
    }
    lcd_set_com(false, lcdcon->value.get() & (LCDCON::LMUX0 | LCDCON::LMUX1));

    if (lcdps->value.get() & LCDPS::LCDA)
    {
        IntSrc->Trigger();
        lcdps->value.put(lcdps->value.get() & ~LCDPS::LCDA);
    }
    lcdps->value.put(lcdps->value.get() | LCDPS::WA);
	    
}

void LCD_MODULE::callback()
{
   Dprintf(("LCD_MODULE::callback() %" G_GINT64_MODIFIER "d phase=%d bias_now=%d\n", future_cycle, phase,  bias_now));

    future_cycle = get_cycles().get() + clock_tick;
    drive_lcd();

    if (typeB() && (phase == (mux_now + 1)))
    {
        IntSrc->Trigger();
        lcdps->value.put(lcdps->value.get() | LCDPS::WA);
    }
    phase++;
    if (phase == num_phases)
    {
        phase = 0;
        save_hold_data();
        if (typeB())
	{
            lcdps->value.put(lcdps->value.get() & ~LCDPS::WA);
	}
        if (!(lcdcon->value.get() & LCDCON::LCDEN))
	{
            stop_clock();
	    future_cycle = 0;
	    return;
	}
    }
    if (lcdps->value.get() & LCDPS::LCDA)
    {
        get_cycles().set_break(future_cycle, this);
    }
}

void LCD_MODULE::save_hold_data()
{
    for (int i = 0; i < 12; i++)
    {
        if (lcddatax[i])
            hold_data[i] = lcddatax[i]->value.get();
    }
}

void LCD_MODULE::set_vlcd()
{
    double bias_top;
    // using internal reference ladder?
    if (lcdref && ((lcdref->get_value()) & LCDREF::LCDIRE))
    {
	if (((lcdref->get_value()) & LCDREF::LCDIRS))
	    bias_top = 3.072;	// DERIVED FROM Fixed Voltage Reference
	else
	    bias_top = cpu->get_Vdd();
	// contrast latter reduces bias top by up to 10%
	// FIXME should be changed for power mode
	bias_top -= bias_top * ( (double)contrast_cst/ 70.);
	vlcd[3] = bias_top;
	if (bias_now == 2)
	    vlcd[2] = vlcd[1] = bias_top/2.;
	else
	{
	    vlcd[2] = bias_top * 0.6667;
	    vlcd[1] = bias_top * 0.3333;
	}
	vlcd[0] = 0.;
    }
    else	// using external reference ladder
    {
        vlcd[0] = 0.;
        vlcd[1] = 0.;
        vlcd[2] = 0.;
	if (Vlcd3_on)
            vlcd[3] = Vlcd3->getPin().get_nodeVoltage();
	else
	{
                fprintf(stderr, "Error VLCD3 must be enabled\n");
		vlcd[3] = 0;
		return;
	}
        if (bias_now == 2)
        {
            if (Vlcd1_on)
                vlcd[1] = vlcd[2] = Vlcd1->getPin().get_nodeVoltage();
            else if (Vlcd2_on)
                vlcd[1] = vlcd[2] = Vlcd2->getPin().get_nodeVoltage();
            else
            {
                fprintf(stderr, "Error bias is 1/2 but neither %s or %s on as bias pin\n", Vlcd1->getPin().name().c_str(), Vlcd2->getPin().name().c_str());
                return;
            }

        }
        else if (bias_now == 3)
        {
            vlcd[1] = Vlcd1->getPin().get_nodeVoltage();
            vlcd[2] = Vlcd2->getPin().get_nodeVoltage();
        }
    }
}

void LCD_MODULE::drive_lcd()
{
    double com_volt[4];
    unsigned int subphase;
    unsigned int shift = 3 * (num_phases - phase - 1);
    guint64 mask = 07 << shift;

    set_vlcd();
    Dprintf(("bias_now=%d mux_now=%d num_phases=%d phase=%d shift=%d mask=%lo\n", bias_now, mux_now, num_phases, phase, shift, mask));

    for(int com_num = 0; com_num <= mux_now; com_num++) // scan across com related output
    {
        unsigned int level= (map_com[com_num] & mask) >> shift;
        com_volt[com_num] = vlcd[level];
        Dprintf(("com%d mask %" G_GINT64_MODIFIER "o level %u %.1f\n", com_num, mask, level, com_volt[com_num]));
        LCDcom[com_num]->getPin().putState(com_volt[com_num]);
    }

    if (typeB())
        subphase = phase % (mux_now + 1);
    else
        subphase =  phase / 2;
    double Von = vlcd[(map_on & mask) >> shift];
    double Voff = vlcd[(map_off & mask) >> shift];
    Dprintf(("typeB=%d phase=%d subphase=%d Von=%.2f Voff=%.2f\n", typeB(), phase, subphase, Von, Voff));
    Dprintf(("phase=%d subphase=%u mask %" G_GINT64_MODIFIER "o Von=%.2f Voff=%.2f\n", phase, subphase, mask, Von, Voff));
    for(int k = 0; (k < 3) && lcdSEn[k]; k++)
    {
        unsigned int lcdSE = lcdSEn[k]->value.get();
        if(lcdSE)
        {
            // subphase packs data into 2 phase SEQ slots for each COMx
            unsigned int data = hold_data[k + 3 * subphase];
            Dprintf(("\t %d com%d phase=%d  lcdSE%d=0x%x data[%d]=0x%x\n", __LINE__, subphase, phase, k, lcdSE, subphase*3 + k, data));
            for(int i = 0; i < 8; i++)
            {
                if (lcdSE & (1 << i))
                {
                    double seg_volt = ((1<<i) & data) ? Von : Voff;
                    Dprintf(("\t\tSEG%d=%.2f com%d=%5.2f delta=%5.2f\n", i+k*8, seg_volt, subphase, com_volt[subphase], com_volt[subphase] - seg_volt));
                    if (LCDsegn[i+k*8])
                        LCDsegn[i+k*8]->getPin().putState(seg_volt);
                    else
                        Dprintf(("LCDsegn[%d] is Zero\n", i+k*8));
                }
            }
        }
    }
}

LCDCON::LCDCON(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module) :
    sfr_register(pCpu, pName, pDesc)
{
    lcd_module = _lcd_module;
}

void LCDCON::put_value(unsigned int new_value)
{
    unsigned int diff = value.get() ^ new_value;
    if (diff == 0) return;
    Dprintf(("LCDCON::put_value new=0x%x old=0x%x \n", new_value, value.get()));
    value.put(new_value);
    if (new_value & (LMUX0 | LMUX1))
	lcd_module->set_bias(new_value & (LMUX0 | LMUX1));
    if (new_value & VLCDEN)
    {
        // Are LCD Bias Voltage Pins Enabled
        if (new_value & VLCDEN)
        {
            lcd_module->set_bias(new_value & (LMUX0 | LMUX1));
        }
	else
        {
            lcd_module->clear_bias();
        }
    }
    // LCD on/off
    if (new_value & LCDEN)
        lcd_module->lcd_on_off(new_value & LCDEN);
}

void LCDCON::put(unsigned int new_value)
{
    new_value &= mask_writeable;
    Dprintf(("LCDCON::put 0x%x\n", new_value));
    trace.raw(write_trace.get() | value.get());

    put_value(new_value);
}


LCDPS::LCDPS(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module, unsigned int bitmask) :
    sfr_register(pCpu, pName, pDesc), lcd_module(_lcd_module),
	mask_writeable(bitmask)
{
}

void LCDPS::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    put_value(new_value & mask_writeable);
}

LCDSEn::LCDSEn(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module, unsigned int _n) :
    sfr_register(pCpu, pName, pDesc)
{
    lcd_module = _lcd_module;
    n = _n;
}

void LCDSEn::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();

    trace.raw(write_trace.get() | value.get());
    put_value(new_value);

    if (lcd_module->get_lcdcon_lcden())
        lcd_module->lcd_set_segPins(n, new_value, diff);
}

LCDDATAx::LCDDATAx(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module, unsigned int _n) :
    sfr_register(pCpu, pName, pDesc)
{
    lcd_module = _lcd_module;
    n =_n;
}

void LCDDATAx::put(unsigned int new_value)
{
    // set error if lcdps:WA not set
    if (!lcd_module->get_lcdps_wa() && lcd_module->mux_now)
    {
        fprintf(stderr, "%s ERROR write with WA == 0\n", name().c_str());
        lcd_module->set_lcdcon_werr();
        return;
    }
    trace.raw(write_trace.get() | value.get());
    put_value(new_value);
}
LCDRL::LCDRL(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module)
    : sfr_register(pCpu, pName, pDesc), lcd_module(_lcd_module)
{
}

LCDRL::~LCDRL()
{
}
void LCDRL::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    put_value(new_value & mask_writeable);
}

void LCDRL::put_value(unsigned int new_value)
{
    value.put(new_value & 0xff);
}
LCDREF::LCDREF(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module)
    : sfr_register(pCpu, pName, pDesc) , lcd_module(_lcd_module)
{
}

LCDREF::~LCDREF()
{
}
void LCDREF::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    put_value(new_value & mask_writeable);
}

void LCDREF::put_value(unsigned int new_value)
{
    unsigned int diff = value.get() ^ new_value;
    value.put(new_value & 0xff);
    if (diff & (VLCD3PE|VLCD2PE|VLCD1PE))
	lcd_module->vlcd_pins_toggle((new_value & (VLCD3PE|VLCD2PE|VLCD1PE)) >> 1);
}

LCDCST::LCDCST(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module)
    : sfr_register(pCpu, pName, pDesc), lcd_module(_lcd_module)
{
}

LCDCST::~LCDCST()
{
}
void LCDCST::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    put_value(new_value & mask_writeable);
}

void LCDCST::put_value(unsigned int new_value)
{
    value.put(new_value & 0xff);
    lcd_module->contrast_cst = new_value;
}

