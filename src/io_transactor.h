// g++ -std=c++11 -g -odj json.cc io_transactor.cc dj.cc -Wl,--no-as-needed -lpthread

#pragma once

#include <condition_variable>
#include <exception>
#include <thread>

#include <signal.h>
#include <unistd.h>

#include "json.h"
#include "promise.h"

namespace dj {

  /* Transact JSON-based requests and replies over standard in/out. */
  class io_transactor_t {
    public:

    /* Do-little.  Make sure you call stop() in your final destructor. */
    virtual ~io_transactor_t();

    /* True iff. the background thread has exited.  If the background thread
       exited with an exception, it is re-thrown here. */
    bool has_exited();

    /* Blocks until the background thread has exited.  If the background
       thread exits with an exception, it is re-thrown here.*/
    void wait_until_exited();

    protected:

    /* Call start() in your final constructor to start the background
       thread. */
    io_transactor_t();

    /* Override to handle a request and return a response. */
    virtual json_t on_request(json_t &request) = 0;

    /* Send a request to our peer and get JSON in the future. */
    future_t send(json_t &&request);

    /* Start the background thread.  Call this in your final constructor. */
    void start();

    /* Stop the background thread.  Call this in your final destructor. */
    void stop();

    private:

    /* The operations we can find in messages. */
    enum class op_t { request, response, stop };

    /* Transacts JSON messages on standard in/out. */
    void bg_main();

    /* Try to parse a JSON message consisting of an op, id, and body.
       Return success/failure. */
    static bool try_parse_msg(
        json_t &msg, op_t &op, int &id, json_t &body);

    /* Write a JSON message which can be aprsed by try_parse_msg(). */
    static void write_msg(op_t op, int id, json_t &&body);

    /* Old signal handler. */
    struct sigaction old_act;

    /* Makes and keeps promises for us. */
    promise_t::keeper_t promise_keeper;

    /* The thread which enters at bg_main(). */
    std::thread bg_thread;

    /* Covers ex_ptr, exited, next_id. */
    std::mutex mutex;

    /* The exception thrown by the background, if any. */
    std::exception_ptr ex_ptr;

    /* True after the background exits. */
    bool exited;

    /* Notifies the foreground when the background exits. */
    std::condition_variable exited_set;

  };  // io_transactor_t

}  // dj