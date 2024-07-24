#pragma once
/**
 * This is for receiving and sending swICC I/O messages over the network.
 */

#include "swicc/common.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Maximum number of clients (cards) that can connect to one server. This is
 * arbitrary but note that the PC/SC IFD handler relies on this so number so
 * larger number means more resources will be used by the PC/SC middleware.
 */
#define SWICC_NET_CLIENT_COUNT_MAX 8U

/* If the keep-alive functionality of sockets should be used. */
#define SWICC_NET_SERVER_CLIENT_KEEPALIVE 0U

/* If the keep-alive messages should be logged with network logger method. */
#define SWICC_NET_CLIENT_LOG_KEEPALIVE false

/* Possible values of the control field of a messaage. */
typedef enum swicc_net_msg_ctrl_e
{
    /* Control values for requests (server -> client). */
    SWICC_NET_MSG_CTRL_NONE = 0,
    SWICC_NET_MSG_CTRL_KEEPALIVE,
    SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_Y,
    SWICC_NET_MSG_CTRL_MOCK_RESET_WARM_PPS_Y,
    SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_N,
    SWICC_NET_MSG_CTRL_MOCK_RESET_WARM_PPS_N,

    /* Control values for responses (client -> server). */
    SWICC_NET_MSG_CTRL_SUCCESS = 0xF0,
    SWICC_NET_MSG_CTRL_FAILURE = 0x0F,
} swicc_net_msg_ctrl_et;

typedef struct swicc_net_msg_hdr_s
{
    uint32_t size; /* The size of the data. */
} __attribute__((packed)) swicc_net_msg_hdr_st;

typedef struct swicc_net_msg_data_s
{
    /* Contact state. */
    uint32_t cont_state;

    /**
     * When sent to swICC, this is unused, when being received by a client, this
     * indicates how many bytes to read from the interface (i.e. the expected
     * buffer length).
     */
    uint32_t buf_len_exp;

    /* Holds both control requests and success/failure of the request. */
    uint8_t ctrl;

    /**
     * +2 because we can have 256 bytes of response followed by 2 status bytes.
     */
    uint8_t buf[SWICC_DATA_MAX + 2U];
} __attribute__((packed)) swicc_net_msg_data_st;

/* Expected message format. */
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

/**
 * @brief Basically a printf function.
 * @param fmt
 */
typedef void swicc_net_logger_ft(char const *const fmt, ...);

/**
 * @brief Since things can break in networking, and swICC does not log anything
 * ever (only provides debugging utilities that generate debug string), a logger
 * must be registered to get useful logging output from the network module.
 * @param[in] logger_func This function shall work like printf but output in
 * whatever way is most suited by the user.
 */
void swicc_net_logger_register(swicc_net_logger_ft *const logger_func);

/**
 * @brief Helper for setting a signal handler (SIGINT, SIGHUP, SIGTERM). This is
 * useful as the network client is an infinite loop that will not return unless
 * a network error occurs. To gracefully disconnect, best to handle the signal
 * correctly.
 * @param[in] sigh_exit Signal handler for when the client wants to exit.
 * @return Return code.
 */
swicc_ret_et swicc_net_client_sig_register(void (*const sigh_exit)(int));

/**
 * @brief Reset the signal handler for SIGINT, SIGHUP, and SIGTERM to the
 * default.
 */
void swicc_net_client_sig_default(void);

/**
 * @brief Create a network server on some port. This will initialize the server
 * context.
 * @param[out] server_ctx The server context that will be initialized.
 * @param[in] port_str Port the server will bind to and listen on.
 * @return Return code.
 * @note Server socket is non-blocking.
 */
swicc_ret_et swicc_net_server_create(swicc_net_server_st *const server_ctx,
                                     char const *const port_str);

/**
 * @brief Destroy the network server. This includes all the sockets (both server
 * and client).
 * @param[in, out] server_ctx
 */
void swicc_net_server_destroy(swicc_net_server_st *const server_ctx);

/**
 * @brief Create a network client and connect it to a host on some port.
 * @param[out] client_ctx The network client context that will be initialized.
 * @param[in] hostname_str String of the server hostname.
 * @param[in] port_str String of the server port.
 * @return Return code.
 * @note Client socket is blocking.
 */
swicc_ret_et swicc_net_client_create(swicc_net_client_st *const client_ctx,
                                     char const *const hostname_str,
                                     char const *const port_str);
/**
 * @brief Destroy the network client.
 * @param[in, out] client_ctx The client to destroy.
 */
void swicc_net_client_destroy(swicc_net_client_st *const client_ctx);

/**
 * @brief Receive a message on a given socket.
 * @param[in] sock Where to receive from.
 * @param[out] msg Where to write the received message.
 * @return Return code.
 */
swicc_ret_et swicc_net_recv(int32_t const sock, swicc_net_msg_st *const msg);

/**
 * @brief Send a message to some socket.
 * @param[in] sock Where to send message.
 * @param[in] msg The message that will be sent.
 * @return Return code.
 */
swicc_ret_et swicc_net_send(int32_t const sock,
                            swicc_net_msg_st const *const msg);

/**
 * @brief Attempt to accept a client connection. Since the server socket is
 * non-blocking, this will indicate if no clients were present in queue, if a
 * client was connected, or if this failed.
 * @param[in, out] server_ctx The server that wants to accept a connection.
 * @param[in] slot On which slot to accept the client.
 * @return Return code.
 * @note The server socket is non-blocking so depending on if a client
 * connection was accepted or not, this will return a different return value.
 */
swicc_ret_et swicc_net_server_client_connect(
    swicc_net_server_st *const server_ctx, uint16_t const slot);

/**
 * @brief Disconnect a client from a server.
 * @param[in, out] server_ctx
 * @param[in] slot Which client to disconnect.
 */
void swicc_net_server_client_disconnect(swicc_net_server_st *const server_ctx,
                                        uint16_t const slot);

/**
 * @brief An implementation of a complete network client with a receive loop
 * which gets messages, processes them using swICC functions, and sends back a
 * response.
 * @param[in, out] swicc_state An initialized swICC state.
 * @param[in, out] client_ctx An initialized network client context.
 * @return Return code.
 * @note Best to register a signal handler before calling this since this will
 * not return until a network error occurs.
 */
swicc_ret_et swicc_net_client(swicc_st *const swicc_state,
                              swicc_net_client_st *const client_ctx);
