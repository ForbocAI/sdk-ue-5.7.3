/*
** sqlite_vss.h — sqlite-vec extension entry point for ForbocAI UE SDK
**
** Declares the auto-extension entry point for sqlite-vec (vec0 virtual
** table).  When sqlite-vec is compiled into the plugin as source
** (vec0.c), this function is called via sqlite3_auto_extension()
** during database initialisation so that the vec0 virtual table
** module is available to all connections.
**
** See https://github.com/asg017/sqlite-vec
*/

#ifndef SQLITE_VEC_H
#define SQLITE_VEC_H

#include "sqlite3.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** sqlite3_vec_init
**
** Standard SQLite extension entry point for sqlite-vec.
** Register via sqlite3_auto_extension() before opening any database:
**
**   sqlite3_auto_extension((void(*)(void))sqlite3_vec_init);
**
** Parameters follow the SQLite extension loading convention:
**   db       — database connection handle
**   pzErrMsg — set to error string on failure (caller frees with sqlite3_free)
**   pApi     — pointer to sqlite3_api_routines (used by loadable extensions;
**              NULL when statically linked)
*/
int sqlite3_vec_init(
  sqlite3 *db,
  char **pzErrMsg,
  const void *pApi
);

#ifdef __cplusplus
}
#endif

#endif /* SQLITE_VEC_H */
