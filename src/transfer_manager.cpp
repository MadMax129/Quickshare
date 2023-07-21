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

void Transfer_Manager::client_request(
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

void Transfer_Manager::client_request_reply(
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
                // TODO: handle client_cancel
            }
            else {
                // add to backlog as cancelled
                // transfer.state.sto
                LOG("TRANSFER DENIED clean it up\n");
            }

            transfer.state.store(
                reply ? 
                    Active_Transfer::State::PENDING : 
                    Active_Transfer::State::EMPTY,
                std::memory_order_release
            );
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
        if (transfer.accept_list[i] == Active_Transfer::EMPTY) {
            all_reply = false;
            break;
        }
    }

    if (all_reply) {

    }
}

bool Transfer_Manager::server_request(const Transfer_Request* request)
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

void Transfer_Manager::request_reply(const Transfer_ID t_id, const bool reply)
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

// // void Transfer_Manager::accept(const Transfer_Type type, 
// //                               const Transfer_ID t_id,
// //                               const bool reply)
// // {
// //     std::lock_guard<std::mutex> lock(t_lock);
// //     auto it = std::find_if(
// //         transfers.begin(),
// //         transfers.end(),

// //         [&](const Transfer& t) {
// //             const Transfer_ID compare_id = 
// //                 type == TRANSFER_RECV ? 
// //                 t.t_hdr.t_id :
// //                 t.client_t_id;

// //             return t.type     == type &&
// //                    compare_id == t_id;
// //         }
// //     );

// //     if (it != transfers.end())
// //         it->state = reply ? SEND_ACCEPT : SEND_DENY;
// // }

void Transfer_Manager::process_cmds()
{
    Transfer_Cmd cmd;
    while (cmd_queue.peek(cmd)) {
        switch (cmd.type)
        {
            case Transfer_Cmd::REQUEST:
                client_request(cmd.d.req.filepath, cmd.d.req.to);
                break;

            case Transfer_Cmd::REPLY:
                request_reply(cmd.t_id, cmd.d.reply);
                break;
                
            case Transfer_Cmd::CANCEL:
                break;
        }
        (void)cmd_queue.pop();
    }
}

bool Transfer_Manager::do_work(Packet* packet)
{
    auto work = [&](Transfer_Array& t_array) -> bool
    {
        for (auto& t : t_array)
        {
            switch (t.state.load(std::memory_order_acquire))
            {
                case Active_Transfer::State::EMPTY:
                case Active_Transfer::State::CANCEL:
                case Active_Transfer::State::PENDING:
                case Active_Transfer::State::GET_RESPONSE:
                case Active_Transfer::State::ACCEPT:
                    break;

                case Active_Transfer::State::ACTIVE:
                case Active_Transfer::State::DENY:
                    t.state.store(
                        Active_Transfer::ACTIVE,
                        std::memory_order_release
                    );
                    break;
                
                case Active_Transfer::State::SEND_REQ:
                    send_req(t, packet);
                    return true;

            }
        }

        return false;
    };

    process_cmds();
    return work(host_transfers);
}

void Transfer_Manager::send_req(Active_Transfer& transfer, Packet* packet)
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

// // void Transfer_Manager::send_reply(Transfer& transfer, Packet* packet)
// // {
// //     PACKET_HDR(
// //         P_TRANSFER_REQUEST,
// //         sizeof(packet->d.request),
// //         packet
// //     );
// // }