/** @file network.hpp
 * 
 * @brief Network definition
 *      
 * Copyright (c) 2022 Maks S.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <variant>

#include "state.hpp"
#include "connection.hpp"
#include "msg.h"
#include "../lib/LockFreeQueueCpp11.h"
#include <sys/select.h>
#include "thread_manager.hpp"

#define SERVER_MSG_QUEUE_SIZE 6

struct Server_Msg {
    enum Type {
        SESSION_KEY,
        TRANSFER_REQ
    };

    struct Session_Key {
        char name[PC_NAME_MAX_LEN];
        char s_id[SESSION_ID_MAX_LEN]; 
    };

    Server_Msg(Type type) : type(type) {}
    
    Type type;
    std::variant<Session_Key> data;
};

using Server_Queue = LockFreeQueueCpp11<Server_Msg>;

class Network {
public:
    enum State {
        UNINITIALIZED,
        INIT_ERROR,
        NET_ERROR,
        OPENED,
        CONNECTED,
        SESSION_ERROR,
        SESSION_SUCCESS
    };

    static Network& getInstance() {
        static Network instance;
        return instance;
    }

    inline State get_state() {
        return state.get();
    }

    void start(const char name[PC_NAME_MAX_LEN], 
               const char s_id[SESSION_ID_MAX_LEN]);
private:
    Network();
    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;

    bool init();
    void clean();
    bool init_conn(Status& active);
    void server_loop(Status& active);

    Connection<Packet> conn;
    State_Manager<State> state;
    Server_Queue msg_queue;
    fd_set read_set, write_set;
    SSL_CTX* ssl_ctx;
    SSL* ssl;
};