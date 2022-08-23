#include "ip_server.hpp"

bool Hosts::create_sql()
{
    sqlite3_open(DATABASE_PATH, &db);

    if (db == NULL) {
        sqlite3_close(db);
        return false;
    }

    assert(!sqlite3_exec(
        db,
        "create table if not exists 'Hosts' ("
	        "'name' TEXT,"
	        "'ip' TEXT,"
	       "'time' DATETIME not null default (datetime(CURRENT_TIMESTAMP, 'localtime'))"
        ");",
        NULL, NULL,NULL
    ));

    return true;
}

void Hosts::find_entry(const char* net_name, Ip_Msg* msg)
{
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
                MAX_IP_LEN-1
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
}

void Hosts::create_entry(const char* net_name, const char* ip, Ip_Msg* msg)
{
    char buffer[256] = {};

    const int l = std::snprintf(
        buffer,
        sizeof(buffer),
        "insert into Hosts values(\"%s\", \"%s\")",
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
            MAX_IP_LEN-1
        );
    }
}

void Hosts::end()
{
    sqlite3_close(db);
}
