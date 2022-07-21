#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <swicc/swicc.h>
#include <sys/socket.h>
#include <unistd.h>

static swicc_net_logger_ft logger_default;
static void logger_default(char const *const fmt, ...)
{
#ifdef DEBUG
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    printf("\n");
    va_end(argptr);
#endif
}
swicc_net_logger_ft *logger = logger_default;

/**
 * @brief Send a message to a given socket.
 * @param sock The socket where the message will be sent.
 * @param msg Message to send.
 * @return Number of byte that were sent on success, -1 on failure.
 */
static swicc_ret_et msg_send(int32_t const sock,
                             swicc_net_msg_st const *const msg)
{
    if (sock < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.", sock);
        return SWICC_RET_PARAM_BAD;
    }

    if (msg->hdr.size > sizeof(msg->data))
    {
        logger("Message header indicates a data size larger than the buffer "
               "itself.");
        return SWICC_RET_PARAM_BAD;
    }

    /* Safe cast since the target type can fit the sum of cast ones. */
    uint32_t const size_msg =
        (uint32_t)sizeof(swicc_net_msg_hdr_st) + msg->hdr.size;
    int64_t const sent_bytes = send(sock, &msg->hdr, size_msg, 0U);
    if (sent_bytes == size_msg)
    {
        /* Success. */
    }
    else if (sent_bytes < 0)
    {
        logger("Call to send() failed: %s.", strerror(errno));
    }
    else
    {
        logger("Failed to send message.");
        return SWICC_RET_ERROR;
    }

    if (sent_bytes != size_msg)
    {
        logger("Failed to send all the message bytes.");
        return SWICC_RET_ERROR;
    }

    return SWICC_RET_SUCCESS;
}

/**
 * @brief Receive a message from a given socket.
 * @param sock The socket from which to receive a message.
 * @param msg Where to write the received message.
 * @return Number of received bytes on success, -1 on failure.
 */
static swicc_ret_et msg_recv(int32_t const sock, swicc_net_msg_st *const msg)
{
    if (sock < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.", sock);
        return SWICC_RET_PARAM_BAD;
    }

    bool recv_failure = false;
    int64_t recvd_bytes;
    recvd_bytes = recv(sock, &msg->hdr, sizeof(swicc_net_msg_hdr_st), 0U);
    if (recvd_bytes < 0)
    {
        logger("Call to recv() failed: %s.", strerror(errno));
        return SWICC_RET_ERROR;
    }
    do
    {
        /* Check if succeeded. */
        if (recvd_bytes == sizeof(swicc_net_msg_hdr_st))
        {
            /**
             * Check if the indicated size is too large for the static
             * message data buffer.
             */
            if (msg->hdr.size > sizeof(msg->data) ||
                msg->hdr.size < offsetof(swicc_net_msg_data_st, buf))
            {
                logger("Value of the size field in the message header is too "
                       "large. Got %u, expected %lu >= n <= %lu.",
                       msg->hdr.size, offsetof(swicc_net_msg_data_st, buf),
                       sizeof(msg->data));
                recv_failure = true;
                break;
            }
            recvd_bytes = recv(sock, &msg->data, msg->hdr.size, 0);
            /**
             * Make sure we received the whole message also the cast here is
             * safe.
             */
            if (recvd_bytes != (int64_t)msg->hdr.size)
            {
                logger("Failed to receive the whole message.");
                recv_failure = true;
                break;
            }
        }
        else
        {
            logger("Failed to receive message header: recvd_bytes=%u "
                   "header_len=%u.",
                   recvd_bytes, sizeof(swicc_net_msg_hdr_st));
            recv_failure = true;
            break;
        }
    } while (0U);
    if (recv_failure)
    {
        return SWICC_RET_ERROR;
    }
    return SWICC_RET_SUCCESS;
}

static void swicc_net_sock_close(int32_t const sock)
{
    if (sock < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.", sock);
        return;
    }

    bool success = true;
    if (sock != -1)
    {
        if (shutdown(sock, SHUT_RDWR) == -1)
        {
            logger("Call to shutdown() failed: %s.", strerror(errno));
            /* A failed shutdown is only a problem for the client. */
        }
        if (close(sock) == -1)
        {
            logger("Call to close() failed: %s.", strerror(errno));
            success = success && false;
        }
    }
    if (success)
    {
        return;
    }
    logger("Failed to close socket.");
}

void swicc_net_logger_register(swicc_net_logger_ft *const logger_func)
{
    logger = logger_func;
}

