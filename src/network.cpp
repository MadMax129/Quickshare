#include "network.hpp"
#include "client_poll.hpp"
#include "users.hpp"
#include "transfer_manager.hpp"

Network::Network() : 
    c_poll(conn),
    msg_queue(SERVER_MSG_QUEUE_SIZE), 
    ssl_ctx(NULL), 
    ssl(NULL)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    state.set(UNINITIALIZED);
}

bool Network::init()
{
    const SSL_METHOD* method = TLS_client_method();

    memset(&rbuf, 0, sizeof(Packet_Buf));
    memset(&wbuf, 0, sizeof(Packet_Buf));

    ssl_ctx = SSL_CTX_new(method);
    rbuf.packet = (Packet*)alloc(sizeof(Packet));
    wbuf.packet = (Packet*)alloc(sizeof(Packet));

    if (!ssl_ctx     ||
        !rbuf.packet ||
        !wbuf.packet || 
        !conn.create_socket(QS_SERVER_IP, QS_SERVER_PORT)
    ) {
        state.set(INIT_ERROR);
        return false;
    }

    conn.set_non_blocking();

    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);

    return true;
}

bool Network::tcp_connect()
{
    if (conn.connect())
        return true;

    fd_set w_set;

#ifdef SYSTEM_UNX
    if (errno == EINPROGRESS || 
        errno == EAGAIN
    ) {
        FD_ZERO(&w_set);
        FD_SET(conn.me(), &w_set);

        struct timeval timeout;
        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;

        const int n_events = select(
            conn.me() + 1, 
            NULL, 
            &w_set, 
            NULL, 
            &timeout
        );

        if (n_events > 0 && FD_ISSET(conn.me(), &w_set)) {
            int optval;
            socklen_t optlen = sizeof(optval);
            getsockopt(
                conn.me(), 
                SOL_SOCKET, 
                SO_ERROR, 
                &optval, 
                &optlen
            );

            if (optval == 0)
                return true;
        }
    }
#else
#error "NOT DONE"
#endif
    
    /* 
        Fall through either 
        timeout or failed connection 
    */
    return false;
}

bool Network::tls_connect()
{
    int ret = SSL_do_handshake(ssl);
    
    while (ret <= 0) {
        const int err = SSL_get_error(ssl, ret);
        
        if (err == SSL_ERROR_WANT_READ || 
            err == SSL_ERROR_WANT_WRITE
        ) {
            fd_set readfds, writefds;
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(conn.me(), &readfds);
            FD_SET(conn.me(), &writefds);

            const int n_events = select(
                conn.me() + 1, 
                &readfds, 
                &writefds, 
                NULL, 
                NULL
            );
            
            if (n_events > 0 && 
                (FD_ISSET(conn.me(), &readfds) || 
                FD_ISSET(conn.me(), &writefds))
            ) {
                ret = SSL_do_handshake(ssl);
            }
            else {
                return false;
            }
        } 
        else {
            return false;
        }
    }
    
    return true;
}

bool Network::init_conn(Status& active)
{
    enum {
        INIT,
        ERROR,
        TCP_CONNECT,
        TLS_CONNECT
    } conn_state = INIT;

    while (active.get()) 
    {
        switch (conn_state)
        {
            case INIT: {
                /* Init SSL obj */
                ssl = SSL_new(ssl_ctx);
                if (!ssl) {
                    state.set(INIT_ERROR);
                    return false;
                }
                SSL_set_connect_state(ssl);
                SSL_set_fd(ssl, conn.me());
                conn_state = TCP_CONNECT;
                break;
            }

            case TCP_CONNECT: {
                if (tcp_connect()) {
                    conn_state = TLS_CONNECT;
                }
                else {
                    sleep(1);
                }
                break;
            }

            case TLS_CONNECT: {
                if (tls_connect()) {
                    state.set(CONNECTED);
                    return true;
                }
                else {
                    conn_state = ERROR;
                }
                break;
            }

            case ERROR: {
                conn_state = INIT;
                P_ERROR("TLC Connect failed\n");
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                ssl = NULL;
                sleep(1);
                break;
            };
        }
    }

    return true;
}

