#include "quickshare.h"
#include "gui.h"
#include "networking.h"

int main(int argc, const char *argv[]) {
    Client_Sock client("192.168.1.31", 5000);
    if (!client.init_socket()) {
        LOGGER("Failed to init socket\n");
        exit(1);
    }
    Context ctx(&client);

    ctx.create_window(1000, 720, "Quickshare");
    ctx.init_imgui();
    ctx.main_loop();

    return 0;
}