#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

void logger_init(const char *path);
void logger_close(void);
void logger_log(const char *level, const char *fmt, ...);

#endif
