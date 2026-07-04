/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg

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

#include "settings_exdbm.h"

#include <cstdio>
#include <cstdlib>
#include <string>

extern "C"
{
#include "../eXdbm/eXdbm.h"
#include "../eXdbm/eXdbmErrors.h"
}

SettingsEXdbm::SettingsEXdbm(const char *appl_name)
{
    if (eXdbmInit() == -1)
    {
        puts(eXdbmGetErrorString(eXdbmGetLastError()));
    }

    const char *homedir = getenv("HOME");
    if (!homedir)
        homedir = ".";

    const std::string path = std::string(homedir) + "/." + appl_name;

    if (eXdbmOpenDatabase((char *)path.c_str(), &dbid) == -1)
    {
        if (eXdbmGetLastError() == DBM_OPEN_FILE)
        {
            if (eXdbmNewDatabase((char *)path.c_str(), &dbid) == -1)
                puts(eXdbmGetErrorString(eXdbmGetLastError()));
            else
            {
                if (eXdbmUpdateDatabase(dbid) == -1)
                    puts(eXdbmGetErrorString(eXdbmGetLastError()));
            }
        }
        else
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
    }
}

bool SettingsEXdbm::set(const char *module, const char *entry, const char *str)
{
    DB_LIST list = eXdbmGetList(dbid, nullptr, (char *)module);
    if (!list)
    {
        if (eXdbmCreateList(dbid, nullptr, (char *)module, nullptr) == -1)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }

        list = eXdbmGetList(dbid, nullptr, (char *)module);
        if (!list)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }
    }

    // We have the list
    if (eXdbmChangeVarString(dbid, list, (char *)entry, (char *)str) == -1)
    {
        if (eXdbmCreateVarString(dbid, list, (char *)entry, nullptr, (char *)str) == -1)
        {
            puts("\n\n\n\ndidn't work");
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            puts("\n\n\n\n");
            return false;
        }
    }

    if (eXdbmUpdateDatabase(dbid) == -1)
    {
        puts(eXdbmGetErrorString(eXdbmGetLastError()));
        return false;
    }

    return true;
}

bool SettingsEXdbm::set(const char *module, const char *entry, int value)
{
    if (!module || !entry)
        return false;

    DB_LIST list = eXdbmGetList(dbid, nullptr, (char *)module);
    if (!list)
    {
        if (eXdbmCreateList(dbid, nullptr, (char *)module, nullptr) == -1)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }

        list = eXdbmGetList(dbid, nullptr, (char *)module);
        if (!list)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }
    }

    // We have the list
    if (eXdbmChangeVarInt(dbid, list, (char *)entry, value) == -1)
    {
        if (eXdbmCreateVarInt(dbid, list, (char *)entry, nullptr, value) == -1)
        {
            puts("\n\n\n\ndidn't work");
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            puts("\n\n\n\n");
            return false;
        }
    }

    if (eXdbmUpdateDatabase(dbid) == -1)
    {
        puts(eXdbmGetErrorString(eXdbmGetLastError()));
        return false;
    }

    return true;
}

bool SettingsEXdbm::get(const char *module, const char *entry, char **str)
{
    DB_LIST list = eXdbmGetList(dbid, nullptr, (char *)module);
    if (!list)
        return false;

    // We have the list
    const int ret = eXdbmGetVarString(dbid, list, (char *)entry, str);
    if (ret != -1)
    {
        static char *last = nullptr;
        free(last);
        last = *str;
    }
    return ret != -1;
}

bool SettingsEXdbm::get(const char *module, const char *entry, int *value)
{
    DB_LIST list = eXdbmGetList(dbid, nullptr, (char *)module);
    if (!list)
        return false;

    // We have the list
    const int ret = eXdbmGetVarInt(dbid, list, (char *)entry, value);
    return ret != -1;
}

bool SettingsEXdbm::remove(const char *module, const char *entry)
{
    DB_LIST list = eXdbmGetList(dbid, nullptr, (char *)module);
    if (!list)
    {
        if (eXdbmCreateList(dbid, nullptr, (char *)module, nullptr) == -1)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }

        list = eXdbmGetList(dbid, nullptr, (char *)module);
        if (!list)
        {
            puts(eXdbmGetErrorString(eXdbmGetLastError()));
            return false;
        }
    }

    // We have the list
    if (eXdbmDeleteEntry(dbid, list, (char *)entry) == -1)
    {
        return false;
    }

    if (eXdbmUpdateDatabase(dbid) == -1)
    {
        puts(eXdbmGetErrorString(eXdbmGetLastError()));
        return false;
    }

    return true;
}
