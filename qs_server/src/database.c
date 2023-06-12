#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "database.h"
#include "util.h"
#include "assert.h"

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
    if (
        (sqlite3_prepare_v2(db->sql, STORE_MSG_TEXT, 
            -1, &db->store_stmt, NULL) != SQLITE_OK) ||
        (sqlite3_prepare_v2(db->sql, GET_MSG_TEXT, 
        -1, &db->get_stmt, NULL) != SQLITE_OK)
    ) {
        die("Failed to prepare stmts");
    }
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

long db_create_session(Database* db, char* name)
{
    try_clean(db);

    sqlite3_reset(db->store_stmt);

    sqlite3_bind_text(
        db->store_stmt, 
        1, 
        name, 
        -1, 
        SQLITE_STATIC
    );

    if (sqlite3_step(db->store_stmt) != SQLITE_DONE) {
        printf(
            "SQLITE ERROR: %s ('%s')\n", 
            sqlite3_errmsg(db->sql),
            name
        );
        return 0;
    }

    return sqlite3_last_insert_rowid(db->sql);
}

long db_get_session(Database* db, char* name)
{
    try_clean(db);

    sqlite3_reset(db->get_stmt);

    sqlite3_bind_text(
        db->get_stmt, 
        1,
        name,
        -1,
        SQLITE_STATIC
    );

    if (sqlite3_step(db->get_stmt) == SQLITE_ROW)
        return sqlite3_column_int64(db->get_stmt, 1);
    
    return 0;
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