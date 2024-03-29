#include <algorithm>
#include <new>

#include "transfer_manager.hpp"

Transfer_Manager::Transfer_Manager() : 
    dirty(false),
    client_transfer_ids(1),
    cmd_queue(TRANSFER_QUEUE_MAX)
{
    for (Active_Transfer& t : host_transfers)
        t.f_manager.init_read();

    for (Active_Transfer& t : recv_transfers)
        t.f_manager.init_write();
}

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

    if (!transfer || 
        !transfer->f_manager.read_file(path, transfer->file_size)
    ) {
        P_ERROR("FAILED TO HOST TRANSFER\n");
        return;
    }
    
    transfer->local_id = client_transfer_ids++;
    (void)std::memcpy(
        transfer->hdr.to,
        c_ids, 
        sizeof(transfer->hdr.to)
    );
    transfer->past_send = std::chrono::high_resolution_clock::now();
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
            }

            transfer.state.store(
                reply ? 
                    Active_Transfer::State::PENDING : 
                    Active_Transfer::State::CANCEL,
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

    bool all_reply  = true;
    bool all_cancel = true;
    for (u32 i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        if (transfer.hdr.to[i] == 0)
            break;
        if (transfer.accept_list[i] != Active_Transfer::DENY)
            all_cancel = false;
        if (transfer.accept_list[i] == Active_Transfer::EMPTY)
            all_reply = false;
    }

    if (all_cancel) {
        transfer.state.store(
            Active_Transfer::CANCEL, 
            std::memory_order_release
        );
    }
    else if (all_reply) {
        transfer.state.store(
            Active_Transfer::ACTIVE, 
            std::memory_order_release
        );
    }
}

bool Transfer_Manager::create_recv_request(
    const Transfer_Request* request
)
{
    Active_Transfer* const transfer = get_transfer(
        recv_transfers
    );

    if (
        !transfer ||
        !transfer->f_manager.write_file(
            request->file_name, 
            request->file_size
        )
    ) {
        return false;
    }

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

    return true;
}

void Transfer_Manager::recv_cancel(const Transfer_Hdr* hdr)
{
    auto search = [&] (Transfer_Array& t_array)
    {
        return std::find_if(
            t_array.begin(),
            t_array.end(),
            [&] (const Active_Transfer& t) {
                return t.can_cancel() &&
                       hdr->t_id == t.hdr.t_id;
            }
        );
    };

    auto t1 = search(recv_transfers);
    auto t2 = search(host_transfers);

    if (t1 != recv_transfers.end()) {
        zero_transfer(*t1);
        push_backlog(Transfer_Info::CANCELLED, t1->file);
    }
    else if (t2 != host_transfers.end()) {
        set_request(*t2, hdr->from, false);
    }
    else {
        P_ERRORF(
            "ERROR CANCEL T_ID: %ld\n", 
            hdr->t_id
        );
    }
}

void Transfer_Manager::host_request_reply(
    const Transfer_ID t_id, 
    const Client_ID c_id, 
    const bool reply
)
{
    for (auto& t : host_transfers) {
        if (
            t.state.load(std::memory_order_acquire) == 
                Active_Transfer::PENDING && 
            t.hdr.t_id == t_id
        ) {
            set_request(t, c_id, reply);
            break;
        }
    }
}

void Transfer_Manager::cmd_recv_request_reply(
    const Transfer_ID t_id, 
    const bool reply
)
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

void Transfer_Manager::cmd_cancel(const Transfer_ID t_id)
{
    auto search = [&] (Transfer_Array& t_array)
    {
        return std::find_if(
            t_array.begin(),
            t_array.end(),
            [&] (const Active_Transfer& t) {
                return t.can_cancel() &&
                       t_id == t.hdr.t_id;
            }
        );
    };

    auto t1 = search(recv_transfers);
    auto t2 = search(host_transfers);

    if (t1 == recv_transfers.end() &&
        t2 == host_transfers.end()
    ) {
        P_ERRORF("CANCEL REQ '%ld' INVALID\n", t_id);
        return;
    }

    auto transfer = t1 != recv_transfers.end() ? t1 : t2;

    transfer->state.store(
        Active_Transfer::CANCEL,
        std::memory_order_release
    );
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
                cmd_cancel(cmd.t_id);
                break;
        }
        (void)cmd_queue.pop();
    }
}

Transfer_Manager::Work_State 
Transfer_Manager::host_transfer_work(Packet* packet)
{
    for (auto& t : host_transfers)
    {
        switch (t.state.load(std::memory_order_acquire))
        {
            case Active_Transfer::EMPTY:
            case Active_Transfer::GET_RESPONSE:
            case Active_Transfer::DENY:
            case Active_Transfer::PENDING:
            case Active_Transfer::ACCEPT:
            default:
                break;
            
            case Active_Transfer::ACTIVE:
                return send_data(t, packet);

            case Active_Transfer::CANCEL:
                send_cancel(t, packet);
                return HAS_WORK;
            
            case Active_Transfer::SEND_REQ:
                send_request(t, packet);
                return HAS_WORK;
        }
    }

    return NO_WORK;
}

