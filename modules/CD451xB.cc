/*
    CD451xB.cc - CD4510B & CD4516B logic ICs for gpsim

    Copyright (C) 2025 Remy Horton
    Copyright (C) 2026 Codethink
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdio>
#include "config.h"
#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "src/gpsim_time.h"
#include "src/stimuli.h"
#include "src/ioports.h"
#include "src/symbol.h"
#include "src/value.h"
#include "src/packages.h"
#include "src/gpsim_interface.h"
#include "CD451xB.h"


namespace CD451xB
{
    class GtkInterface : public Interface
    {
        private:
            CD451xB *base;

        public:
            virtual void SimulationHasStopped(gpointer object)
            {
                Update(object);
            }
            virtual void Update(gpointer object)
            {
#ifdef HAVE_GUI
                if (this->base)
                    this->base->update();
#endif
            }

            explicit GtkInterface(CD451xB *_cd45)
                : Interface((gpointer *) _cd45), base(_cd45)
            {
            }
    };


/*****************************************************************************
 ** Pin state changes
 ** Forward the stimuli to the state signalling
 *****************************************************************************/

void ClockPin::setDrivenState(bool new_state)
{
    bool old_state = getDrivenState();
    IOPIN::setDrivenState(new_state);
    if( (! old_state) && new_state )
    {
        cd4516->signalClock();
    }
}

void PresetPin::setDrivenState(bool new_state)
{
    bool old_state = getDrivenState();
    IOPIN::setDrivenState(new_state);
    if (old_state != new_state)
        cd4516->signalPreset(new_state);
}

void ResetPin::setDrivenState(bool new_state)
{
    bool old_state = getDrivenState();
    IOPIN::setDrivenState(new_state);
    if (old_state != new_state)
        cd4516->signalReset(new_state);
}

void DataPin::setDrivenState(bool new_state)
{
    IO_bi_directional_pu::setDrivenState(new_state);
    cd4516->signalP();
}

void UpDownPin::setDrivenState(bool new_state)
{
    bool old_state = getDrivenState();
    IOPIN::setDrivenState(new_state);
    if (old_state != new_state)
        cd4516->signalDirection(new_state);
}


/*****************************************************************************
 ** State change signals
 *****************************************************************************/

void CD451xB::signalClock()
{
    Dprintf(("CD451xB::signalClock()\n"));
    if( m_self.pinReset->getDrivenState() )
    {
        Dprintf(("  Reset asserted\n"));
    }
    else if( m_self.pinPreset->getDrivenState() )
    {
        Dprintf(("  Preset asserted\n"));
    }
    else if( m_self.pinCarryIn->getDrivenState() )
    {
        Dprintf(("  Carry in disabled\n"));
    }
    else
    {
        unsigned int value = m_self.portQ->get();
        bool increment = m_self.pinUpDown->getDrivenState();

        if( increment )
        {
            if( value < m_self.valueMax )
            {
                value++;
                if( value == m_self.valueMax )
                    m_self.pinCarryOut->putState(false);
                else
                    m_self.pinCarryOut->putState(true);
            }
            else
            {
                value = 0;
                m_self.pinCarryOut->putState(true);
            }
            m_self.portQ->put(value);
        }
        else
        {
            if( value == 0 )
            {
                value = m_self.valueMax;
                m_self.pinCarryOut->putState(true);
            }
            else
            {
                value--;
                if( value == 0 )
                    m_self.pinCarryOut->putState(false);
                else
                    m_self.pinCarryOut->putState(true);
            }
            m_self.portQ->put(value);
        }
    }
}

void CD451xB::signalReset(const bool state)
{
    Dprintf(("CD451xB::signalReset(%s)\n", state ? "hi" : "lo"));
    for(int idxPin=0; idxPin < 4; idxPin++)
        m_self.pinsQ[idxPin]->putState( false );
    this->signalDirection( m_self.pinUpDown->getDrivenState() );
}

void CD451xB::signalPreset(const bool state)
{
    Dprintf(("CD451xB::signalPreset(%s)\n", state ? "hi" : "lo"));
    if( m_self.pinReset->getDrivenState() )
        return;
    if( state )
        signalLoadPresets();
}

void CD451xB::signalP()
{
    Dprintf(("CD451xB::signalP()\n"));
    if( m_self.pinReset->getDrivenState() )
    {
        Dprintf(("  Reset asserted\n"));
    }
    else if( m_self.pinPreset->getDrivenState() )
    {
        // Preset is not clocked.
        Dprintf(("  Preset asserted\n"));
        signalLoadPresets();
    }
}

void CD451xB::signalLoadPresets()
{
    for(int idxPin=0; idxPin < 4; idxPin++)
    {
        bool pin = m_self.pinsP[idxPin]->getDrivenState();
        m_self.pinsQ[idxPin]->putState( pin );
    }
    this->signalDirection( m_self.pinUpDown->getDrivenState() );
}

