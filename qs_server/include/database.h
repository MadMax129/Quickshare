#ifndef QS_DB
#define QS_DB

#include <sqlite3.h>
#include <time.h>

#define DATABASE_PATH "../db/qs_db"
#define DB_INIT_FILE "../db/db_init.sql"

#define STORE_MSG_TEXT \
    "insert into Sessions ('name') values(?)"

#define GET_MSG_TEXT \
    "select * from Sessions where name=?"

typedef struct {
    sqlite3* sql;
    sqlite3_stmt* store_stmt;
    sqlite3_stmt* get_stmt;
    time_t last_clean_time;
} Database;

typedef long Session_ID;

void db_init(Database* db);
Session_ID db_create_session(Database* db, char* name);
Session_ID db_get_session(Database* db, char* name);

#endif /* QS_DB */