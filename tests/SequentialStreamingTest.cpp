#include "cpt_node.h"
#include "easy_pc_private.h"

#include <CppUTest/TestHarness.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

#define MAX_FRAGMENTS 10

struct ProducerArgs
{
    int fd;
    char const * fragments[MAX_FRAGMENTS];
    int delays_ms[MAX_FRAGMENTS];
    int fragment_count;
};

static void *
producer_thread(void * arg)
{
    ProducerArgs * args = (ProducerArgs *)arg;

    for (int i = 0; i < args->fragment_count; ++i)
    {
        if (args->delays_ms[i] > 0)
        {
            usleep((useconds_t)args->delays_ms[i] * 1000);
        }
        char const * data = args->fragments[i];
        size_t len = strlen(data);
        if (write(args->fd, data, len) != (ssize_t)len)
        {
            break;
        }
    }

    /* Keep FD open until the end of the test to allow multiple objects */
    return NULL;
}

static bool g_complete_fired = false;
static void
on_complete_cb(void * user_data)
{
    (void)user_data;
    g_complete_fired = true;
}

TEST_GROUP(SequentialStreamingTest)
{
    pthread_t producer;
    ProducerArgs args;
    int pipe_fds[2];
    bool thread_started;
    epc_parser_list * list;
    epc_parse_session_t session;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
        thread_started = false;
        g_complete_fired = false;
        pipe_fds[0] = -1;
        pipe_fds[1] = -1;
        if (pipe(pipe_fds) != 0)
        {
            FAIL("Failed to create pipe");
        }
    }

    void teardown() override
    {
        if (thread_started)
        {
            pthread_join(producer, NULL);
        }
        if (pipe_fds[0] != -1)
        {
            close(pipe_fds[0]);
        }
        if (pipe_fds[1] != -1)
        {
            close(pipe_fds[1]);
        }
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }

    void start_producer(char const * data, int delay_ms = 0)
    {
        args.fd = pipe_fds[1];
        args.fragments[0] = data;
        args.delays_ms[0] = delay_ms;
        args.fragment_count = 1;
        thread_started = true;
        pthread_create(&producer, NULL, producer_thread, &args);
    }

    void wait_for_completion(int timeout_ms = 1000)
    {
        int total = 0;
        while (!g_complete_fired && total < timeout_ms)
        {
            usleep(10000);
            total += 10;
        }
        if (g_complete_fired)
        {
            epc_parse_session_sync_result(&session);
        }
    }
};

TEST(SequentialStreamingTest, ParseTwoStringsSequentially)
{
    epc_parser_t * p = epc_string(list, NULL, "hello");

    // 1. Start reactive session
    session = epc_parse_fd_reactive(p, pipe_fds[0], on_complete_cb, NULL, NULL);

    // 2. Write first object
    if (write(pipe_fds[1], "hello", 5) != 5)
        FAIL("write failed");

    // 3. Notify readable
    epc_streaming_notify_readable(&session);

    // 4. Wait for completion
    wait_for_completion();
    CHECK_TRUE(g_complete_fired);
    CHECK_FALSE(session.result.is_error);

    // 5. Advance to next object
    g_complete_fired = false;
    CHECK_TRUE(epc_parse_session_advance(&session, p));

    // 6. Write second object
    if (write(pipe_fds[1], "hello", 5) != 5)
        FAIL("write failed");
    epc_streaming_notify_readable(&session);

    // 7. Wait for completion
    wait_for_completion();
    CHECK_TRUE(g_complete_fired);
    CHECK_FALSE(session.result.is_error);
}

TEST(SequentialStreamingTest, ParseWithLeftoverData)
{
    epc_parser_t * p = epc_string(list, NULL, "abc");

    session = epc_parse_fd_reactive(p, pipe_fds[0], on_complete_cb, NULL, NULL);

    // Write "abc" + "a" (start of next "abc")
    if (write(pipe_fds[1], "abca", 4) != 4)
        FAIL("write failed");
    epc_streaming_notify_readable(&session);

    wait_for_completion();
    CHECK_TRUE(g_complete_fired);

    // Advance. It should compact "a" to the beginning.
    g_complete_fired = false;
    CHECK_TRUE(epc_parse_session_advance(&session, p));

    // Write the rest of "abc" (which is "bc")
    if (write(pipe_fds[1], "bc", 2) != 2)
        FAIL("write failed");
    epc_streaming_notify_readable(&session);

    wait_for_completion();
    CHECK_TRUE(g_complete_fired);
    CHECK_FALSE(session.result.is_error);
}
