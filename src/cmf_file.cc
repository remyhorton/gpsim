/*
   Copyright (C) 2024 Michele Alessandrini <michelealessandrini74@gmail.com>

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


#include "cmf_file.h"
#include "ui.h"
#include <iostream>
#include <cstdio>
#include <algorithm>
#include "processor.h"


CmfProgramFileType::CmfProgramFileType() {
    RegisterProgramFileType(this);
}

#ifdef _WIN32
// This function not in windows stdio
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (!lineptr || !stream || !n) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = (char *)malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
        while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = (char *)realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
        *n = size;

    return p - bufptr - 1;
}
#endif //_WIN32


int CmfProgramFileType::LoadProgramFile(Processor **pProcessor, const char * pFilename, FILE *pFile, const char * /*pProcessorName*/) {

    if (verbose) std::cout << "CmfProgramFileType::LoadProgramFile load cmf\n";
    cpu = *pProcessor;
    if (!cpu) cpu = get_active_cpu();
    if (!cpu) return ERR_NEED_PROCESSOR_SPECIFIED;

    char *lineptr = nullptr;
    size_t n, line_num = 0;
    std::string sec_name;
    bool error = false;

    while (getline(&lineptr, &n, pFile) >= 0) {
        std::string line(lineptr);
        ++line_num;
        while (line.size() && std::isspace(line.back())) line.resize(line.size() - 1);
        if (!line.size() || line[0] == '#') continue;
        if (line[0] == '%') {
            // start of a new section
            sec_name = line;
        }
        else {
            // line belonging to a specific section
            bool ok = sec_name == "%LINETAB" ? read_code_line(line) :
                sec_name == "%SYMTAB" ? read_symbol(line) : true;
            if (!ok || !sec_name.size()) {
                error = true;
                break;
            }
        }
    }
    free(lineptr);
    if (error) return ERR_BAD_FILE;
    cpu->read_src_files();
    return SUCCESS;
}


bool CmfProgramFileType::read_code_line(const std::string &line) {
    // Example of a line in %LINETAB section:
    // F8 ioCode CODE >30:/....../io.asm
    if (line[0] == '$') return true;
    size_t spc1 = line.find(' '),
        spc2 = spc1 != std::string::npos ? line.find(' ', spc1 + 1) : std::string::npos,
        spc3 = spc2 != std::string::npos ? line.find(' ', spc2 + 1) : std::string::npos;
    if (spc1 == 0 || spc3 == std::string::npos) return false;
    std::string f = line.substr(spc3 + 1);
    if (f.size() < 4 || f[0] != '>') return false;
    char *endptr;
    unsigned long address = strtoul(line.c_str(), &endptr, 16);
    if (*endptr != ' ') return false;
    unsigned long line_num = strtoul(f.c_str() + 1, &endptr, 10);
    if (*endptr != ':') return false;
    std::string file_name = f.substr(f.find(':') + 1);
    int i = cpu->files.Find(file_name);
    if (i < 0) i = cpu->files.Add(file_name);
    cpu->attach_src_line(address, i, line_num, 0);
    return true;
}


bool CmfProgramFileType::read_symbol(const std::string &line) {
    // Example of a line in %SYMTAB section:
    // readAdc 1F0 0 CODE 0 ioCode build/default/production/io.o
    size_t spc1 = line.find(' '),
        spc2 = spc1 != std::string::npos ? line.find(' ', spc1 + 1) : std::string::npos;
    if (spc1 == 0 || spc2 == std::string::npos) return false;
    std::string sym = line.substr(0, spc1);
    if (sym.substr(0, 2) == "__") return true;
    cpu->addSymbol(new Integer(sym.c_str(), strtoul(line.c_str() + spc1 + 1, nullptr, 16)));
    return true;
}
