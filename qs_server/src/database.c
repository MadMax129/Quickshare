#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "database.h"
#include "die.h"
#include "assert.h"

static const char* sqlite_stmts_text[] = {
    [SESSION_STMT_CREATE] = 
        "insert into Sessions ('name') values(?)",
    [SESSION_STMT_GET] =
        "select * from Sessions where name=?",
    [TRANSFER_STMT_CREATE] = 
        "insert into Transfers ('creator_id') values (?)",
    [TRANSFER_STMT_GET] = 
        "select * from Transfers where creator_id=?",
    [TRANSFER_STMT_DEL] =
        "delete from Transfers where transfer_id=?",
    [TRANSFER_CLIENT_CREATE] = 
        "insert into TransferClients ('transfer_id', 'client_id') values (?, ?)",
    [TRANSFER_GET_CREATOR] = 
        "select Transfers.transfer_id, Transfers.creator_id "
        "from Transfers "
        "join TransferClients "
        "on Transfers.transfer_id = TransferClients.transfer_id "
        "where TransferClients.client_id=?",
    [TRANSFER_CLIENT_DEL] =
        "delete from TransferClients where client_id=?",
    [TRANSFER_CLIENT_GET_ALL] =
        "select * from TransferClients where transfer_id=? and accepted != 0",
    [TRANSFER_CLIENT_DEL_ALL] =
        "delete from TransferClients where transfer_id=?",
    [TRANSFER_CLEANUP] =
        "begin transaction; delete from TransferClients where transfer_id=?;"
        "delete from Transfers where transfer_id=?; commit",
    [BEGIN_TRANSACTION] = 
        "begin transaction;",
    [COMMIT_TRANSACTION] =
        "commit;",
    [ROLLBACK_TRANSACTION] =
        "rollback;",
    [NUM_STMT_TYPES] = NULL
};

static void read_and_exec(Database* db, const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file) die("read_and_exec()");

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* sql_text = (char*)malloc(length + 1);
    if (!sql_text) {
        fclose(file);
        die("read_and_exec()");
    }

    size_t r = fread(sql_text, 1, length, file);
    (void)r;
    sql_text[length] = '\0';
    fclose(file);

    printf("%s\n", sql_text);

    char* errmsg;
    if (sqlite3_exec(
        db->sql,
        sql_text,
        NULL, NULL, &errmsg
    ) != SQLITE_OK) {
        printf("Sqlite %s\n", errmsg);
        die("read_and_exec()");
    }

    free(sql_text);
}

static void prepare_stmts(Database* db)
{
    for (int i = 0; i < NUM_STMT_TYPES; i++) {
        const int e = sqlite3_prepare_v2(
            db->sql, 
            sqlite_stmts_text[i], 
            -1, 
            &db->stmts[i], 
            NULL
        );

        if (e != SQLITE_OK)
            die("Failed to prepare stmts");
    }
}

void db_init(Database* db)
{
    if (sqlite3_open(DATABASE_PATH, &db->sql) != SQLITE_OK)
        die("sqlite_open()");

    read_and_exec(db, DB_INIT_FILE);
    prepare_stmts(db);

    db->last_clean_time = time(NULL);

    assert(sqlite3_threadsafe() == 1);
}

static inline void try_clean(Database* db)
{
    time_t time_now = time(NULL);

    if ((difftime(time_now, db->last_clean_time) / 60.0) >= 10.0) {
        printf("Running cleaner...\n");
        if (sqlite3_exec(
            db->sql, 
            "delete from Sessions where Cast (("
            "    julianday('now', 'localtime') - julianday(time)"
            ") * 24 * 60 as integer) >= 10;",
            NULL,
            NULL,
            NULL
        ) != SQLITE_OK) {
            printf("Failed to cleanup old keys '%s'\n", sqlite3_errmsg(db->sql));
        }
        db->last_clean_time = time(NULL);
    }
}

Session_ID db_create_session(Database* db, char* name)
{
    const DB_Stmt_Type type = SESSION_STMT_CREATE;

    try_clean(db);

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_text(
        db->stmts[type], 
        1, 
        name, 
        -1,
        SQLITE_STATIC
    );

    if (sqlite3_step(db->stmts[type]) != SQLITE_DONE) {
        printf(
            "SQLITE ERROR: %s ('%s')\n", 
            sqlite3_errmsg(db->sql),
            name
        );
        return 0;
    }

    return sqlite3_last_insert_rowid(db->sql);
}

