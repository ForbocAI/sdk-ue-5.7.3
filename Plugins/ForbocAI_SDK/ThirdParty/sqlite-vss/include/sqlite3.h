/*
** sqlite3.h — Minimal declarations for ForbocAI UE SDK
**
** This header declares the subset of the SQLite C API used by
** NativeEngine.cpp (WITH_FORBOC_SQLITE_VEC=1).  The actual
** implementation comes from the sqlite3 amalgamation (sqlite3.c)
** compiled into the plugin by the build system.
**
** SQLite version: 3.49.0
** Header generated for compilation only — see https://sqlite.org
*/

#ifndef SQLITE3_H
#define SQLITE3_H

#ifdef __cplusplus
extern "C" {
#endif

/*
** Version information
*/
#define SQLITE_VERSION        "3.49.0"
#define SQLITE_VERSION_NUMBER 3049000

/*
** Result codes
*/
#define SQLITE_OK           0   /* Successful result */
#define SQLITE_ERROR        1   /* Generic error */
#define SQLITE_ROW        100   /* sqlite3_step() has another row ready */
#define SQLITE_DONE       101   /* sqlite3_step() has finished executing */

/*
** Special destructor value: content is transient, SQLite must copy it.
*/
#define SQLITE_TRANSIENT   ((void(*)(void*))-1)

/*
** Fundamental types
*/
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long int sqlite3_int64;

/*
** Database connection lifecycle
*/
int sqlite3_open(const char *filename, sqlite3 **ppDb);
int sqlite3_close(sqlite3 *db);

/*
** One-step query execution (no result processing)
*/
int sqlite3_exec(
  sqlite3 *db,
  const char *sql,
  int (*callback)(void*, int, char**, char**),
  void *arg,
  char **errmsg
);

/*
** Prepared statement lifecycle
*/
int sqlite3_prepare_v2(
  sqlite3 *db,
  const char *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const char **pzTail
);
int sqlite3_finalize(sqlite3_stmt *pStmt);

/*
** Bind values to prepared statement parameters
*/
int sqlite3_bind_text(
  sqlite3_stmt *pStmt,
  int index,
  const char *value,
  int n,
  void (*destructor)(void*)
);
int sqlite3_bind_int(sqlite3_stmt *pStmt, int index, int value);
int sqlite3_bind_double(sqlite3_stmt *pStmt, int index, double value);
int sqlite3_bind_int64(sqlite3_stmt *pStmt, int index, sqlite3_int64 value);

/*
** Execute a prepared statement (one step)
*/
int sqlite3_step(sqlite3_stmt *pStmt);

/*
** Extract column values from the current result row
*/
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
double              sqlite3_column_double(sqlite3_stmt *pStmt, int iCol);
sqlite3_int64       sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol);

#ifdef __cplusplus
}
#endif

#endif /* SQLITE3_H */
