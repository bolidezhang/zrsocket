// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_VERSION_H
#define ZRSOCKET_VERSION_H

#define ZRSOCKET_VERSION_MAJOR  1
#define ZRSOCKET_VERSION_MINOR  7
#define ZRSOCKET_VERSION_PATCH  4

#define ZRSOCKET_MAKE_VERSION(major, minor, patch) \
    (major * 1000 * 1000u + minor * 1000u + patch)

//! This is zrsocket version number as unsigned integer.  This must
//! be kept on a single line. It is used by Autotool and CMake build
//! systems to parse version number.
#define ZRSOCKET_VERSION ZRSOCKET_MAKE_VERSION(ZRSOCKET_VERSION_MAJOR, ZRSOCKET_VERSION_MINOR, ZRSOCKET_VERSION_PATCH)

//! internal macros, covert macro values to str,
#define ZRSOCKET_INTERNAL_STR(v) #v
#define ZRSOCKET_XSTR(v) ZRSOCKET_INTERNAL_STR(v)

//! This is zrsocket version number as a string.
#define ZRSOCKET_VERSION_STR ZRSOCKET_XSTR(ZRSOCKET_VERSION_MAJOR) "." ZRSOCKET_XSTR(ZRSOCKET_VERSION_MINOR) "." ZRSOCKET_XSTR(ZRSOCKET_VERSION_PATCH)

#endif
