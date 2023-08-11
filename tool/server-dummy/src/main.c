#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_CLR
#include <swicc/swicc.h>

swicc_net_server_st server_ctx;

static void sig_exit_handler(__attribute__((unused)) int signum)
{
    fprintf(stderr, "Shutting down...\n");
    swicc_net_server_destroy(&server_ctx);
    exit(0);
}

static void print_usage(char const *const arg0)
{
    // clang-format off
    fprintf(stderr, "Usage: %s <"CLR_VAL("port")"> <"CLR_VAL("/path/to/dummy-data")">"
        "\n"
        "\nThis expects to get a text file with dummy data where each item in"
        "\nthe list is delimited by a newline character (\\n). The data will "
        "\nbe sent one by one to any client which connects. After all data is"
        "\nsent, the client gets disconnected and the server waits for a new "
        "\nclient to connect and it will again send data and disconnect it."
        "\n",
        arg0);
    // clang-format on
}

static uint32_t parse_data(char const *const data_in, uint8_t *const data_out,
                           uint32_t const data_in_len,
                           uint32_t const data_out_len_max)
{
    uint32_t data_out_len = 0U;
    uint8_t nibble_count = 0U;
    uint32_t nibbles[2U] = {0U};
    for (uint32_t data_idx = 0U; data_idx < data_in_len; ++data_idx)
    {
        if (data_in[data_idx] == ' ')
        {
            continue;
        }
        char const nibble_char = data_in[data_idx];
        char base;
        uint8_t base_val;
        if (nibble_char >= '0' && nibble_char <= '9')
        {
            base = '0';
            base_val = 0;
        }
        else if (nibble_char >= 'A' && nibble_char <= 'F')
        {
            base = 'A';
            base_val = 0x0A;
        }
        else if (nibble_char >= 'a' && nibble_char <= 'f')
        {
            base = 'a';
            base_val = 0x0A;
        }
        else
        {
            break;
        }
        nibbles[nibble_count++] =
            (uint8_t)(base_val + (data_in[data_idx] - base));
        if (nibble_count == 2U)
        {
            if (data_out_len + 1U > data_out_len_max)
            {
                break;
            }
            data_out[data_out_len++] =
                (uint8_t)((nibbles[0U] << 4U) | nibbles[1U]);
            nibble_count = 0U;
        }
    }
    return data_out_len;
}

int32_t main(int32_t const argc, char const *const argv[argc])
{
    if (argc != 3U)
    {
        switch (argc)
        {
        case 2U:
            fprintf(
                stderr,
                CLR_TXT(
                    CLR_RED,
                    "Missing 2nd argument, it should be the path to dummy data.\n"));
            break;
        case 1U:
            fprintf(
                stderr,
                CLR_TXT(
                    CLR_RED,
                    "Missing 1st argument, it should be the port on which to host the server, usually this is 37324.\n"));
            break;
        case 0U:
            __builtin_unreachable();
        }
        print_usage(argv[0U]);
        return -1;
    }

    /* Prepare the server context. */
    server_ctx.sock_server = -1;
    for (uint16_t client_idx = 0U;
         client_idx <
         sizeof(server_ctx.sock_client) / sizeof(server_ctx.sock_client[0U]);
         ++client_idx)
    {
        server_ctx.sock_client[client_idx] = -1;
    }

    char const *const str_port = argv[1U];
    char const *const str_data_path = argv[2U];

    fprintf(stderr, "Starting server on port %s with data at '%s'...\n",
            str_port, str_data_path);

    if (swicc_net_client_sig_register(sig_exit_handler) != SWICC_RET_SUCCESS)
    {
        fprintf(stderr, "Failed to register signal handler.\n");
        return -1;
    }

    if (swicc_net_server_create(&server_ctx, str_port) != SWICC_RET_SUCCESS)
    {
        swicc_net_server_destroy(&server_ctx);
        fprintf(stderr, "Failed to create server context.\n");
        return -1;
    }

    for (;;)
    {
        swicc_ret_et const ret_accept =
            swicc_net_server_client_connect(&server_ctx, 0U);
        if (ret_accept == SWICC_RET_SUCCESS)
        {
            fprintf(stderr, "Client connected.\n");

            FILE *file_data = fopen(str_data_path, "r");
            if (file_data != NULL)
            {
                uint8_t file_done = 0U;
                while (!file_done)
                {
                    char file_line[1024U] = {0U};
                    if (fgets(file_line, sizeof(file_line) - 1U, file_data) ==
                        NULL)
                    {
                        fprintf(stderr, "Failed to read line from file.\n");
                        break;
                    }
                    /* Safe cast due to static assert. */
                    static_assert(
                        UINT32_MAX >= sizeof(file_line),
                        "A 32 bit uint may not be able to hold length of array.");
                    uint32_t const file_line_len_raw =
                        (uint32_t)strnlen(file_line, sizeof(file_line));
                    uint32_t const file_line_len =
                        file_line[file_line_len_raw - 1U] == '\n'
                            ? file_line_len_raw - 1U
                            : file_line_len_raw;
                    if (file_line_len > 0U)
                    {
                        swicc_net_msg_st msg = {0U};
                        uint32_t const msg_data_len =
                            parse_data(file_line, msg.data.buf, file_line_len,
                                       sizeof(msg.data.buf));
                        fprintf(stderr, "File line: '%.*s' Data length: %u\n",
                                file_line_len, file_line, msg_data_len);

                        static_assert(offsetof(swicc_net_msg_data_st, buf) <
                                          UINT32_MAX,
                                      "Offset too large to fit in a uint32.");
                        msg.hdr.size =
                            (uint32_t)(offsetof(swicc_net_msg_data_st, buf) +
                                       msg_data_len);
                        msg.data.ctrl = SWICC_NET_MSG_CTRL_NONE;
                        msg.data.buf_len_exp = 0U;
                        msg.data.cont_state = 0U;

                        if (swicc_net_send(server_ctx.sock_client[0U], &msg) !=
                            SWICC_RET_SUCCESS)
                        {
                            fprintf(stderr,
                                    "Failed to send message to client.\n");
                            break;
                        }
                        if (swicc_net_recv(server_ctx.sock_client[0U], &msg) !=
                            SWICC_RET_SUCCESS)
                        {
                            fprintf(stderr,
                                    "Failed to receive message from client.\n");
                            break;
                        }
                    }

                    if (file_line_len == 0U || feof(file_data) ||
                        ferror(file_data))
                    {
                        file_done = 1U;
                    }
                }
            }
            fclose(file_data);

            /* Accepted client connection, so send data and disconnect it. */
            swicc_net_server_client_disconnect(&server_ctx, 0U);
            fprintf(stderr, "Client disconnected.\n");
        }
        else if (ret_accept == SWICC_RET_NET_CONN_QUEUE_EMPTY)
        {
            /* Queue was empty so try again. */
        }
        else if (ret_accept == SWICC_RET_ERROR)
        {
            swicc_net_server_destroy(&server_ctx);
            fprintf(stderr, "Error while accepting client connection.\n");
            return -1;
        }
    }

    return 0;
}
