//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Command line arguments to display PSI/SI tables.
//
//----------------------------------------------------------------------------

#include "tsTablesDisplayArgs.h"
#include "tsHexa.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TablesDisplayArgs::TablesDisplayArgs() :
    raw_dump(false),
    raw_flags(hexa::HEXA),
    tlv_syntax()
{
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::addHelp(Args& args) const
{
    std::string help =
        "\n"
        "Tables and sections formatting options:\n"
        "\n"
        "  -c\n"
        "  --c-style\n"
        "      Same as --raw-dump (no interpretation of section) but dump the\n"
        "      bytes in C-language style.\n"
        "\n"
        "  -r\n"
        "  --raw-dump\n"
        "      Raw dump of section, no interpretation.\n"
        "\n"
        "  --tlv syntax\n"
        "      For sections of unknown types, this option specifies how to interpret\n"
        "      some parts of the section payload as TLV records. Several --tlv options\n"
        "      are allowed, each one describes a part of the section payload.\n"
        "\n"
        "      Each syntax string has the form \"start,size,tagSize,lengthSize,order\".\n"
        "      The start and size fields define the offset and size of the TLV area\n"
        "      in the section payload. If the size field is \"auto\", the TLV extends up\n"
        "      to the end of the section. If the start field is \"auto\", the longest\n"
        "      TLV area in the section payload will be used. The fields tagSize and\n"
        "      lengthSize indicate the size in bytes of the Tag and Length fields in\n"
        "      the TLV structure. The field order must be either \"msb\" or \"lsb\" and\n"
        "      indicates the byte order of the Tag and Length fields.\n"
        "\n"
        "      All fields are optional. The default values are \"auto,auto,1,1,msb\".\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::defineOptions(Args& args) const
{
    args.option("c-style", 'c');
    args.option("raw-dump", 'r');
    args.option("tlv", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::load(Args& args)
{
    raw_dump = args.present("raw-dump");
    raw_flags = hexa::HEXA;
    if (args.present("c-style")) {
        raw_dump = true;
        raw_flags |= hexa::C_STYLE;
    }

    // Get all TLV syntax specifications.
    tlv_syntax.clear();
    const size_t count = args.count("tlv");
    for (size_t i = 0; i < count; ++i) {
        TLVSyntax tlv;
        tlv.fromString(args.value("tlv", "", i), args);
        tlv_syntax.push_back(tlv);
    }
    std::sort(tlv_syntax.begin(), tlv_syntax.end());
}