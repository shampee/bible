#ifndef CORE_H
#define CORE_H
#include "base/base_inc.h"
#include <unistd.h>

#define N_CORES sysconf(_SC_NPROCESSORS_ONLN)

#endif // CORE_H
