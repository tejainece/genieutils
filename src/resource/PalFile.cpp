/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Armin Preiml

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "genie/resource/PalFile.h"

#include <iostream>
#include <stdexcept>

#include "genie/util/Logger.h"

namespace genie {

const PalFile PalFile::null;

using std::string;

Logger &PalFile::log = Logger::getLogger("genie.PalFile");

//------------------------------------------------------------------------------
PalFile::PalFile()
{
}

//------------------------------------------------------------------------------
PalFile::~PalFile()
{
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::vector<Color> &PalFile::getColors(void) const
{
    return colors_;
}

//------------------------------------------------------------------------------
size_t PalFile::size(void) const
{
    return colors_.size();
}

//------------------------------------------------------------------------------
std::string PalFile::getHeader(void) const
{
    return "JASC-PAL";
}

//------------------------------------------------------------------------------
std::string PalFile::getHeader2(void) const
{
    return "0100";
}

//------------------------------------------------------------------------------
void PalFile::serializeObject(void)
{
    if (isOperation(OP_READ)) {
        std::istream *istr = getIStream();

        std::string header;

        *istr >> header;

        if (header.compare(getHeader()) != 0) {
            log.error("Not a color palette!");
            //TODO: Exception
            return;
        }

        *istr >> header;

        if (header.compare(getHeader2()) != 0)
            log.warn("Different header2 in PalFile");

        *istr >> num_colors_;

        for (uint32_t i = 0; i < num_colors_; i++) {
            uint32_t r, g, b;
            *istr >> r >> g >> b;
            colors_.push_back(Color(r, g, b));
        }
    } else {
        std::ostream *ostr = getOStream();

        std::string CR_LF = "\r\n"; // windows line ending

        *ostr << getHeader() << CR_LF;
        *ostr << getHeader2() << CR_LF;

        if (colors_.size() > 256)
            log.error("Too much colors (>256)");

        *ostr << colors_.size() << CR_LF;

        for (uint32_t i = 0; i < colors_.size(); i++) {
            *ostr << (int)colors_[i].r << " ";
            *ostr << (int)colors_[i].g << " ";
            *ostr << (int)colors_[i].b << CR_LF;
        }
    }
}

//------------------------------------------------------------------------------
size_t PalFile::numOfChars(uint8_t number)
{
    if (number < 10)
        return 1;

    if (number < 100)
        return 2;

    return 3;
}

//------------------------------------------------------------------------------
size_t PalFile::objectSize(void)
{
    int CL_LF_SIZE = 2;

    size_t size = 0;

    size += getHeader().size() + CL_LF_SIZE;
    size += getHeader2().size() + CL_LF_SIZE;

    if (colors_.size() > 256)
        log.error("Too much colors (>256)");

    size += numOfChars(colors_.size()) + CL_LF_SIZE;

    for (uint32_t i = 0; i < colors_.size(); i++) {
        size += numOfChars(colors_[i].r);
        size += numOfChars(colors_[i].g);
        size += numOfChars(colors_[i].b);

        size += 2; // spaces
        size += CL_LF_SIZE;
    }

    return size;
}

bool PalFile::isValid() const
{
    return !colors_.empty();
}

}
