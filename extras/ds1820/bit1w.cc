/*  Copyright (C) 2012 Eduard Timotei Budulea
    Copyright (C) 2013 Roy R. Rankin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "config.h"
#include "bit1w.h"

// 1-wire bus timing parameters
#define tRSTL   0.000480
#define tRSTH   0.000480
#define tPDHIGH 0.000050
#define tPDLOW  0.000200
#define tSLOT   0.000060
#define tRDV    0.000015

// Read sampling must be done within the slot time, typically halfway through.
#define tREAD   (tSLOT / 2)
// To write a 0, the line must be kept low between tRDV and tSLOT.
#define tWRITE0 ((tRDV + tSLOT) / 2)
// When writing a 1, the line must go high within tRDV. Allow for a margin.
#define tWRITE1 (tRDV * 2)

static bool debug = false;

LowLevel1W::LowLevel1W(const char *name, const char *desc):
    Module(name, desc), cycleMark(0), lastValue(true), 
    lastTimeout(false),
    state(&LowLevel1W::idle), ignoreCallback(false), bit_break(0)
{
    pin = new Pin1W("pin", *this);
    addSymbol(pin);
    create_pkg(1);
    assign_pin(1, pin);
    pin->putState(false);
    pin->update_direction(IOPIN::DIR_INPUT, true);
    change(true);
}
LowLevel1W::~LowLevel1W()
{
    removeSymbol(pin);
}

void LowLevel1W::callback() {
    change(false);
}

void LowLevel1W::change(bool pinChange) {
    if (ignoreCallback) return;
    guint64 cycle = get_cycles().get();
    bool bitValue = false;
    switch(pin->getBitChar()) {
    case 'Z':
    case '1':
    case 'x':
    case 'W':
        bitValue = true;
    }
    bool isTimeout = cycle >= cycleMark;


    if ((lastValue != bitValue || lastTimeout != isTimeout) && debug) {
        cout << name() << " +++change state: line = " << bitValue << ", timeout = " << isTimeout << "; time = " << hex << cycle << ", mark = " << cycleMark << endl;
    }
    lastValue = bitValue;
    lastTimeout = isTimeout;
    ignoreCallback = true;
    (this->*state)(bitValue, isTimeout);
    ignoreCallback = false;
    if (cycleMark > cycle) {
        if (!pinChange && bit_break >= cycle) {
            get_cycles().clear_break(bit_break);
        }
	if(cycleMark != bit_break)
            get_cycles().set_break(cycleMark, this);
	if(debug)
	    printf("%s now %" PRINTF_GINT64_MODIFIER "x next break  %" PRINTF_GINT64_MODIFIER "x last break %" PRINTF_GINT64_MODIFIER "x delta(usec) %.1f\n", name().c_str(), cycle, cycleMark, bit_break, (cycleMark-cycle)*4./20.);
	bit_break = cycleMark;
    }
}

void LowLevel1W::idle(bool input, bool isTimeout) {
    if(debug && !isTimeout)
      cout << name() << " idle input=" << input << " timeout=" << isTimeout << endl;
    if (input) return;
    switch(gotBitStart()) {
    case WRITE1:
	if(verbose) cout << name() << " ===write1" << endl;
        // Nothing to do. Only make sure the line doesn't stay low for too long
        state = &LowLevel1W::inWritting1;
        cycleMark = get_cycles().get(tWRITE1);
        return;

    case WRITE0:
        if (verbose) cout << name() << " ===write0" << endl;
        // Pull the line low for a period between tRDV and tSLOT
        state = &LowLevel1W::inWritting0;
        cycleMark = get_cycles().get(tWRITE0);
        pin->update_direction(IOPIN::DIR_OUTPUT, true);
        return;

    case READ:
        if(verbose) cout << name() << " ===read" << endl;
        // Sample the line in the middle of the slot
        state = &LowLevel1W::inReading;
        cycleMark = get_cycles().get(tREAD);
        return;

    case RESET:
        if(verbose) cout << name() << " ===expect reset" << endl;
        // Check for a reset pulse
        state = &LowLevel1W::inResetPulse;
        cycleMark = get_cycles().get(tRSTL);
        return;

    case IDLE:
        state = &LowLevel1W::idle;
	return;
    }
}

void LowLevel1W::inResetPulse(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (input) {
        // Reset pulse too short
        state = &LowLevel1W::idle;
        return;
    }
    if (!isTimeout) return;
    // Reset pulse detected, wait for it to end
    state = &LowLevel1W::endResetPulse;
}

void LowLevel1W::endResetPulse(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!input) return;
    // Reset pulse seen. Wait a while before sending the presence pulse.
    gotReset();
    state = &LowLevel1W::inPresencePulse;
    cycleMark = get_cycles().get(tPDHIGH);
}

void LowLevel1W::inPresencePulse(bool input, bool isTimeout) {
    if(debug)
    cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!isTimeout) return;
    // Pull the line low to announce our presence
    state = &LowLevel1W::endPresencePulse;
    pin->update_direction(IOPIN::DIR_OUTPUT, true);
    cycleMark = get_cycles().get(tPDLOW);
}

void LowLevel1W::endPresencePulse(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!isTimeout) return;
    // Our presence pulse is done. Wait for other devices to also finish.
    pin->update_direction(IOPIN::DIR_INPUT, true);
    state = &LowLevel1W::waitIdle;
    cycleMark = get_cycles().get(tRSTH - tPDHIGH - tPDLOW);
}

void LowLevel1W::waitIdle(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!input) return;
    state = &LowLevel1W::idle;
}

void LowLevel1W::inWritting0(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!isTimeout) return;
    // Finished writing a 0. Make sure the line goes back to idle.
    state = &LowLevel1W::finalizeBit;
    pin->update_direction(IOPIN::DIR_INPUT, true);
    cycleMark = get_cycles().get(tSLOT * 2 - tWRITE0);
}

void LowLevel1W::inWritting1(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (!isTimeout) return;
    if(!input) // OOPS either a reset or bus collision
    {
        state = &LowLevel1W::finalizeBit;
        cycleMark = get_cycles().get(tSLOT * 2 - tWRITE1);
	return;
    }
    state = &LowLevel1W::idle;
    if (bit_remaining() == 0)  gotBitStart();
}

void LowLevel1W::inReading(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (input) {
        readBit(true);
        state = &LowLevel1W::idle;
        if (bit_remaining() == 0)  gotBitStart();
        return;
    }
    if (!isTimeout) return;
    readBit(false);
    state = &LowLevel1W::finalizeBit;
    // assume read < 120 us (30 + 90)
    cycleMark = get_cycles().get(tSLOT * 2 - tREAD);
}

// Wait for a 1 then go to idle
void LowLevel1W::finalizeBit(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << " input=" << input << " timeout=" << isTimeout << endl;
    if (input) {
        state = &LowLevel1W::idle;
	if (bit_remaining() == 0)  gotBitStart();
        return;
    }
    if (!isTimeout) return;
    // The line is low for longer than expected.
    state = &LowLevel1W::inResetPulse;
    cycleMark = get_cycles().get(tRSTL - 2 * tSLOT);
}
