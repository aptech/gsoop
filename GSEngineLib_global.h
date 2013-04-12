#ifndef GSENGINELIB_GLOBAL_H
#define GSENGINELIB_GLOBAL_H

#include <QtCore/qglobal.h>
#include <stdio.h>
#include "mteng.h"

#if defined(GSENGINELIB_LIBRARY)
#  define GSENGINELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define GSENGINELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#include "gsenginelib.h"

#endif // GSENGINELIB_GLOBAL_H
