#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtGlobal>

#ifdef METADLL
#define METAEXPORT Q_DECL_EXPORT
#else
#define METAEXPORT Q_DECL_IMPORT
#endif

#endif // GLOBAL_H
