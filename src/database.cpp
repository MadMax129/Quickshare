#include "database.hpp"
#include <algorithm>
#include <iterator>

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
            [&] (const Slot& c) { 
                return c.id == id;
            }
        ) == std::end(client_list))
            break;
    }

    return id;
}