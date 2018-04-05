#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               0
#define VERSION_MINOR               1

// This must be generated AUTOMATICALY in Pre-Compile step.
// There are actual VERSION_REVISION and VERSION_BUILD
#include "build_version.h"

#ifndef VERSION_REVISION
#define VERSION_REVISION            0
#endif

#ifndef VERSION_BUILD
#define VERSION_BUILD               0
#endif

#define VER_COMPANY_NAME            "Quest"

#define VER_FILE_DESCRIPTION_STR    "Life export library"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD, VERSION_REVISION
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)           \
                                    "." STRINGIZE(VERSION_MINOR)       \
                                    "." STRINGIZE(VERSION_BUILD)       \
                                    "." STRINGIZE(VERSION_REVISION)    \

#define VER_PRODUCTNAME_STR         "Rapid Recovery"
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   "LifeExportLibrary.dll"
#define VER_INTERNAL_NAME_STR       VER_ORIGINAL_FILENAME_STR
#define VER_COPYRIGHT_STR           "Copyright (C) 2018"

#ifdef _DEBUG
#define VER_VER_DEBUG             VS_FF_DEBUG
#else
#define VER_VER_DEBUG             0
#endif

#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               VER_VER_DEBUG
#define VER_FILETYPE                VFT_APP
