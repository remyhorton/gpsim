/*
    CD451xB.h - CD4510B & CD4516B logic ICs for gpsim

    Copyright (C) 2025 Remy Horton
    Copyright (C) 2026 Codethink
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MODULES_CD451XB_H
#define MODULES_CD451XB_H

#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define IN_MODULE

#include "src/modules.h"
#include "src/ioports.h"


namespace CD451xB
{

class CD451xB;

/*****************************************************************************
 ** Pin subclasses
 *****************************************************************************/

class ClockPin : public IOPIN
    {
        public:
            ClockPin(CD451xB *chip) : IOPIN("Clock"), cd4516(chip) { }
            void setDrivenState(bool new_state) override;
        private:
            CD451xB *cd4516;
    };

class PresetPin : public IOPIN
    {
        public:
            PresetPin(CD451xB *chip) : IOPIN("Preset"), cd4516(chip) { }
            void setDrivenState(bool new_state) override;
        private:
            CD451xB *cd4516;
    };

class ResetPin : public IOPIN
    {
        public:
            ResetPin(CD451xB *chip) : IOPIN("Reset"), cd4516(chip) { }
            void setDrivenState(bool new_state) override;
        private:
            CD451xB *cd4516;
    };

class UpDownPin : public IOPIN
    {
        public:
            UpDownPin(CD451xB *chip) : IOPIN("UpDown"), cd4516(chip) { }
            void setDrivenState(bool new_state) override;
        private:
            CD451xB *cd4516;
    };

class CtrlPin : public IO_bi_directional
    {
        public:
            CtrlPin(const char *_name, const int dir) : IO_bi_directional(_name)
        {
            this->update_direction(dir, true);
        }
    };

class DataPin : public IO_bi_directional_pu
    {
        public:
            DataPin(CD451xB *chip, const char *name)
                : IO_bi_directional_pu(name), cd4516(chip)
            {
            }
            void setDrivenState(bool new_state) override;
        private:
            CD451xB *cd4516;
    };


/*****************************************************************************
 ** Data port
 ** Allows a set of pins to be treated as a single end-point for convenience.
 *****************************************************************************/

class DataPort : public PortModule
    {
        public:
            DataPort() : PortModule(4)
            {
            }
            unsigned int get(void)
            {
                unsigned int value = 0;

                for(int idxPin = 0; idxPin < 4; idxPin++)
                    if( getPin(idxPin)->getState() )
                        value |= 1 << idxPin;
                return value;
            }
            void put(const unsigned int value)
            {
                for(int idxPin = 0; idxPin < 4; idxPin++)
                {
                    if( value & (1 << idxPin) )
                        getPin(idxPin)->putState(true);
                    else
                        getPin(idxPin)->putState(false);
                }
            }
    };


/*****************************************************************************
 ** Main IC class
 ** This is not used directly so some of the functions that GPsim normally
 ** needs have been omitted.
 *****************************************************************************/

class CD451xB : public Module
    {
        public:
            explicit CD451xB(const char *_name, const uint8_t max_value, const char *desc);

            ~CD451xB()
            {
            }

            void signalClock();
            void signalReset(const bool state);
            void signalPreset(const bool state);
            void signalP();
            void signalDirection(bool state);

            virtual void create_iopin_map();
            virtual void build_window();
            virtual void update();

#ifdef HAVE_GUI
            static gboolean draw_window(GtkWidget*, GdkEvent*, gpointer);
#endif

        protected:
            struct
            {
                DataPort *portP;
                DataPort *portQ;
                GtkWidget *gtkarea;
                ClockPin *pinClock;
                ResetPin *pinReset;
                PresetPin *pinPreset;
                UpDownPin *pinUpDown;
                CtrlPin *pinCarryIn;
                CtrlPin *pinCarryOut;
                DataPin *pinsP[4];
                DataPin *pinsQ[4];
                uint8_t valueMax;
            } m_self;

            void signalLoadPresets();
    };


/*****************************************************************************
 ** Pin subclasses
 *****************************************************************************/

class CD4510B : public CD451xB
    {
        public:
            CD4510B(const char *_name) : CD451xB(_name, 9, "Presettable BCD Up/Down Counter")
            {
            }

            static Module *construct(const char *name)
            {
                std::string attrName = name;
                CD451xB *cd4510 = new CD4510B(name);
                cd4510->create_iopin_map();
                return cd4510;
            }
    };

class CD4516B : public CD451xB
    {
        public:
            CD4516B(const char *_name) : CD451xB(_name, 15, "Presettable Binary Up/Down Counter")
            {
            }

            static Module *construct(const char *name)
            {
                std::string attrName = name;
                CD451xB *cd4516 = new CD4516B(name);
                cd4516->create_iopin_map();
                return cd4516;
            }
    };

}

#endif // MODULES_CD451XB_H
