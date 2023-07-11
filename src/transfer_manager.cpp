#include "transfer_manager.hpp"

Transfer_Manager::Transfer_Manager() : 
    client_transfer_ids(1),
    dirty(false)
{}

bool Transfer_Manager::create_host_request(
    const char* path,
    const Transfer_ID t_id[TRANSFER_CLIENTS_MAX]
)
{
    Transfer transfer(TRANSFER_HOST);

    transfer.session = f_manager.read_file(path);
    if (!transfer.session)
        return false;

    transfer.client_t_id = client_transfer_ids++;
    (void)std::memcpy(
        transfer.t_hdr.to, 
        t_id, 
        sizeof(transfer.t_hdr.to)
    );
    transfer.file_path = path;
    transfer.state = SEND_REQ;

    push_transfer(transfer);

    return true;
}

void Transfer_Manager::create_request(const Transfer_Request* request)
{
    Transfer transfer(TRANSFER_RECV);

    (void)std::memcpy(
        (void*)&transfer.t_hdr,
        (const void*)&request->hdr,
        sizeof(Transfer_Hdr)
    );
    transfer.file_path = request->file_name;
    transfer.state = PENDING;
    transfer.session = f_manager.write_file(request->file_name);

    push_transfer(transfer);
}

bool Transfer_Manager::write_packet(Packet* packet)
{
    std::lock_guard<std::mutex> lock(t_lock);
    for (auto& transfer : transfers)
    {
        if (transfer.type == TRANSFER_HOST)
        {
            switch (transfer.state)
            {
                case SEND_REQ:
                    PACKET_HDR(
                        P_TRANSFER_REQUEST,
                        sizeof(packet->d.request),
                        packet
                    );

                    packet->d.request.client_transfer_id = transfer.client_t_id;
                    (void)std::memcpy(
                        packet->d.request.file_name,
                        transfer.file_path.filename().c_str(),
                        transfer.file_path.filename().string().length()
                    );
                    packet->d.request.file_size = 0;
                    packet->d.request.hdr = transfer.t_hdr;
                    transfer.state = PENDING;             
                    return true;

                case ACTIVE:
                    /* Check if done also */
                    break;

                case PENDING:
                case CANCELLED:
                case ERROR:
                case COMPLETE:
                    break;
            }
        }
    }

    return false;
}