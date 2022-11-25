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

#include "s_msg.hpp"
#include "s_net.hpp"
#include "c_net.hpp"
#include "locator.hpp"
#include "state.hpp"
#include "connection.hpp"
#include "thread_manager.hpp"
#include <thread>

extern Thread_Manager thread_manager;

using Net_Con = Connection<Server_Msg>;

class Network {
public:
    enum State {
        /* Idle default state */
        INACTIVE,
        /* Initilization to server unsuccessful */
        INIT_FAILED,
    };

    Network(Locator& loc);

    static bool get_ip(char* ip_buffer);

    void init_network(bool is_server);

    State_Manager<State> state;
    Net_Con conn;
    const Locator& loc;

private:
    void loop(bool is_server, Status& status);
    bool conn_setup(bool is_server);
    
    Client client;
    Server server;

    std::thread network_thread;
};
