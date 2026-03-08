#include "packet.h"

#include <easy_pc/easy_pc.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * This callback is called from the background consumer thread!
 * We must be careful about thread safety. Here we just write a byte to a pipe
 * to wake up the main loop's poll() call.
 */
static void
on_parse_complete(void * user_data)
{
    int * completion_pipe = (int *)user_data;
    char const cmd = 'C'; // 'C' for Complete
    if (write(completion_pipe[1], &cmd, 1) != 1)
    {
        perror("on_parse_complete: write failed");
    }
}

int
main(int argc, char ** argv)
{
    /*
     * In a real application, you might use an eventfd or a pipe to signal the main loop.
     * For this example, we'll use a simple pipe.
     */
    int completion_pipe[2];

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input_file_or_pipe>\n", argv[0]);
        fprintf(stderr, "Or try: \"stty cbreak; %s -; stty sane;\"\n", argv[0]);
        fprintf(stderr, " to enter character-at-a-time mode.\n");
        return EXIT_FAILURE;
    }

    int input_fd;
    if (strcmp(argv[1], "-") == 0)
    {
        input_fd = STDIN_FILENO;
    }
    else
    {
        input_fd = open(argv[1], O_RDONLY);
        if (input_fd < 0)
        {
            perror("Failed to open input file");
            return EXIT_FAILURE;
        }
    }

    /* Initialize signal pipe */
    if (pipe(completion_pipe) != 0)
    {
        perror("Failed to create signal pipe");
        return EXIT_FAILURE;
    }

    epc_parser_list * list = epc_parser_list_create();
    epc_parser_t * packet_parser = create_packet_parser(list);

    printf("Starting reactive streaming session...\n");

    /*
     * epc_parse_fd_reactive returns immediately.
     * It automatically sets input_fd to O_NONBLOCK.
     */
    epc_parse_session_t session;
    session = epc_parse_fd_reactive(packet_parser, input_fd, on_parse_complete, completion_pipe, NULL);

    struct pollfd fds[2];
    fds[0].fd = input_fd;
    fds[0].events = POLLIN;
    fds[1].fd = completion_pipe[0];
    fds[1].events = POLLIN;

    bool running = true;
    bool eof_reached = false;
    int packet_count = 0;

    while (running)
    {
        int ret = poll(fds, 2, -1); // Block until something happens
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll failed");
            break;
        }

        /* 1. Check for new data on input FD */
        if (fds[0].fd != -1 && (fds[0].revents & POLLIN))
        {
            epc_streaming_notify_readable(&session);
        }
        if (fds[0].fd != -1 && (fds[0].revents & (POLLHUP | POLLERR)))
        {
            epc_streaming_notify_eof(&session);
            fds[0].fd = -1; // Stop polling this FD
            eof_reached = true;
        }

        /* 2. Check for completion signal from callback */
        if (fds[1].revents & POLLIN)
        {
            char cmd;
            if (read(completion_pipe[0], &cmd, 1) == 1 && cmd == 'C')
            {
                /*
                 * The callback only told us 'something finished'.
                 * We MUST call sync_result to move the result from internal storage to session->result.
                 */
                epc_parse_session_sync_result(&session);

                if (session.result.is_error)
                {
                    /*
                     * If we reached EOF and the last attempt was an error,
                     * it might just be the EOI check failing because no more packets exist.
                     */
                    if (!eof_reached)
                    {
                        fprintf(stderr, "Parse Error: %s\n", session.result.data.error->message);
                    }
                    running = false;
                }
                else
                {
                    packet_count++;
                    printf("\n--- Packet #%d Parsed ---\n", packet_count);
                    char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
                    if (cpt_str)
                    {
                        printf("%s\n", cpt_str);
                        free(cpt_str);
                    }
                    printf("Advancing to next packet...\n");
                    epc_parse_session_advance(&session, packet_parser);
                }
            }
        }
    }

    printf("\nStream finished. Total packets parsed: %d\n", packet_count);

    epc_parse_session_destroy(&session);
    epc_parser_list_free(list);
    close(completion_pipe[0]);
    close(completion_pipe[1]);
    if (input_fd != STDIN_FILENO)
        close(input_fd);

    return EXIT_SUCCESS;
}
