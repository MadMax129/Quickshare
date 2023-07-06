#include "transfer_manager.hpp"

Transfer_Manager::Transfer_Manager()
{
    client_transfer_ids = 0xf;
}

void Transfer_Manager::create_request()
{
    Transfer transfer(TRANSFER_HOST);
    transfer.client_t_id = client_transfer_ids++;

    transfers.push_back(transfer);
}

void Transfer_Manager::create_request(const Transfer_ID t_id)
{
    Transfer transfer(TRANSFER_RECV);

    transfer.t_hdr.t_id = t_id;
    transfers.push_back(transfer);
}
