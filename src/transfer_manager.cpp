#include <algorithm>
#include <new>

#include "transfer_manager.hpp"

Transfer_Manager::Transfer_Manager() : 
    dirty(false),
    client_transfer_ids(1),
    cmd_queue(TRANSFER_QUEUE_MAX),
    t_count(0)
{
    for (u32 i = 0; i < transfers.size(); i++) {
        transfers.at(i).type = 
            i < (SIM_TRANSFERS_MAX / 2) ? 
            Active_Transfer::TRANSFER_RECV :
            Active_Transfer::TRANSFER_HOST;
    }
}

Active_Transfer* Transfer_Manager::get_transfer(const Active_Transfer::Type type)
{
    auto transfer = std::find_if(
        transfers.begin(),
        transfers.end(),

        [&](const Active_Transfer& t) {
            return t.type == type &&
                   t.state.load(std::memory_order_acquire) == 
                   Active_Transfer::State::EMPTY;
        }
    );

    return transfer != transfers.end() ? 
        transfer : 
        NULL;
}

void Transfer_Manager::process_cmds()
{
    Transfer_Cmd cmd;

    while (cmd_queue.peek(cmd)) {
        LOGF("GOT CMD %d\n", cmd.type);
        cmd_queue.pop();
    }
}

bool Transfer_Manager::server_request(const Transfer_Request* request)
{
    Active_Transfer* const transfer = get_transfer(
        Active_Transfer::Type::TRANSFER_RECV
    );

    if (!transfer)
        return false;

    (void)std::memcpy(
        &transfer->hdr,
        &request->hdr,
        sizeof(Transfer_Hdr)
    );
    transfer->file = request->file_name;
    t_count.fetch_add(1, std::memory_order_relaxed);
    transfer->state.store(
        Active_Transfer::State::PENDING,
        std::memory_order_release
    );
    // transfer.session = f_manager.write_file(request->file_name);

    return true;
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

// bool Transfer_Manager::write_packet(Packet* packet)
// {
//     std::lock_guard<std::mutex> lock(t_lock);
//     for (auto& transfer : transfers)
//     {
//         if (transfer.type == TRANSFER_HOST)
//         {
//             switch (transfer.state)
//             {
//                 case SEND_REQ:
//                     send_req(transfer, packet);       
//                     return true;

//                 case ACTIVE:
//                     LOG("ACCEPTED\n");
//                     /* Check if done also */
//                     break;

//                 case SEND_ACCEPT:
//                 case SEND_DENY:
//                     send_reply(transfer, packet);
//                     break;

//                 case CANCELLED:
//                     LOG("DENIED\n");
//                     break;

//                 case PENDING:
//                 case ERROR:
//                 case COMPLETE:
//                     break;
//             }
//         }
//     }

//     return false;
// }

// void Transfer_Manager::send_req(Transfer& transfer, Packet* packet)
// {
//     PACKET_HDR(
//         P_TRANSFER_REQUEST,
//         sizeof(packet->d.request),
//         packet
//     );

//     packet->d.request.client_transfer_id = transfer.client_t_id;
//     (void)std::memcpy(
//         packet->d.request.file_name,
//         transfer.file_path.filename().c_str(),
//         transfer.file_path.filename().string().length()
//     );
//     packet->d.request.file_size = 0;
//     packet->d.request.hdr = transfer.t_hdr;
//     transfer.state = PENDING;
// }

// // void Transfer_Manager::send_reply(Transfer& transfer, Packet* packet)
// // {
// //     PACKET_HDR(
// //         P_TRANSFER_REQUEST,
// //         sizeof(packet->d.request),
// //         packet
// //     );
// // }