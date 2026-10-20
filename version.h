#pragma once

// The component version. This is the only place to bump it: version.cpp (the foobar2000 component list),
// version.rc2 (the DLL's file properties) and tools/package.ps1 (the package name) all read it.
// Release tags are vMAJOR.MINOR.PATCH.
#define NAUPURE_VERSION_MAJOR 1
#define NAUPURE_VERSION_MINOR 2
#define NAUPURE_VERSION_PATCH 0

#define NAUPURE_STRINGIZE_(x) #x
#define NAUPURE_STRINGIZE(x) NAUPURE_STRINGIZE_(x)
// "1.2.0"
#define NAUPURE_VERSION_STRING NAUPURE_STRINGIZE(NAUPURE_VERSION_MAJOR) "." NAUPURE_STRINGIZE(NAUPURE_VERSION_MINOR) "." NAUPURE_STRINGIZE(NAUPURE_VERSION_PATCH)