void CD451xB::signalDirection(bool increment)
{
    unsigned int value = m_self.portQ->get();

    // Flipping direction can change the carry-out signal
    if( increment && value == m_self.valueMax )
        m_self.pinCarryOut->putState(false);
    else if( ! increment && value == 0 )
        m_self.pinCarryOut->putState(false);
    else
        m_self.pinCarryOut->putState(true);
}



/*****************************************************************************
 ** Chip setup and presentation
 *****************************************************************************/

CD451xB::CD451xB(const char *_name, const uint8_t max_value, const char *_desc)
    : Module(_name, _desc)
{
    m_self.portP = new DataPort();
    m_self.portQ = new DataPort();
    m_self.valueMax = max_value;
}

void CD451xB::create_iopin_map()
{
    Dprintf(("CD451xB::create_iopin_map()\n"));
    int idx;
    char name[4] = "Px\0";

    m_self.pinClock = new ClockPin(this);
    m_self.pinReset = new ResetPin(this);
    m_self.pinPreset = new PresetPin(this);
    m_self.pinUpDown = new UpDownPin(this);
    m_self.pinCarryIn = new CtrlPin("CarryIn", 0);
    m_self.pinCarryOut = new CtrlPin("CarryOut", 1);

    for(idx=0; idx<4; idx++)
    {
        name[0] = 'P';
        name[1] = '1' + idx;
        m_self.pinsP[idx] = new DataPin(this, name);
        name[0] = 'Q';
        m_self.pinsQ[idx] = new DataPin(this, name);
        m_self.pinsQ[idx]->update_direction(1, true);
    }

    m_self.pinCarryOut->putState(false);

    package = new Package(16);
    package->assign_pin(15, m_self.pinClock);
    package->assign_pin(1, m_self.pinPreset);
    package->assign_pin(9, m_self.pinReset);
    package->assign_pin(10, m_self.pinUpDown);
    package->assign_pin(5, m_self.pinCarryIn);
    package->assign_pin(7, m_self.pinCarryOut);
    package->assign_pin(4,  m_self.portP->addPin(m_self.pinsP[0], 0));
    package->assign_pin(12, m_self.portP->addPin(m_self.pinsP[1], 1));
    package->assign_pin(13, m_self.portP->addPin(m_self.pinsP[2], 2));
    package->assign_pin(3,  m_self.portP->addPin(m_self.pinsP[3], 3));
    package->assign_pin(6,  m_self.portQ->addPin(m_self.pinsQ[0], 0));
    package->assign_pin(11, m_self.portQ->addPin(m_self.pinsQ[1], 1));
    package->assign_pin(14, m_self.portQ->addPin(m_self.pinsQ[2], 2));
    package->assign_pin(2,  m_self.portQ->addPin(m_self.pinsQ[3], 3));

    for(idx=0; idx<4; idx++)
    {
        addSymbol(m_self.pinsP[idx]);
        addSymbol(m_self.pinsQ[idx]);
    }
    addSymbol(m_self.pinClock);
    addSymbol(m_self.pinPreset);
    addSymbol(m_self.pinReset);
    addSymbol(m_self.pinCarryIn);
    addSymbol(m_self.pinCarryOut);
    addSymbol(m_self.pinUpDown);
}

#ifdef HAVE_GUI

void CD451xB::build_window()
{
    m_self.gtkarea = gtk_drawing_area_new();
    gtk_widget_set_size_request(m_self.gtkarea, 60, 120);
    g_signal_connect(m_self.gtkarea, "expose_event", G_CALLBACK(draw_window), this);
    gtk_widget_set_events(m_self.gtkarea, GDK_EXPOSURE_MASK);
    gtk_widget_show(m_self.gtkarea);
    set_widget(m_self.gtkarea);
}

gboolean CD451xB::draw_window(
    GtkWidget *widget,
    GdkEvent *event,
    gpointer user_data)
{
    CD451xB *base = static_cast<CD451xB *>(user_data);
    g_return_val_if_fail(widget != nullptr, TRUE);
    g_return_val_if_fail(GTK_IS_DRAWING_AREA(widget), TRUE);

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    guint max_width = allocation.width;
    guint max_height = allocation.height;

    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    cairo_t *cairo = gdk_cairo_create(gdk_win);
    cairo_rectangle(cairo, 0.0, 0.0, max_width, max_height);
    cairo_set_source_rgb(cairo, 0.75, 0.75, 0.75);
    cairo_fill(cairo);

    cairo_destroy(cairo);

    return FALSE;
}

void CD451xB::update()
{
    if (get_interface().bUsingGUI())
    {
        gtk_widget_queue_draw(m_self.gtkarea);
    }
}
#endif // HAVE_GUI

} // Namespace CD451xB end