void Network::clean()
{
    if (ssl)
        SSL_free(ssl);

    if (ssl_ctx) 
        SSL_CTX_free(ssl_ctx);

    if (conn.me()) 
        CLOSE_SOCKET(conn.me());

    state.set(UNINITIALIZED);
    ssl     = NULL;
    ssl_ctx = NULL;
}

void Network::start()
{
    assert(thread_manager.new_thread(&Network::server_loop, this));
    state.set(State::OPENED);
}

void Network::session(const char name[PC_NAME_MAX_LEN], 
                      const char s_id[SESSION_ID_MAX_LEN],
                      const bool opt)
{
    const State current_state = state.get();
    const bool can_queue = 
        current_state == UNINITIALIZED || 
        current_state == SESSION_ERROR;
    
    /* Start network */
    if (current_state == UNINITIALIZED)
        start();
    
    if (can_queue) {
        Session msg(
            name,
            s_id,
            opt
        );
   
        // think over TODO:
        assert(msg_queue.push(msg));
        state.set(State::CONNECTED);
    }
}

void Session::to_packet(Packet* packet)
{
    PACKET_HDR(
        P_CLIENT_INTRO,
        sizeof(packet->d.intro),
        packet
    );

    safe_strcpy(
        packet->d.intro.id, 
        s_id, 
        SESSION_ID_MAX_LEN
    );

    safe_strcpy(
        packet->d.intro.name, 
        name, 
        PC_NAME_MAX_LEN
    );

    packet->d.intro.session  = opt;
}

bool Network::convert_msg()
{
    Packet* packet = wbuf.packet;
    Session msg;

    const Transfer_Manager::Work_State t_work = 
        Transfer_Manager::get_instance().do_work(packet);

    if (t_work == Transfer_Manager::NO_WORK) {
        if (msg_queue.peek(msg)) {
            msg.to_packet(packet);
            msg_queue.pop();
        }
        else {
            return false;
        }
    }
    else if (t_work == Transfer_Manager::WAIT_WORK) {
        wbuf.len  = 0;
        wbuf.size = 0;
        return true;
    }

    wbuf.len  = 0;
    wbuf.size = 
        packet->hdr.size + 
        sizeof(Packet_Hdr);

    return true;
}

void Network::write_data()
{
    if ((wbuf.size - wbuf.len) != 0) {
        const int nbytes = SSL_write(
            ssl,
            wbuf.packet + wbuf.len,
            wbuf.size   - wbuf.len 
        );

        if (nbytes > 0) {
            wbuf.len += nbytes;

            if (wbuf.len == wbuf.size) {
                wbuf.len  = 0;
                wbuf.size = 0;
            }
        }
        else {
            P_ERRORF(
                "< 0 SSL_write %d\n", 
                SSL_get_error(ssl, nbytes) == SSL_ERROR_WANT_WRITE
            ); 
            assert(false);
        }
    }
}

void Network::read_data()
{
    do {
        const int nbytes = SSL_read(
            ssl,
            (char*)rbuf.packet  + rbuf.len,
            (sizeof(Packet_Hdr) + rbuf.size) - rbuf.len
        );

        if (nbytes > 0) {
            rbuf.len += nbytes;

            if (rbuf.len == sizeof(Packet_Hdr)) {
                const Packet_Hdr* hdr = (Packet_Hdr*)rbuf.packet;
                rbuf.size = hdr->size;
            }

            if (rbuf.len == (rbuf.size + sizeof(Packet_Hdr))) {
                analize();
                rbuf.size = 0;
                rbuf.len  = 0;
            }
        }
        else {
            const i32 err = SSL_get_error(ssl, nbytes);
            if (err == SSL_ERROR_ZERO_RETURN ||
                err == SSL_ERROR_SSL         ||
                err == SSL_ERROR_SYSCALL
            ) {
                P_ERROR("Server disconnect\n");
                assert(false);
                break;
            }

            if (err == SSL_ERROR_WANT_WRITE ||
                err == SSL_ERROR_WANT_READ
            ) {
                LOG("read WANT MORE\n");
                break;
            }
        }
    } while (SSL_pending(ssl));
}

