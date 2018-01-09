//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSysInfo.h"
#include "tsSysUtils.h"
#include "tsMemoryUtils.h"
#if defined(TS_MAC)
#include "tsMacPList.h"
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
TSDUCK_SOURCE;

// Define singleton instance
TS_DEFINE_SINGLETON(ts::SysInfo);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::SysInfo::SysInfo() :
#if defined(TS_LINUX)
    _isLinux(true),
#else
    _isLinux(false),
#endif
    _isFedora(false),
    _isRedHat(false),
    _isUbuntu(false),
#if defined(TS_MAC)
    _isMacOS(true),
#else
    _isMacOS(false),
#endif
#if defined(TS_WINDOWS)
    _isWindows(true),
#else
    _isWindows(false),
#endif
#if defined(TS_I386)
    _isIntel32(true),
#else
    _isIntel32(false),
#endif
#if defined(TS_X86_64)
    _isIntel64(true),
#else
    _isIntel64(false),
#endif
#if defined(TS_UNIX)
    _isRootUser(::geteuid() == 0),
#else
    _isRootUser(false),
#endif
    _systemVersion(),
    _systemName()
{
#if defined(TS_LINUX)

    // On Linux, the actual system shall determined dynamically.
    UStringList lines;
    Environment env;
    if (UString::Load(lines, u"/etc/fedora-release")) {
        _isFedora = true;
        _systemName = u"Fedora";
        if (!lines.empty()) {
            _systemVersion = lines.front();
        }
    }
    else if (UString::Load(lines, u"/etc/redhat-release")) {
        _isRedHat = true;
        _systemName = u"Red Hat Entreprise Linux";
        if (!lines.empty()) {
            _systemVersion = lines.front();
        }
    }
    else if (LoadEnvironment(env, u"/etc/lsb-release")) {
        _systemName = env[u"DISTRIB_ID"];
        _systemVersion = env[u"DISTRIB_DESCRIPTION"];
        if (_systemVersion.empty()) {
            _systemVersion = env[u"DISTRIB_RELEASE"];
        }
        _isUbuntu = _systemName.similar(u"Ubuntu");
    }
    if (_systemName.empty()) {
        _systemName = u"Linux";
    }

#elif defined(TS_MAC)

    // Get system version.
    MacPList sysList(u"/System/Library/CoreServices/SystemVersion.plist");
    const UString sysName(sysList[u"ProductName"]);
    const UString sysVersion(sysList[u"ProductVersion"]);
    if (!sysName.empty() && !sysVersion.empty()) {
        _systemName = sysName;
        _systemVersion = sysName + u" " + sysVersion;
    }
    else {
        _systemName = u"macOS";
    }

    // Get kernel version.
    int mib[2];
    size_t len;
    mib[0] = CTL_KERN;
    mib[1] = KERN_OSRELEASE;
    if (::sysctl(mib, 2, NULL, &len, NULL, 0) == 0 ) {
        std::string version(len, 0);
        if (::sysctl(mib, 2, &version[0], &len, NULL, 0) == 0 ) {
            const bool wasEmpty = _systemVersion.empty();
            if (!wasEmpty) {
                _systemVersion += u" (";
            }
            _systemVersion += u"Darwin " + UString::FromUTF8(&version[0], len);
            if (!wasEmpty) {
                _systemVersion += u")";
            }
        }
    }

#elif defined(TS_WINDOWS)

    _systemName = u"Windows";

    // With Microsoft compiler:
    // warning C4996: 'GetVersionExW': was declared deprecated
#if defined(TS_MSC)
#pragma warning(push)
#pragma warning(disable:4996)
#endif

    // System version.
    ::OSVERSIONINFOW info;
    TS_ZERO(info);
    info.dwOSVersionInfoSize = sizeof(info);
    if (::GetVersionExW(&info)) {
        _systemVersion = UString::Format(u"Windows %d.%d Build %d %s", {info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber, UString(info.szCSDVersion)});
        _systemVersion.trim();
    }

#if defined(TS_MSC)
#pragma warning(pop)
#endif

    // Detect 32-bit application on 64-bit system.
    ::BOOL wow64 = 0;
    if (::IsWow64Process(::GetCurrentProcess(), &wow64) && wow64) {
        // 32-bit application on 64-bit system => set system characteristics, not application.
        _isIntel32 = false;
        _isIntel64 = true;
    }

#endif

    // System version defaults to system name.
    if (_systemVersion.empty()) {
        _systemVersion = _systemName;
    }
}