swicc_ret_et swicc_net_client_sig_register(void (*const sigh_exit)(int))
{
    struct sigaction action_new, action_old;
    action_new.sa_handler = sigh_exit;
    if (sigemptyset(&action_new.sa_mask) != 0)
    {
        logger("Call to sigemptyset() failed: %s.", strerror(errno));
        return SWICC_RET_ERROR;
    }
    action_new.sa_flags = 0;

    if (sigaction(SIGINT, NULL, &action_old) == 0)
    {
        if (action_old.sa_handler != SIG_IGN)
        {
            if (sigaction(SIGINT, &action_new, NULL) == 0)
            {
                if (sigaction(SIGHUP, NULL, &action_old) == 0)
                {
                    if (action_old.sa_handler != SIG_IGN)
                    {
                        if (sigaction(SIGHUP, &action_new, NULL) == 0)
                        {
                            if (sigaction(SIGTERM, NULL, &action_old) == 0)
                            {
                                if (action_old.sa_handler != SIG_IGN)
                                {
                                    if (sigaction(SIGTERM, &action_new, NULL) ==
                                        0)
                                    {
                                        return SWICC_RET_SUCCESS;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    swicc_net_client_sig_default();
    return SWICC_RET_ERROR;
}

void swicc_net_client_sig_default()
{
    /**
     * These calls reset the signal handler and should not fail but are asserted
     * just to be sure.
     */
    assert(signal(SIGINT, SIG_DFL) != SIG_ERR);
    assert(signal(SIGHUP, SIG_DFL) != SIG_ERR);
    assert(signal(SIGTERM, SIG_DFL) != SIG_ERR);
}

swicc_ret_et swicc_net_server_create(swicc_net_server_st *const server_ctx,
                                     char const *const port_str)
{
    uint16_t const port = (uint16_t)strtol(port_str, NULL, 10U);
    if (port == 0U)
    {
        logger("Bad port was given.");
        return SWICC_RET_PARAM_BAD;
    }

    int32_t const sock = socket(AF_INET, SOCK_STREAM, 0U);
    if (sock != -1)
    {
        struct sockaddr_in const sock_addr = {
            .sin_zero = {0U},
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htobe16(port),
        };
        if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != -1)
        {
            if (listen(sock, SWICC_NET_CLIENT_COUNT_MAX) != -1)
            {
                if (fcntl(sock, F_SETFL, O_NONBLOCK) == 0U)
                {
                    logger("Listening on port %s.", port);
                    server_ctx->sock_server = sock;
                    return SWICC_RET_SUCCESS;
                }
                else
                {
                    logger(
                        "Failed to set listening socket to non-blocking: %s.",
                        strerror(errno));
                }
            }
            else
            {
                logger("Call to listen() failed: %s.", strerror(errno));
            }
        }
        else
        {
            logger("Call to bind() failed: %s.", strerror(errno));
        }
        if (close(sock) == -1)
        {
            logger("Call to close() failed: %s.", strerror(errno));
        }
    }
    else
    {
        logger("Call to socket() failed: %s.", strerror(errno));
    }
    logger("Failed to create a server socket.");
    return SWICC_RET_ERROR;
}

void swicc_net_server_destroy(swicc_net_server_st *const server_ctx)
{
    swicc_net_sock_close(server_ctx->sock_server);
    for (uint32_t client_idx = 0U;
         client_idx <
         sizeof(server_ctx->sock_client) / sizeof(server_ctx->sock_client[0U]);
         ++client_idx)
    {
        if (server_ctx->sock_client[client_idx] >= 0)
        {
            swicc_net_sock_close(server_ctx->sock_client[client_idx]);
        }
        server_ctx->sock_client[client_idx] = -1;
    }
    server_ctx->sock_server = -1;
}

swicc_ret_et swicc_net_client_create(swicc_net_client_st *const client_ctx,
                                     char const *const hostname_str,
                                     char const *const port_str)
{
    struct in_addr hostname;
    if (inet_aton(hostname_str, &hostname) == 0)
    {
        logger("Call to inet_aton() failed: invalid hostname.");
        return SWICC_RET_PARAM_BAD;
    }

    uint16_t const port = (uint16_t)strtol(port_str, NULL, 10U);
    if (port == 0U)
    {
        logger("Call to strtol() failed: %s.", strerror(errno));
        return SWICC_RET_PARAM_BAD;
    }

    struct sockaddr_in const server_addr = {
        .sin_zero = {0U},
        .sin_family = AF_INET,
        .sin_addr = hostname,
        .sin_port = htobe16(port),
    };

    int32_t const sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock != -1)
    {
        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == 0)
        {
            client_ctx->sock_client = sock;
            return SWICC_RET_SUCCESS;
        }
        else
        {
            logger("Call to connect() failed: %s.", strerror(errno));
        }
    }
    else
    {
        logger("Call to socket() failed: %s.", strerror(errno));
    }
    return SWICC_RET_ERROR;
}

void swicc_net_client_destroy(swicc_net_client_st *const client_ctx)
{
    if (client_ctx->sock_client >= 0)
    {
        swicc_net_sock_close(client_ctx->sock_client);
    }
    client_ctx->sock_client = -1;
}

swicc_ret_et swicc_net_recv(int32_t const sock, swicc_net_msg_st *const msg)
{
    if (sock < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.", sock);
        return SWICC_RET_PARAM_BAD;
    }

    if (msg_recv(sock, msg) == SWICC_RET_SUCCESS)
    {
        return SWICC_RET_SUCCESS;
    }

    logger("Failed to receive message.");
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_net_send(int32_t const sock,
                            swicc_net_msg_st const *const msg)
{
    if (sock < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.", sock);
        return SWICC_RET_PARAM_BAD;
    }

    if (msg_send(sock, msg) == SWICC_RET_SUCCESS)
    {
        return SWICC_RET_SUCCESS;
    }

    logger("Failed to send message.");
    return SWICC_RET_ERROR;
}

swicc_ret_et swicc_net_server_client_connect(
    swicc_net_server_st *const server_ctx, uint16_t const slot)
{
    if (slot > SWICC_NET_CLIENT_COUNT_MAX)
    {
        logger("Requested slot is not present.");
        return SWICC_RET_PARAM_BAD;
    }
    if (server_ctx->sock_client[slot] != -1)
    {
        logger("Value of socket must be -1 before accepting.");
        return SWICC_RET_PARAM_BAD;
    }

    int32_t const sock = accept(server_ctx->sock_server, NULL, NULL);
    if (sock >= 0)
    {
        if (SWICC_NET_SERVER_CLIENT_KEEPALIVE == 1)
        {
            /**
             * Enable keep-alive to detect when the ICC is ejected as soon as
             * possible.
             */
            int32_t const tcp_keepalive_yes = 1,
                          tcp_keepalive_idle = 1 /* seconds */,
                          tcp_keepalive_intvl = 1 /* seconds */,
                          tcp_keepalive_pcktmax = 2 /* packets */;
            if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &tcp_keepalive_yes,
                           sizeof(tcp_keepalive_yes)) != 0 ||
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &tcp_keepalive_idle,
                           sizeof(tcp_keepalive_idle)) != 0 ||
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
                           &tcp_keepalive_intvl,
                           sizeof(tcp_keepalive_intvl)) != 0 ||
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
                           &tcp_keepalive_pcktmax,
                           sizeof(tcp_keepalive_pcktmax)) != 0)
            {
                logger("Failed to enable keep-alive for client socket.");
                swicc_net_sock_close(sock);
                return SWICC_RET_ERROR;
            }
        }
        logger("Client connected.");
        server_ctx->sock_client[slot] = sock;
        return SWICC_RET_SUCCESS;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        logger("Tried accepting connections but no client was queued.");
        return SWICC_RET_NET_CONN_QUEUE_EMPTY;
    }
    else if (errno == ECONNABORTED || errno == EPERM || errno == EPROTO)
    {
        logger("Failed to accept a client connection because of client-side "
               "problems, retrying...");
        return SWICC_RET_ERROR;
    }
    else
    {
        logger(
            "Failed to accept a client connection because accept() failed: %s.",
            strerror(errno));
        return SWICC_RET_ERROR;
    }
}

void swicc_net_server_client_disconnect(swicc_net_server_st *const server_ctx,
                                        uint16_t const slot)
{
    if (slot > SWICC_NET_CLIENT_COUNT_MAX)
    {
        logger("Requested slot is not present.");
        return;
    }
    if (server_ctx->sock_client[slot] < 0)
    {
        logger("Invalid socket FD, expected >=0 got %i.",
               server_ctx->sock_client[slot]);
        return;
    }

    swicc_net_sock_close(server_ctx->sock_client[slot]);
    /**
     * On failure to close, socket is still set to -1 so it is lost forever
     * because there is no way to recover here.
     */
    server_ctx->sock_client[slot] = -1;

    return;
}

swicc_ret_et swicc_net_client(swicc_st *const swicc_state,
                              swicc_net_client_st *const client_ctx)
{
    /* For debugging. */
    static char dbg_buf[2048U];
    uint16_t dbg_buf_len;

    swicc_ret_et ret = SWICC_RET_ERROR;
    swicc_net_msg_st msg_rx;
    swicc_net_msg_st msg_tx;

    swicc_state->buf_rx = msg_rx.data.buf;
    swicc_state->buf_rx_len = 0U;
    swicc_state->buf_tx = msg_tx.data.buf;
    swicc_state->buf_tx_len = sizeof(msg_tx.data.buf);

    while (swicc_net_recv(client_ctx->sock_client, &msg_rx) ==
           SWICC_RET_SUCCESS)
    {
        if (SWICC_NET_CLIENT_LOG_KEEPALIVE ||
            msg_rx.data.ctrl != SWICC_NET_MSG_CTRL_KEEPALIVE)
        {
            dbg_buf_len = sizeof(dbg_buf);
            if (swicc_dbg_net_msg_str(dbg_buf, &dbg_buf_len, "RX:\n",
                                      &msg_rx) == SWICC_RET_SUCCESS)
            {
                logger("%.*s", dbg_buf_len, dbg_buf);
            }
        }

        static_assert(offsetof(swicc_net_msg_data_st, buf) < UINT8_MAX,
                      "Data buffer is offset further than 255 bytes into "
                      "message data which leads to an unsafe cast.");
        /**
         * Safe cast since buf is not offset further than 255 bytes (as
         * asserted).
         */
        uint32_t const buf_rx_len =
            msg_rx.hdr.size - (uint8_t)offsetof(swicc_net_msg_data_st, buf);

        if (buf_rx_len <= UINT16_MAX)
        {
            /* Perform control operations first. */
            if (msg_rx.data.ctrl != 0U)
            {
                /**
                 * Control operations may send back data and have to indicate
                 * success or failure hence these values are set to defaults
                 * before performing the requested operation.
                 */
                msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_FAILURE;
                msg_tx.hdr.size = offsetof(swicc_net_msg_data_st, buf);

                switch (msg_rx.data.ctrl)
                {
                case SWICC_NET_MSG_CTRL_KEEPALIVE:
                    msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_SUCCESS;
                    break;
                case SWICC_NET_MSG_CTRL_MOCK_RESET_WARM_PPS_Y:
                case SWICC_NET_MSG_CTRL_MOCK_RESET_WARM_PPS_N:
                    /**
                     * A warm reset is not a cold reset but functionally they
                     * are the same.
                     */
                case SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_Y:
                case SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_N:
                    if (swicc_mock_reset_cold(
                            swicc_state,
                            msg_rx.data.ctrl ==
                                    SWICC_NET_MSG_CTRL_MOCK_RESET_WARM_PPS_Y ||
                                msg_rx.data.ctrl ==
                                    SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_Y) ==
                        SWICC_RET_SUCCESS)
                    {
                        static_assert(sizeof(swicc_atr) <=
                                          sizeof(msg_tx.data.buf),
                                      "Card ATR does not fit in the message "
                                      "data buffer.");
                        memcpy(msg_tx.data.buf, swicc_atr, sizeof(swicc_atr));
                        msg_tx.hdr.size = offsetof(swicc_net_msg_data_st, buf) +
                                          sizeof(swicc_atr);
                        msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_SUCCESS;
                    }
                    break;
                }

                /**
                 * These shall not be modified by the control operations because
                 * they represent the state of the ICC after the request.
                 */
                msg_tx.data.cont_state = swicc_state->cont_state_tx;
                msg_tx.data.buf_len_exp = swicc_state->buf_rx_len;

                if (swicc_net_send(client_ctx->sock_client, &msg_tx) !=
                    SWICC_RET_SUCCESS)
                {
                    ret = SWICC_RET_ERROR;
                    break;
                }
            }
            else
            {
                /* Handle data. */
                swicc_state->buf_rx = msg_rx.data.buf;
                swicc_state->buf_rx_len =
                    (uint16_t)buf_rx_len; /* Safe cast due to bound check. */
                swicc_state->buf_tx = msg_tx.data.buf;
                swicc_state->buf_tx_len = sizeof(msg_tx.data.buf);
                swicc_io(swicc_state);

                /* Prepare response. */
                if (sizeof(msg_tx.data.cont_state) + swicc_state->buf_tx_len >
                    UINT32_MAX)
                {
                    break;
                }
                /* Safe cast because it was checked. */
                msg_tx.hdr.size =
                    (uint32_t)(offsetof(swicc_net_msg_data_st, buf) +
                               swicc_state->buf_tx_len);
                msg_tx.data.cont_state = swicc_state->cont_state_tx;
                msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_SUCCESS;
                msg_tx.data.buf_len_exp = swicc_state->buf_rx_len;
                memcpy(msg_tx.data.buf, swicc_state->buf_tx,
                       swicc_state->buf_tx_len);
                if (swicc_net_send(client_ctx->sock_client, &msg_tx) !=
                    SWICC_RET_SUCCESS)
                {
                    ret = SWICC_RET_ERROR;
                    break;
                }
            }

            if (SWICC_NET_CLIENT_LOG_KEEPALIVE ||
                msg_rx.data.ctrl != SWICC_NET_MSG_CTRL_KEEPALIVE)
            {
                dbg_buf_len = sizeof(dbg_buf);
                if (swicc_dbg_net_msg_str(dbg_buf, &dbg_buf_len, "TX:\n",
                                          &msg_tx) == SWICC_RET_SUCCESS)
                {
                    logger("%.*s", dbg_buf_len, dbg_buf);
                }
            }
        }
    }
    return ret;
}
