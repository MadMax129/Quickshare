#include "locator.hpp"
#include "config.hpp"

bool Locator::locate(Locator_Conn& lc)
{
    if (!conn.create_socket(STATIC_QS_SERVER_IP, STATIC_QS_SERVER_PORT))
        return false;

    if (!conn.connect())
        return false;

    LOG("Connected to Qs Server...\n");

    // either send to join a session
    // or send to create session 

    // for both read result

    // return error if cannot otherwise return success

}

void Locator::cleanup()
{
}