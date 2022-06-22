#pragma once
/**
 * This is for receiving and sending swICC I/O messages over the network.
 */

#include "swicc/common.h"
#include <stdbool.h>
#include <stdint.h>

#define SWICC_NET_CLIENT_COUNT_MAX 8U
#define SWICC_NET_SERVER_CLIENT_KEEPALIVE 0U
#define SWICC_NET_CLIENT_LOG_KEEPALIVE false

typedef enum swicc_net_msg_ctrl_e
{
    /* Control values for requests (server -> client). */
    SWICC_NET_MSG_CTRL_NONE = 0,
    SWICC_NET_MSG_CTRL_KEEPALIVE,
    SWICC_NET_MSG_CTRL_MOCK_RESET_COLD,
    SWICC_NET_MSG_CTRL_MOCK_RESET_WARM,

    /* Control values for responses (client -> server). */
    SWICC_NET_MSG_CTRL_SUCCESS = 0xF0,
    SWICC_NET_MSG_CTRL_FAILURE = 0x0F,
} swicc_net_msg_ctrl_et;

typedef struct swicc_net_msg_hdr_s
{
    uint32_t size;
} __attribute__((packed)) swicc_net_msg_hdr_st;

typedef struct swicc_net_msg_data_s
{
    /* Holds both control requests and success/failure of the request. */
    uint8_t ctrl;

    /* Holding contact state. */
    uint32_t cont_state;

    /**
     * When sent to swICC, this is unused, when being received by a client, this
     * indicates how many bytes to read from the interface (i.e. the expected
     * buffer length).
     */
    uint32_t buf_len_exp;

    uint8_t buf[SWICC_DATA_MAX];
} __attribute__((packed)) swicc_net_msg_data_st;

/**
 * Expected message format.
 * @note Might want to create a raw and internal representation of the message
 * if using sizeof(msg.data) feels unsafe.
 */
typedef struct swicc_net_msg_s
{
    swicc_net_msg_hdr_st hdr;
    swicc_net_msg_data_st data;
} __attribute__((packed)) swicc_net_msg_st;

typedef struct swicc_net_server_s
{
    int32_t sock_server;
    int32_t sock_client[SWICC_NET_CLIENT_COUNT_MAX];
} swicc_net_server_st;

typedef struct swicc_net_client_s
{
    int32_t sock_client;
} swicc_net_client_st;

typedef void swicc_net_logger_ft(char const *const fmt, ...);
void swicc_net_logger_register(swicc_net_logger_ft *const logger_func);

swicc_ret_et swicc_net_client_sig_register(void (*const sigh_exit)(int));
void swicc_net_client_sig_default();

swicc_ret_et swicc_net_server_create(swicc_net_server_st *const server_ctx,
                                     char const *const port_str);
void swicc_net_server_destroy(swicc_net_server_st *const server_ctx);

swicc_ret_et swicc_net_client_create(swicc_net_client_st *const client_ctx,
                                     char const *const hostname_str,
                                     char const *const port_str);
void swicc_net_client_destroy(swicc_net_client_st *const client_ctx);

swicc_ret_et swicc_net_recv(int32_t const sock, swicc_net_msg_st *const msg);
swicc_ret_et swicc_net_send(int32_t const sock,
                            swicc_net_msg_st const *const msg);

swicc_ret_et swicc_net_server_client_connect(
    swicc_net_server_st *const server_ctx, uint16_t const slot);
void swicc_net_server_client_disconnect(swicc_net_server_st *const server_ctx,
                                        uint16_t const slot);

swicc_ret_et swicc_net_client(swicc_st *const swicc_state,
                              swicc_net_client_st *const client_ctx);
