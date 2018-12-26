#ifndef ZRSOCKET_VERSION_H_
#define ZRSOCKET_VERSION_H_

#define ZRSOCKET_VERSION_MAJOR  1
#define ZRSOCKET_VERSION_MINOR  2
#define ZRSOCKET_VERSION_PATCH  1

#define ZRSOCKET_MAKE_VERSION(major, minor, patch) \
    (major * 1000 * 1000u + minor * 1000u + patch)

#define ZRSOCKET_MAKE_VERSION_STR(major, minor, patch) \
    #major "." #minor "." #patch

//! This is zrsocket version number as unsigned integer.  This must
//! be kept on a single line. It is used by Autotool and CMake build
//! systems to parse version number.
#define ZRSOCKET_VERSION ZRSOCKET_MAKE_VERSION(ZRSOCKET_VERSION_MAJOR, ZRSOCKET_VERSION_MINOR, ZRSOCKET_VERSION_PATCH)

//! This is zrsocket version number as a string.
#define ZRSOCKET_VERSION_STR ZRSOCKET_MAKE_VERSION_STR(1, 2, 1)

#endif
