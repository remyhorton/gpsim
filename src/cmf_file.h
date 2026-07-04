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


/*
Read debug information from CMF file, produced by MPLAB X together with HEX.
Usage example: gpsim -s myprog.cmf  -p p16f887  myprog.hex

CMF files are self-documented in comments. They contain symbols and source file
matching (memory -> source line), but no binary code or processor information.
*/


#ifndef CMF_FILE_H
#define CMF_FILE_H

#include "program_files.h"
#include <string>
#include <vector>


class CmfProgramFileType : public ProgramFileType {
public:
    CmfProgramFileType();

    // ProgramFileType overrides
    int LoadProgramFile(Processor **pProcessor, const char *pFilename,
                        FILE *pFile, const char *pProcessorName) override;

private:
    bool read_code_line(std::string const &line);
    bool read_symbol(std::string const &line);
    Processor *cpu;
};


#endif // CMF_FILE_H
