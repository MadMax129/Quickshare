#include <algorithm>
#include <new>

#include "transfer_manager.hpp"

Transfer_Manager::Transfer_Manager() : 
    dirty(false),
    client_transfer_ids(1),
    cmd_queue(TRANSFER_QUEUE_MAX),
    recv_transfers{},
    host_transfers{}
{}

Active_Transfer* Transfer_Manager::get_transfer(Transfer_Array& t_array)
{
    auto transfer = std::find_if(
        t_array.begin(),
        t_array.end(),

        [&](const Active_Transfer& t) {
            return t.state.load(std::memory_order_acquire) == 
                Active_Transfer::State::EMPTY;
        }
    );

    return transfer != t_array.end() ? 
        transfer : 
        NULL;
}

void Transfer_Manager::cmd_host_request(
    const char* path,
    const Client_ID c_ids[TRANSFER_CLIENTS_MAX]
)
{
    Active_Transfer* const transfer = get_transfer(
        host_transfers
    );

    if (!transfer)
        return;
    
    // transfer.session = f_manager.read_file(path);
    // if (!transfer.session)
    //     return false;

    transfer->local_id = client_transfer_ids++;
    (void)std::memcpy(
        transfer->hdr.to,
        c_ids, 
        sizeof(transfer->hdr.to)
    );
    transfer->file = path;
    transfer->state.store(
        Active_Transfer::State::SEND_REQ, 
        std::memory_order_release
    );
}

void Transfer_Manager::host_request_valid(
    const Transfer_Request* req, 
    const bool reply
)
{
    /* Client sends transfer request
        server call client_request_reply
        to confirm or deny the request
    */
    for (auto& transfer : host_transfers) {
        if (
            transfer.state.load(std::memory_order_acquire) == 
                Active_Transfer::State::GET_RESPONSE && 
            transfer.local_id == req->client_transfer_id
        ) {
            if (reply) {
                (void)std::memcpy(
                    &transfer.hdr,
                    &req->hdr,
                    sizeof(Transfer_Hdr)
                );
                transfer.state.store(
                    Active_Transfer::State::PENDING,
                    std::memory_order_release
                );
            }
            else {
                host_cleanup(transfer);
            }

            return;
        }
    }
}

void Transfer_Manager::set_request(
    Active_Transfer& transfer,
    const Client_ID c_id,
    const bool reply
) 
{
    for (u32 i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        if (transfer.hdr.to[i] == c_id) {
            transfer.accept_list[i] = reply ? 
                Active_Transfer::ACCEPT : 
                Active_Transfer::DENY;
            break;
        }
    }

    bool all_reply = true;
    for (u32 i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        if (transfer.hdr.to[i] == 0) {
            break;
        }
        else if (transfer.accept_list[i] == Active_Transfer::EMPTY) {
            all_reply = false;
            break;
        }
    }

    if (all_reply) {
        transfer.state.store(
            Active_Transfer::ACTIVE, 
            std::memory_order_release
        );
    }
}

bool Transfer_Manager::create_recv_request(const Transfer_Request* request)
{
    Active_Transfer* const transfer = get_transfer(
        recv_transfers
    );

    if (!transfer)
        return false;

    (void)std::memcpy(
        &transfer->hdr,
        &request->hdr,
        sizeof(Transfer_Hdr)
    );
    transfer->file = request->file_name;
    transfer->state.store(
        Active_Transfer::State::GET_RESPONSE,
        std::memory_order_release
    );
    // transfer.session = f_manager.write_file(request->file_name);

    return true;
}

void Transfer_Manager::host_request_reply(
    const Transfer_ID t_id, 
    const Client_ID c_id, 
    const bool reply
)
{
    for (auto& t : host_transfers) {
        if (t.state.load(std::memory_order_acquire) == Active_Transfer::PENDING && 
            t.hdr.t_id == t_id
        ) {
            set_request(t, c_id, reply);
            break;
        }
    }
}

