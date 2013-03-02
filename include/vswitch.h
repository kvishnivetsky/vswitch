#ifndef _CONFIG_
#define _CONFIG_

#include <map>

typedef struct _VSPORT_ {
    int FDType;
} VSPORT, * LPVSPORT;

static std::map<int,VSPORT> links;

bool config_from_file(const char * cfgFile);

bool config_from_args(int argc, char ** argv);

int getLinkCount();

int start_loop();

#endif
