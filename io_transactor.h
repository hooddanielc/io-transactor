// g++ -std=c++11 -g -odj json.cc io_transactor.cc dj.cc -Wl,--no-as-needed -lpthread

#pragma once

#include <condition_variable>
#include <exception>
#include <thread>

#include <poll.h>
#include <unistd.h>

#include "json.h"

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

    /* TODO */
    virtual bool on_msg(json_t &msg, json_t &reply) = 0;

    /* Start the background thread.  Call this in your final constructor. */
    void start();

    /* Stop the background thread.  Call this in your final destructor. */
    void stop();

    private:

    /* Transacts JSON messages on standard in/out. */
    void bg_main();

    /* Used by stop() to tell the background that the foreground wishes to
       stop. */
    int fds[2];

    /* The thread which enters at bg_main(). */
    std::thread bg_thread;

    /* Covers ex_ptr and exited. */
    std::mutex mutex;

    /* The exception thrown by the background, if any. */
    std::exception_ptr ex_ptr;

    /* True after the background exits. */
    bool exited;

    /* Notifies the foreground when the background exits. */
    std::condition_variable exited_set;

  };  // io_transactor_t

}  // dj
