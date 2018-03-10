/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Armin Preiml
    Copyright (C) 2015 - 2016  Mikko "Tapsa" P

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

#pragma once

#include <istream>
#include <vector>

#include "genie/file/IFile.h"
#include "genie/util/Logger.h"
#include "PalFile.h"
#include "SlpFrame.h"
#include "SlpFile.h"

namespace genie {

enum Slope {
    SlopeFlat        = 0,
    SlopeSouthUp     = 1,
    SlopeNorthUp     = 2,
    SlopeWestUp      = 3,
    SlopeEastUp      = 4,
    SlopeSouthWestUp = 5,
    SlopeNorthWestUp = 6,
    SlopeSouthEastUp = 7,
    SlopeNorthEastUp = 8,
    SLOPE_S_UP2      = 9,    // what are these?
    SLOPE_N_UP2      = 10,
    SLOPE_W_UP2      = 11,
    SLOPE_E_UP2      = 12,
    SlopeNorthDown   = 13,
    SlopeSouthDown   = 14,
    SlopeWestDown    = 15,
    SlopeEastDown    = 16,

    SlopeCount       = 17,

    SlopeSouthWestEastUp = SlopeNorthDown,
    SlopeNorthWestEastUp = SlopeSouthDown,
    SlopeNorthSoutEastUp = SlopeWestDown,
    SlopeNorthSouthWestUp = SlopeWestDown,
};

class IcmFile : public IFile
{
public:
    struct InverseColorMap
    {
        std::array<std::array<std::array<uint8_t, 32>, 32>, 32> map;

        inline uint8_t paletteIndex(const int r, const int g, const int b) const {
            return map[r >> 3][g >> 3][b >> 3];
        }
    };
    std::vector<InverseColorMap> maps;

    operator bool() {
        return m_loaded;
    }

private:
    virtual void serializeObject() override
    {
        while (!getIStream()->eof()) {
            InverseColorMap map;
            serialize(map);
            maps.push_back(std::move(map));
        }

        if (getOperation() == OP_READ) {
            m_loaded = true;
        }

        std::cout << "Loaded " << maps.size() << " inverse color maps" << std::endl;
    }

    bool m_loaded = true;
};

class PatternMasksFile : public IFile
{
public:
    // TODO: find better names for the rest of the values
    enum PatternMasks {
        FlatPattern = 0,
        BlackPattern = 1,
        DiagDownPattern = 2,
        DiagUpPattern = 3,
        HalfDownPattern = 4,
        HalfUpPattern = 5,
        HalfLeftPattern = 6,
        HalfRightPattern = 7,
        DownPattern = 8,
        UpPattern = 9,
        LeftPattern = 10,
        RightPattern = 11,
        Pattern12 = 12,
        Pattern13 = 13,
        Pattern14 = 14,
        Pattern15 = 15,
        Pattern16 = 16,
        Pattern17 = 17,
        Pattern18 = 18,
        Pattern19 = 19,
        Pattern20 = 20,
        Pattern21 = 21,
        Pattern22 = 22,
        Pattern24 = 24,
        Pattern25 = 25,
        Pattern26 = 26,
        Pattern27 = 27,
        Pattern28 = 28,
        Pattern29 = 29,
        Pattern30 = 30,
        Pattern31 = 31,
        Pattern32 = 32,
        Pattern33 = 33,
        Pattern34 = 34,
        Pattern35 = 35,
        Pattern36 = 36,
        Pattern37 = 37,
        Pattern38 = 38,
        Pattern39 = 39,
        PatternMasksCount = 40,
    };

    struct PatternMask {
        std::array<uint8_t, 4096> mask;
        inline bool ignore(int index) const {
            return mask[index] & 0x1;
        }
        inline bool brighten(const int index) const {
            return mask[index] & 0x2;
        }
        inline bool darken(const int index) const {
            return (mask[index] & 0x2) == 0;
        }
        inline uint8_t apply(uint8_t input, const int index) const {
            if (ignore(index)) {
                return input;
            }

            const uint8_t icm = mask[index] >> 2;
            input = (input >> 2) & 0x1f;
            if (icm & 2 && icm > input) {
                return icm;
            } else if (icm < input) {
                return icm;
            } else {
                return input;
            }
        }
    };

    operator bool() {
        return m_loaded;
    }

    std::array<PatternMask, PatternMasksCount> masks;

private:
    virtual void serializeObject() override {
        for (int i=0; i<40; i++) {
            int32_t size = 4096;
            serialize(size);
            serialize(masks[i].mask);
        }

        if (getOperation() == OP_READ) {
            m_loaded = true;
        }
    }

    bool m_loaded = false;
};

class FiltermapFile : public IFile
{
public:
    struct Filtermap {
        int height;
        std::vector<uint8_t> commands;
    };
    std::array<Filtermap, SlopeCount> maps;

    operator bool() {
        return m_loaded;
    }

private:
    virtual void serializeObject() override {
        for (int i=0; i<SlopeCount; i++) {
            uint32_t mapSize = 0;
            serialize(mapSize);
            serialize(maps[i].height);
            serialize(maps[i].commands, mapSize - sizeof(maps[i].height));
//            std::cout << "Loaded " << maps[i].commands.size() << " commands" << std::endl;
        }
        if (getOperation() == OP_READ) {
            m_loaded = true;
        }
    }
    bool m_loaded = false;
};

//------------------------------------------------------------------------------
/// Basically a patch for an SLP frame, for sloped terrain
/// Overwrites the header (size, hotspot and command tables)
//
class SlpTemplateFile : public IFile
{
    struct SlpTemplate {
        uint32_t width_;
        uint32_t height_;

        int32_t hotspot_x;
        int32_t hotspot_y;

        int32_t data_size;

        std::vector<unsigned short> left_edges_;
        std::vector<unsigned short> right_edges_;

        std::vector<uint32_t> cmd_offsets_;

        uint32_t outline_table_offset_;
        uint32_t cmd_table_offset_;
    };

public:
    FiltermapFile filtermapFile;
    PatternMasksFile patternmasksFile;
    IcmFile icmFile;

    //----------------------------------------------------------------------------
    /// Constructor
    //
    SlpTemplateFile();

    //----------------------------------------------------------------------------
    /// Destructor
    //
    virtual ~SlpTemplateFile();

    //----------------------------------------------------------------------------
    /// Frees all content of a slp file.
    //
    void unload(void);

    //----------------------------------------------------------------------------
    /// Check whether the files content is loaded or not.
    //
    bool isLoaded(void) const;

    //----------------------------------------------------------------------------
    /// Returns the slp frame at given frame index.
    ///
    /// @param frame frame index
    /// @return SlpFrame
    //
    SlpFramePtr getFrame(const SlpFilePtr &baseFile, const Slope slope);

private:
    static Logger &log;

    bool loaded_ = false;

    //----------------------------------------------------------------------------
    virtual void serializeObject(void);

    //----------------------------------------------------------------------------
    /// Loads the file and its frames.
    //
    void loadFile(void);
    void saveFile(void);

    std::array<SlpTemplate, SlopeCount> templates_;
};

typedef std::shared_ptr<SlpTemplateFile> SlpTemplateFilePtr;
}
