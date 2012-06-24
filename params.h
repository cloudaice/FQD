#ifndef _PARAMS_H_
#define _PARAMS_H_

//定义fuse的API号
#define FUSE_USE_VERSION 26

#define _XOPEN_SOURCE 500

// maintain bbfs state in here
#include <limits.h>
#include <stdio.h>
struct bb_state {
    FILE *logfile;
    char *rootdir;
};
#define BB_DATA ((struct bb_state *) fuse_get_context()->private_data)

#endif