Session_ID db_get_session(Database* db, char* name)
{
    const DB_Stmt_Type type = SESSION_STMT_GET;
    try_clean(db);

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_text(
        db->stmts[type], 
        1,
        name,
        -1,
        SQLITE_STATIC
    );

    if (sqlite3_step(db->stmts[type]) == SQLITE_ROW)
        return sqlite3_column_int64(db->stmts[type], 1);
    
    return 0;
}

void db_cleanup_transfer(Database* db, Transfer_ID t_id)
{
    sqlite3_reset(db->stmts[TRANSFER_STMT_DEL]);
    sqlite3_reset(db->stmts[TRANSFER_CLIENT_DEL_ALL]);

    (void)sqlite3_bind_int64(
        db->stmts[TRANSFER_STMT_DEL], 
        1,
        t_id
    );

    (void)sqlite3_bind_int64(
        db->stmts[TRANSFER_CLIENT_DEL_ALL], 
        1,
        t_id
    );

    (void)sqlite3_step(db->stmts[TRANSFER_STMT_DEL]);
    (void)sqlite3_step(db->stmts[TRANSFER_CLIENT_DEL_ALL]);
}

Transfer_ID db_create_transfer(Database* db, Client_ID c_id)
{
    const DB_Stmt_Type type = TRANSFER_STMT_CREATE;

    try_clean(db);

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type], 
        1,
        c_id
    );

    if (sqlite3_step(db->stmts[type]) != SQLITE_DONE) {
        printf(
            "SQLITE ERROR: %s\n", 
            sqlite3_errmsg(db->sql)
        );
        return 0;
    }

    return sqlite3_last_insert_rowid(db->sql);
}

static sqlite_int64 db_step_int64(Database* db, const DB_Stmt_Type type, const int row)
{
    if (sqlite3_step(db->stmts[type]) == SQLITE_ROW)
        return sqlite3_column_int64(db->stmts[type], row);
    
    return 0;
}

Transfer_ID db_get_transfer(Database* db, Client_ID c_id)
{
    const DB_Stmt_Type type = TRANSFER_STMT_GET;
    
    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type],
        1,
        c_id
    );

    return db_step_int64(db, type, 0);
}

sqlite_int64 db_transfer_step(Database* db)
{
    return db_step_int64(db, TRANSFER_STMT_GET, 0);
}

Client_ID db_get_client_all(Database* db, Transfer_ID t_id)
{
    const DB_Stmt_Type type = TRANSFER_CLIENT_GET_ALL;

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type],
        1,
        t_id
    );

    return db_step_int64(db, type, 1);
}

Client_ID db_client_all_step(Database* db)
{
    return db_step_int64(db, TRANSFER_CLIENT_GET_ALL, 1);
}

Transfer_Info db_get_creator(Database* db, Client_ID c_id)
{
    const DB_Stmt_Type type = TRANSFER_GET_CREATOR;

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type],
        1,
        c_id
    );

    return db_creator_step(db);
}

Transfer_Info db_creator_step(Database* db)
{
    Transfer_Info info = {0, 0};

    if (sqlite3_step(db->stmts[TRANSFER_GET_CREATOR]) == SQLITE_ROW) {
        info.t_id    = sqlite3_column_int64(
            db->stmts[TRANSFER_GET_CREATOR],
            0
        );
        
        info.creator = sqlite3_column_int64(
            db->stmts[TRANSFER_GET_CREATOR], 
            1
        );
    }

    return info;
}

void db_client_delete(Database* db, Client_ID c_id)
{
    const DB_Stmt_Type type = TRANSFER_CLIENT_DEL;

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type],
        1,
        c_id
    );

    (void)sqlite3_step(db->stmts[type]);
}

bool db_transaction(Database* db, DB_Stmt_Type type)
{
    sqlite3_reset(db->stmts[type]);

    if (sqlite3_step(db->stmts[type]) != SQLITE_DONE) {
        printf(
            "SQLITE ERROR: %s\n", 
            sqlite3_errmsg(db->sql)
        );
        return false;
    }

    return true;
}

bool db_create_client(Database* db, Transfer_ID t_id, Client_ID c_id)
{
    const DB_Stmt_Type type = TRANSFER_CLIENT_CREATE;

    sqlite3_reset(db->stmts[type]);

    sqlite3_bind_int64(
        db->stmts[type], 
        1,
        t_id
    );

    sqlite3_bind_int64(
        db->stmts[type], 
        2,
        c_id
    );

    if (sqlite3_step(db->stmts[type]) != SQLITE_DONE) {
        printf(
            "SQLITE ERROR: %s\n", 
            sqlite3_errmsg(db->sql)
        );
        return false;
    }
    
    return true;
}