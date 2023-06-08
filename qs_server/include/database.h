#ifndef QS_DB
#define QS_DB

#include <sqlite3.h>

typedef struct {
    sqlite3* db;
} Database;

#endif /* QS_DB */