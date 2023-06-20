#ifndef QS_DB
#define QS_DB

#include <sqlite3.h>
#include <time.h>
#include <stdbool.h>
#include "config.h"

#define DATABASE_PATH "../db/qs_db"
#define DB_INIT_FILE  "../db/db_init.sql"

typedef enum {
    SESSION_STMT_CREATE,
    SESSION_STMT_GET,
    TRANSFER_STMT_CREATE,
    TRANSFER_STMT_GET,
    TRANSFER_CLIENT_CREATE,
    TRANSFER_CLIENT_GET,
    TRANSFER_CLIENT_GET_ALL,
    TRANSFER_CLEANUP,
    BEGIN_TRANSACTION,
    COMMIT_TRANSACTION,
    ROLLBACK_TRANSACTION,
    NUM_STMT_TYPES
} DB_Stmt_Type;

typedef struct {
    sqlite3* sql;
    sqlite3_stmt *stmts[NUM_STMT_TYPES];
    time_t last_clean_time;
} Database;

typedef long Session_ID;

void db_init(Database* db);
Session_ID db_create_session(Database* db, char* name);
Session_ID db_get_session(Database* db, char* name);
Transfer_ID db_create_transfer(Database* db, Client_ID c_id);
Transfer_ID db_get_transfer(Database* db, Client_ID c_id);
sqlite_int64 db_transfer_step(Database* db);
void db_cleanup_transfer(Database* db, Transfer_ID t_id);
bool db_transaction(Database* db, DB_Stmt_Type type);
bool db_create_client(Database* db, Transfer_ID t_id, Client_ID c_id);
Client_ID db_get_client_all(Database* db, Transfer_ID t_id);
Client_ID db_client_all_step(Database* db);

#endif /* QS_DB */