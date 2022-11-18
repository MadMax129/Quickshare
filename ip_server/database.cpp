#include "ip_server.hpp"

bool Hosts::create_sql()
{
    if (sqlite3_open(DATABASE_PATH, &db) != SQLITE_OK)
        return false;

    if (sqlite3_exec(
        db,
        "create table if not exists 'Hosts' ("
	        "'name' TEXT not null unique,"
	        "'ip' TEXT not null,"
	       "'time' DATETIME not null default (datetime(CURRENT_TIMESTAMP, 'localtime'))"
        ");",
        NULL, NULL,NULL
    ) != SQLITE_OK)
        return false;

    assert(sqlite3_threadsafe() == 1);

    c_th = std::thread(&Hosts::cleaner, this);

    return true;
}

void Hosts::cleaner()
{
    uint32_t num_cleans = 0;
    
    while (global_state.load(std::memory_order_relaxed) == ONLINE) {
        std::unique_lock<std::mutex> lk(c_mtx);
        cond.wait_for(lk, std::chrono::minutes(5));
        if (global_state.load(std::memory_order_relaxed) == SHUTDOWN)
            break;

        std::printf("Cleaning #%d\n", num_cleans++);

        sqlite3_mutex_enter(sqlite3_db_mutex(db));
        std::printf("Running cleaner...\n");
        if (sqlite3_exec(
            db, 
            "delete from Hosts where Cast (("
		    "    julianday('now', 'localtime') - julianday(time)"
	        ") * 24 * 60 as integer) >= 5;",
            NULL,
            NULL,
            NULL
        ) != SQLITE_OK) {
            std::printf("Failed to cleanup old keys '%s'\n", sqlite3_errmsg(db));
        }

        sqlite3_mutex_leave(sqlite3_db_mutex(db));
    }
}

void Hosts::find_entry(const char* net_name, Ip_Msg* msg)
{
    sqlite3_mutex_enter(sqlite3_db_mutex(db));
    sqlite3_stmt *stmt;
    char buffer[256] = {};

    const int l = std::snprintf(
        buffer, 
        sizeof(buffer), 
        "select * from Hosts where name=\"%s\"", 
        net_name
    );
    
    assert(l < (int)sizeof(buffer));

    sqlite3_prepare_v2(db, buffer, -1, &stmt, NULL);

    int ret = sqlite3_step(stmt);

    switch (ret) 
    {
        case SQLITE_ROW: {
            msg->type = Ip_Msg::RESPONSE;
            std::strncpy(
                msg->response.ip, 
                (char*)sqlite3_column_text(stmt, 1), 
                IP_ADDR_LEN
            );
            break;
        }

        default: {
            // Either error or row does not exist
            if (ret == SQLITE_ERROR)
                std::printf("Error occured '%s'\n", sqlite3_errmsg(db));
            msg->type = Ip_Msg::INVALID;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_mutex_leave(sqlite3_db_mutex(db));
}

void Hosts::create_entry(const char* net_name, const char* ip, Ip_Msg* msg)
{
    sqlite3_mutex_enter(sqlite3_db_mutex(db));
    char buffer[256] = {};

    const int l = std::snprintf(
        buffer,
        sizeof(buffer),
        "insert into Hosts ('ip', 'name') values(\"%s\", \"%s\")",
        net_name, 
        ip
    );

    assert(l < (int)sizeof(buffer));

    int ret = sqlite3_exec(db, buffer, NULL, NULL, NULL);

    if (ret) {
        std::printf("Error creating '%s' already exists...\n", net_name);
        msg->type = Ip_Msg::INVALID;
    }
    else {
        msg->type = Ip_Msg::RESPONSE;
        std::strncpy(
            msg->response.ip, 
            ip, 
            SESSION_KEY_LEN
        );
    }
    sqlite3_mutex_leave(sqlite3_db_mutex(db));
}

void Hosts::end()
{
    c_th.join();
    sqlite3_close(db);
}
