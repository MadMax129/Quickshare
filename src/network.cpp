#include "network.hpp"

Network::Network() : msg_queue(SERVER_MSG_QUEUE_SIZE), ssl_ctx(NULL), ssl(NULL)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    state.set(UNINITIALIZED);
}

bool Network::init()
{
    const SSL_METHOD* method = TLS_client_method();

    ssl_ctx = SSL_CTX_new(method);

    if (!ssl_ctx) {
        state.set(INIT_ERROR);
        return false;
    }

    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);

    return true;
}

bool Network::init_conn(Status& active)
{
    while (active.get()) {
        ssl = SSL_new(ssl_ctx);

        if (!ssl || 
            !conn.create_socket(QS_SERVER_IP, QS_SERVER_PORT)
        ) {
            state.set(INIT_ERROR);
            return false;
        }

        SSL_set_fd(ssl, conn.me());

        if (conn.connect()) {
            if (SSL_connect(ssl) > 0) {
                state.set(CONNECTED);
                break;
            }
        }

        P_ERROR("Connect failed\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        conn.close();
        sleep(1);
    }

    return true;
}

void Network::clean()
{
    if (ssl) SSL_free(ssl);
    if (ssl_ctx) SSL_CTX_free(ssl_ctx);
    if (conn.me()) CLOSE_SOCKET(conn.me());
    ssl = NULL;
    ssl_ctx = NULL;
}

void Network::start(const char name[PC_NAME_MAX_LEN], 
                    const char s_id[SESSION_ID_MAX_LEN])
{
    const State current_state = state.get();
    if (current_state == UNINITIALIZED) {
        assert(thread_manager.new_thread(&Network::server_loop, this));
        state.set(State::OPENED);
    }

    if (current_state == OPENED || 
        current_state == SESSION_ERROR
    ) {
        return;
    }

    (void)name;
    (void)s_id;
}

void Network::server_loop(Status& active)
{
    if (!init() || !init_conn(active))
        return;

    (void)active;
}