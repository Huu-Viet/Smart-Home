#pragma once

#if __has_include(<local_config.h>)
#include <local_config.h>
#define DEVICE_LOCAL_CONFIG_AVAILABLE 1
#else
#define DEVICE_LOCAL_CONFIG_AVAILABLE 0
#endif