void Network::analize()
{
    switch (rbuf.packet->hdr.type)
    {
        case P_SERVER_OK:
        case P_SERVER_DENY:
            server_response();
            break;
    
        case P_SERVER_NEW_USERS:
        case P_SERVER_DEL_USERS:
            update_users();
            break;

        case P_TRANSFER_REQUEST:
            assert(Transfer_Manager::get_instance()
                .create_recv_request(
                    &rbuf.packet->d.request
                ));
            // handel if failed
            break;
        
        case P_TRANSFER_VALID:
        case P_TRANSFER_INVALID:
            Transfer_Manager::get_instance()
                .host_request_valid(
                    &rbuf.packet->d.request,
                    rbuf.packet->hdr.type == P_TRANSFER_VALID
                );
            break;
        
        case P_TRANSFER_CANCEL:
            Transfer_Manager::get_instance()
                .recv_cancel(
                    &rbuf.packet->d.transfer_state.hdr
                );
            break;

        case P_TRANSFER_REPLY:
            Transfer_Manager::get_instance()
                .host_request_reply(
                    rbuf.packet->d.transfer_reply.hdr.t_id,
                    rbuf.packet->d.transfer_reply.hdr.from,
                    rbuf.packet->d.transfer_reply.accept
                );
            break;

        case P_TRANSFER_DATA:
            Transfer_Manager::get_instance()
                .recv_data(
                    &rbuf.packet->d.transfer_data
                );
            break;

        case P_TRANSFER_COMPLETE:
            Transfer_Manager::get_instance()
                .recv_complete(
                    rbuf.packet->d.transfer_state.hdr.t_id
                );
            break;

        default:
            P_ERRORF(
                "Unknown server pacekt type '%d'\n",
                rbuf.packet->hdr.type 
            );
            break;
    }
}

void Network::server_response()
{
    const Packet* const p = rbuf.packet;

    if (p->hdr.type == P_SERVER_OK)
        state.set(State::SESSION_SUCCESS);
    else 
        state.set(State::SESSION_ERROR);
}

void Network::update_users()
{
    const Packet* const p = rbuf.packet;

    if (p->hdr.type == P_SERVER_NEW_USERS)
        User_List::get_instance().add_users(p);
    else 
        User_List::get_instance().remove_user(p);
}

void Network::check_write()
{
    /* Unset WRITE */
    // double check logic
    c_poll.get_events() &= ~EVENT_WRITE;

    if (wbuf.len == 0 && convert_msg())
        c_poll.get_events() |= EVENT_WRITE;
}

void Network::handle(Status& active)
{
    c_poll.set_socket(conn.me());
    c_poll.set_events(Poll_Event::EVENT_READ | 
                      Poll_Event::EVENT_ERROR);

    while (active.get())
    {
        check_write();

        const i32 n_ready = c_poll.poll(20);

        if (n_ready < 0 || 
            c_poll.is_set(EVENT_ERROR)
        ) {
            P_ERROR("Network Error\n");
            state.set(NET_ERROR);
            return;
        }

        if (c_poll.is_set(EVENT_READ))
            read_data();
        
        if (c_poll.is_set(EVENT_WRITE))
            write_data();
    }
}

void Network::server_loop(Status& active)
{
    while (active.get()) {
        if (!init())
            return;

        if (init_conn(active))
            handle(active);

        if (state.get() == NET_ERROR)
            return;

        /* Exited on error */
        clean();
    }
}