void Transfer_Manager::cmd_recv_request_reply(const Transfer_ID t_id, const bool reply)
{
    for (auto& t : recv_transfers) {
        if (t.state.load(std::memory_order_acquire) == 
                Active_Transfer::GET_RESPONSE &&
            t.hdr.t_id == t_id
        ) {
            t.state.store(
                reply ? 
                    Active_Transfer::ACCEPT :
                    Active_Transfer::DENY,
                std::memory_order_release
            );
            return;
        }
    }
}

void Transfer_Manager::process_cmds()
{
    Transfer_Cmd cmd;
    while (cmd_queue.peek(cmd)) {
        switch (cmd.type)
        {
            case Transfer_Cmd::REQUEST:
                cmd_host_request(cmd.d.req.filepath, cmd.d.req.to);
                break;

            case Transfer_Cmd::REPLY:
                cmd_recv_request_reply(cmd.t_id, cmd.d.reply);
                break;
                
            case Transfer_Cmd::CANCEL:
                break;
        }
        (void)cmd_queue.pop();
    }
}

bool Transfer_Manager::do_work(Packet* packet)
{
    process_cmds();
    for (auto& t : host_transfers)
    {
        switch (t.state.load(std::memory_order_acquire))
        {
            case Active_Transfer::EMPTY:
            case Active_Transfer::GET_RESPONSE:
            case Active_Transfer::DENY:
            case Active_Transfer::PENDING:
                break;
            
            case Active_Transfer::ACCEPT:
            case Active_Transfer::CANCEL:
            case Active_Transfer::ACTIVE:
                break;
            
            case Active_Transfer::SEND_REQ:
                send_request(t, packet);
                return true;
        }
    }

    for (auto& t : recv_transfers) 
    {
        switch (t.state.load(std::memory_order_acquire))
        {
            case Active_Transfer::EMPTY:
            case Active_Transfer::GET_RESPONSE:
            case Active_Transfer::CANCEL:
            case Active_Transfer::PENDING:
            case Active_Transfer::ACTIVE:
            case Active_Transfer::SEND_REQ:
                break;

            case Active_Transfer::ACCEPT:
            case Active_Transfer::DENY:
                send_recv_request_reply(t, packet);
                return true;
        }
    }
    return false;
}

void Transfer_Manager::host_cleanup(Active_Transfer& transfer)
{
    const Active_Transfer::State state = transfer.state.load(std::memory_order_acquire);

    if (state != Active_Transfer::GET_RESPONSE) {
        // send CANCEL_TRANSFER
    }

    transfer.state.store(Active_Transfer::EMPTY, std::memory_order_release);

    (void)memset(
        transfer.accept_list,
        0,
        sizeof(transfer.accept_list)
    );
    (void)memset(
        &transfer.hdr,
        0,
        sizeof(transfer.hdr)
    );
    transfer.local_id = 0;
}

void Transfer_Manager::recv_cleanup()
{

}

void Transfer_Manager::send_request(Active_Transfer& transfer, Packet* packet)
{
    PACKET_HDR(
        P_TRANSFER_REQUEST,
        sizeof(packet->d.request),
        packet
    );

    packet->d.request.client_transfer_id = transfer.local_id;
    (void)std::memcpy(
        packet->d.request.file_name,
        transfer.file.filename().c_str(),
        transfer.file.filename().string().length()
    );
    packet->d.request.file_size = 0;
    packet->d.request.hdr = transfer.hdr;
    transfer.state.store(
        Active_Transfer::State::GET_RESPONSE,
        std::memory_order_release
    );
}

void Transfer_Manager::send_recv_request_reply(Active_Transfer& transfer, Packet* packet)
{
    PACKET_HDR(
        P_TRANSFER_REPLY,
        sizeof(packet->d.transfer_reply),
        packet
    );

    packet->d.transfer_reply.accept = 
        transfer.state.load(std::memory_order_acquire) == 
            Active_Transfer::ACCEPT ?
            1 : 0;

    packet->d.transfer_reply.hdr.t_id = transfer.hdr.t_id; 
    transfer.state.store(
        Active_Transfer::State::ACTIVE,
        std::memory_order_release
    );
}