#include "network.hpp"
#include <algorithm>
#include <iterator>
#include <cstring>

Database::Database()
{
    cleanup();
}

void Database::cleanup()
{
    client_list.fill({});
    client_count = 0;
}

UserId Database::get_id() const 
{
    UserId id;
    while (true) {
        id = time(NULL);

        if (std::find_if(
            std::begin(client_list), 
            std::end(client_list), 
            [&] (const Client& c) { 
                return c.id == id;
            }
        ) == std::end(client_list))
            break;
    }

    return id;
}

Client* Database::new_client(struct sockaddr_in* addr, socket_t sock)
{
    auto cli = std::find_if(
            std::begin(client_list), 
            std::end(client_list),
            [] (const Client &c) { 
                return c.state == Client::EMPTY; 
            }
    );

    cli->id = get_id();
    cli->addr = *addr;
    cli->socket = sock;
    cli->state = Client::OPEN;
    ++client_count;

    LOGF("[%lld] Client created '%s:%d'\n", 
        cli->id,
        inet_ntoa(cli->addr.sin_addr), 
        cli->addr.sin_port);

    assert(cli != std::end(client_list));

    return cli;
}

Client* Database::get_client_by_id(UserId id)
{
    auto cli = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Client& c) { return c.id == id; }
    );

    if (cli == std::end(client_list))
        return NULL;
    else 
        return cli;
}

Client* Database::get_client(socket_t sock)
{
    auto cli = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Client& c) { return c.socket == sock; }
    );

    if (cli == std::end(client_list))
        return NULL;
    else 
        return cli;
}

void Database::remove_client(socket_t sock)
{
    Client* cli = get_client(sock);

    assert(cli);

    LOGF("[%lld] Client \"%s\" disconnected '%s:%d'\n", 
        cli->id, 
        (char*)cli->name, 
        inet_ntoa(cli->addr.sin_addr), 
        cli->addr.sin_port);
    
    std::memset(cli, 0, sizeof(Client));
    --client_count;
}

void Database::create_msg(Msg* msg, const Client* cli)
{
    memset(msg, 0, sizeof(Msg));
    msg->hdr.type = Msg::CLIENT_LIST;
    msg->hdr.recipient_id = cli->id;
    u32 i = 0;
 
    for (const Client& c : client_list) {
        if (c.state == Client::COMPLETE && c.id != cli->id) {
            msg->list.clients[i].id = c.id;
            std::wcsncpy(msg->list.clients[i].name, c.name, CLIENT_NAME_LEN);
            ++i;
        }
    }
    msg->list.client_count = client_count - 1;
}

void Database::debug_clients() const
{
    printf("Client amount: %u\n", client_count);
    for (const Client& cli : client_list) {
        if (cli.state != Client::EMPTY) {
            if (cli.state == Client::OPEN) {
                colored_printf(CL_BLUE, "<...>: #%u\n", cli.id);
                colored_print(CL_YELLOW, "\tState: Open\n");
            
            }
            else if (cli.state == Client::COMPLETE) {
                colored_printf(CL_BLUE, "%s: #%u\n", cli.name, cli.id);
                colored_print(CL_GREEN, "\tState: Completed\n");
            }
            printf("\t'%s:%d'\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port);
        }
    }
}
