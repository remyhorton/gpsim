/*
   Copyright (C) 2017   Roy R Rankin

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

#ifndef SRC_LCD_H_
#define SRC_LCD_H_

#include <glib.h>

#include "registers.h"
#include "trigger.h"
class InterruptSource;
class LCD_MODULE;
class PinModule;
class Processor;
class T1CON;

// LCDCON - LIQUID CRYSTAL DISPLAY CONTROL REGISTER

class LCDCON : public sfr_register
{
public:
  enum
  {
      LMUX0  = 1 << 0,	//LMUX<1:0> Commons Select bits
      LMUX1  = 1 << 1,
      CS0    = 1 << 2,	//CS<1:0> Clock Source Select bits
      CS1    = 1 << 3,
      VLCDEN = 1 << 4,	// LCD Bias Voltage Pins Enable bit
      WERR   = 1 << 5, // LCD Write Failed Error bit
      SLPEN  = 1 << 6,	// LCD Driver Enable in Sleep mode bit
      LCDEN  = 1 << 7	// LCD Driver Enable bit
  };

  LCDCON(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;

  LCD_MODULE *lcd_module;
  unsigned int mask_writeable = 0xdf;
};

// LCDPS - LCD PRESCALER SELECT REGISTER
class LCDPS : public sfr_register
{
public:

  enum
  {
    LP0    = 1 << 0,	//LP<3:0>: LCD Prescaler Select bits
    LP1    = 1 << 1,
    LP2    = 1 << 2,
    LP3    = 1 << 3,
    WA     = 1 << 4,	// LCD Write Allow Status bit
    LCDA   = 1 << 5,	// LCD Active Status bit
    BIASMD = 1 << 6,	// Bias Mode Select bit
    WFT    = 1 << 7, 	// Waveform Type Select bit

    LPMASK = (LP0 | LP1 | LP2 | LP3)
  };

  LCDPS(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *, unsigned int);

  void put(unsigned int new_value) override;
  LCD_MODULE *lcd_module;
  unsigned int mask_writeable = 0xcf;
};

// LCDSEn - LCD SEGMENT REGISTERS

class LCDSEn : public sfr_register
{
public:
  LCDSEn(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *, unsigned int _n);

  void put(unsigned int new_value) override;

  LCD_MODULE *lcd_module;
  unsigned int n;
};

// LCDDATAx - LCD DATA REGISTERS
class LCDDATAx : public sfr_register
{
public:
  LCDDATAx(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *, unsigned int _n);

  void put(unsigned int new_value) override;
  // bypass put for Power On Reset so WERR flag not set
  void putRV(RegisterValue rv) override
  {
    value.init = rv.init;
    value.put(rv.data);
  }

    LCD_MODULE *lcd_module;
    unsigned int n;
};
//LCD REFERENCE VOLTAGE CONTROL REGISTER
class LCDREF : public sfr_register
{
public:
    enum LCDREF_bits
    {
        VLCD1PE =       1 << 1, //VLCD1 Pin Enable bit
        VLCD2PE =       1 << 2, //VLCD2 Pin Enable bit
        VLCD3PE =       1 << 3, //VLCD3 Pin Enable bit
        LCDIRI =        1 << 5, //LCD Internal Reference Ladder Idle Enable bit
        LCDIRS =        1 << 6, //LCD Internal Reference Source bit
        LCDIRE =        1 << 7,	//LCD Internal Reference Enable bit
    };

    LCDREF(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);
    ~LCDREF();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int mask_writeable = 0xee;
protected:
    LCD_MODULE *lcd_module;
};

//LCD REFERENCE LADDER CONTROL REGISTERS
class LCDRL : public sfr_register
{
public:
    enum LCDRL_bits
    {
        LRLAT0 =        1 << 0,  //Ladder A Time interval control bits
        LRLAT1 =        1 << 1,  //	"
        LRLAT2 =        1 << 2,  //	"
        LRLBP0 =        1 << 4,  //Ladder B Time Power Control bits
        LRLBP1 =        1 << 5,  //	"
        LRLAP0 =        1 << 6,  //Ladder A Time Power Control bits
        LRLAP1 =        1 << 7,  //	"
    };

    LCDRL(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);
    ~LCDRL();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

    unsigned int mask_writeable = 0xf7;
protected:
    LCD_MODULE *lcd_module;
};

class LCDCST : public sfr_register
{
public:
    enum LCDCST_bits
    {
        LCDCST0 =       1 << 0,
        LCDCST1 =       1 << 1,
        LCDCST2 =       1 << 2,
    };

    LCDCST(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);
    ~LCDCST();

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;

    unsigned int mask_writeable = 0x07;
protected:
    LCD_MODULE *lcd_module;
};


class LCD_MODULE: public TriggerObject
{
public:
    LCD_MODULE(Processor *pCpu, bool full_reg_set);
    LCD_MODULE(const LCD_MODULE &) = delete;
    LCD_MODULE& operator =(const LCD_MODULE &) = delete;
    ~LCD_MODULE();

    // define external bias pins
    void set_Vlcd(PinModule *, PinModule *, PinModule *);
    // define COM pins
    void set_LCDcom(PinModule *, PinModule *, PinModule *, PinModule *);
    // Set SEG pins up to four at a time
    void set_LCDsegn(unsigned int, PinModule *, PinModule *, PinModule *, PinModule *);
    void set_t1con(T1CON *t1c) {t1con = t1c;}
    void lcd_on_off(bool lcdOn);
    void set_bias(unsigned int lmux);
    void set_vlcd();
    void lcd_set_com(bool lcdOn, unsigned int lmux);
    void lcd_set_segPins(unsigned int regno, unsigned int old, unsigned int diff);
    void clear_bias();
    void set_lcdcon_werr() { lcdcon->value.put(lcdcon->value.get() | LCDCON::WERR); };
    bool get_lcdps_wa();
    bool get_lcdcon_lcden() { return lcdcon->value.get() & LCDCON::LCDEN;}
    bool typeB() {return ((lcdps->value.get() & LCDPS::WFT) && mux_now);}
    void callback() override;
    virtual void setIntSrc(InterruptSource *_IntSrc) { IntSrc = _IntSrc;}
    void vlcd_pins_toggle(unsigned char mask);
    void start_clock();
    void stop_clock();
    void drive_lcd();
    void save_hold_data();
    void start_typeA();
    void start_typeB();
    virtual void sleep();
    virtual void wake();

    Processor 		*cpu;
    InterruptSource 	*IntSrc = nullptr;
    bool		Vlcd1_on = false, Vlcd2_on = false, Vlcd3_on = false;
    bool		is_sleeping = false;
    double		vlcd[4]; // Bias voltages
    PinModule 		*Vlcd1 = nullptr, *Vlcd2 = nullptr, *Vlcd3 = nullptr;
    PinModule		*LCDsegn[24];	//Segment pins
    PinModule		*LCDcom[4];	//COM pins
    unsigned char	LCDsegDirection[3];
    unsigned char	LCDcomDirection = 0;
    unsigned char	hold_data[12];
    unsigned char	bias_now = 0;
    unsigned char	mux_now = 0;
    unsigned char	phase = 0;
    unsigned char	num_phases = 0;
    unsigned char	contrast_cst = 0;
    unsigned int 	clock_tick = 0;
    guint64 		future_cycle = 0;
    guint64		map_com[4];
    guint64		map_on;
    guint64		map_off;

    LCDCON	*lcdcon;
    LCDPS	*lcdps;
    LCDREF	*lcdref = nullptr;
    LCDRL	*lcdrl = nullptr;
    LCDCST	*lcdcst = nullptr;
    LCDSEn   	*lcdSEn[3];
    LCDDATAx 	*lcddatax[12];
    T1CON	*t1con = nullptr;
};

#endif // SRC_LCD_H_