Transfer_Manager::Work_State 
Transfer_Manager::recv_transfer_work(Packet* packet)
{
    for (auto& t : recv_transfers) 
    {
        switch (t.state.load(std::memory_order_acquire))
        {
            case Active_Transfer::EMPTY:
            case Active_Transfer::GET_RESPONSE:
            case Active_Transfer::PENDING:
            case Active_Transfer::SEND_REQ:
            case Active_Transfer::ACTIVE:
                break;

            case Active_Transfer::CANCEL:
                send_cancel(t, packet);
                return HAS_WORK;

            case Active_Transfer::ACCEPT:
            case Active_Transfer::DENY:
                send_recv_request_reply(t, packet);
                return HAS_WORK;
        }
    }

    return NO_WORK;
}

Transfer_Manager::Work_State Transfer_Manager::do_work(Packet* packet)
{
    process_cmds();

    const Work_State host = host_transfer_work(packet);
    
    return host != NO_WORK ? 
        host : 
        recv_transfer_work(packet);
}

void Transfer_Manager::recv_data(const Transfer_Data* t_data)
{
    for (auto& t : recv_transfers) {
        if (
            t.state.load(std::memory_order_acquire) == 
                Active_Transfer::ACTIVE && 
            t_data->t_id == t.hdr.t_id
        ) {
            handle_transfer_data(t, t_data);
            break;
        }
    }
}

void Transfer_Manager::recv_complete(const Transfer_ID t_id)
{
    for (auto& t : recv_transfers) {
        if (
            t.state.load(std::memory_order_acquire) == 
                Active_Transfer::ACTIVE && 
            t_id == t.hdr.t_id
        ) {
            t.f_manager.close();
            zero_transfer(t);
            push_backlog(Transfer_Info::COMPLETE, t.file);
            break;
        }
    }
}

void Transfer_Manager::handle_transfer_data(
    Active_Transfer& transfer,
    const Transfer_Data* t_data
)
{
    const File_Manager::State state = 
        transfer.f_manager.write_work(t_data);

    switch (state)
    {
        case File_Manager::WORKING:
        case File_Manager::COMPLETE:
            break;

        case File_Manager::EMPTY:
        case File_Manager::ERROR:
            break;
    }
}

void Transfer_Manager::zero_transfer(Active_Transfer& transfer)
{
    transfer.state.store(
        Active_Transfer::EMPTY,
        std::memory_order_release
    );
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

Transfer_Manager::Work_State 
Transfer_Manager::send_data(
    Active_Transfer& transfer, 
    Packet* packet
)
{
    const std::chrono::milliseconds elapsed_duration = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() -
            transfer.past_send
        );
    
    const i64 elapsed_time = elapsed_duration.count();

    if (elapsed_time < 1)
        return WAIT_WORK;

    const File_Manager::State state =  
        transfer.f_manager.read_work(packet);
    
    switch (state)
    {
        case File_Manager::WORKING: {
            packet->d.transfer_data.t_id = transfer.hdr.t_id;
            break;
        }

        case File_Manager::COMPLETE: {
            LOG("SEND TRANSFER COMPLETE\n");
            PACKET_HDR(
                P_TRANSFER_COMPLETE,
                sizeof(packet->d.transfer_state),
                packet
            );
            packet->d.transfer_state.hdr.t_id = transfer.hdr.t_id;
            transfer.f_manager.close();
            zero_transfer(transfer);
            push_backlog(Transfer_Info::COMPLETE, transfer.file);
            break;
        }

        case File_Manager::ERROR:
        case File_Manager::EMPTY: {
            send_cancel(transfer, packet);
            break;
        }   
    }

    transfer.past_send = std::chrono::high_resolution_clock::now();

    return HAS_WORK;
}

void Transfer_Manager::send_cancel(
    Active_Transfer& transfer, 
    Packet* packet
)
{
    PACKET_HDR(
        P_TRANSFER_CANCEL,
        sizeof(packet->d.transfer_state),
        packet
    );

    (void)std::memcpy(
        &packet->d.transfer_state.hdr,
        &transfer.hdr,
        sizeof(Transfer_Hdr)
    );

    zero_transfer(transfer);
    push_backlog(Transfer_Info::CANCELLED, transfer.file);
}

void Transfer_Manager::send_request(
    Active_Transfer& transfer, 
    Packet* packet
)
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
        std::min(
            transfer.file.filename().string().length() + 1,
            sizeof(packet->d.request.file_name)
        )
    );

    if (
        (transfer.file.filename().string().length() + 1) >= 
        sizeof(packet->d.request.file_name)
    ) {
        packet->d.request.file_name[
            sizeof(packet->d.request.file_name) - 1
        ] = '\0';
    }
    packet->d.request.file_size = transfer.file_size;
    packet->d.request.hdr = transfer.hdr;
    transfer.state.store(
        Active_Transfer::State::GET_RESPONSE,
        std::memory_order_release
    );
}

void Transfer_Manager::send_recv_request_reply(
    Active_Transfer& transfer, 
    Packet* packet
)
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

    if (packet->d.transfer_reply.accept) {
        transfer.state.store(
            Active_Transfer::State::ACTIVE, 
            std::memory_order_release
        );
    }
    else {
        zero_transfer(transfer);
        push_backlog(Transfer_Info::DENIED, transfer.file);
    }
}