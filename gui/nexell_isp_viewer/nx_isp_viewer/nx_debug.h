#ifndef NxDebug_H
#define NxDebug_H

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#if 0
#include <stdarg.h>
#include <stdio.h>
//#define NxDebug(...) printf(__VA_ARGS__)
#define NxDebug(f_, ...) printf((f_), __VA_ARGS__)
#define NxDebug_err(f_, ...) printf((f_), __VA_ARGS__)
#else
#include "qdebug.h"
#define NxDebug(...) qDebug(__VA_ARGS__)
#define NxDebugErr(...) qDebug(__VA_ARGS__)
#endif
#else
#define NxDebug(...)
#define NxDebugErr(...) qDebug(__VA_ARGS__)
#endif

#endif // NxDebug_H
