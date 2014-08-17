#include "io_transactor.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace dj;

io_transactor_t::~io_transactor_t() {
  assert(this);
  assert(!bg_thread.joinable());
  close(fds[0]);
  close(fds[1]);
}

bool io_transactor_t::has_exited() {
  assert(this);
  assert(bg_thread.joinable());
  unique_lock<std::mutex> lock(mutex);
  if (ex_ptr) {
    rethrow_exception(ex_ptr);
  }
  return exited;
}

void io_transactor_t::wait_until_exited() {
  assert(this);
  assert(bg_thread.joinable());
  unique_lock<std::mutex> lock(mutex);
  while (!exited) {
    exited_set.wait(lock);
  }
  if (ex_ptr) {
    rethrow_exception(ex_ptr);
  }
}

io_transactor_t::io_transactor_t() {
  if (pipe(fds) < 0) {
    throw runtime_error("pipe");
  }
}

void io_transactor_t::start() {
  assert(this);
  stop();
  exited = false;
  ex_ptr = exception_ptr();
  bg_thread = thread(&io_transactor_t::bg_main, this);
}

void io_transactor_t::stop() {
  assert(this);
  if (bg_thread.joinable()) {
    write(fds[1], "x", 1);
    bg_thread.join();
  }
}

void io_transactor_t::bg_main() {
  assert(this);
  try {
    pollfd polls[2];
    polls[0].fd = fds[0];
    polls[0].events = POLLIN;
    polls[1].fd = 0;
    polls[1].events = POLLIN;
    for (;;) {
      if (poll(polls, 2, -1) < 0) {
        throw runtime_error("poll");
      }
      if (polls[0].revents & POLLIN) {
        break;
      }
      if (polls[1].revents & POLLIN) {
        json_t msg, reply;
        cin >> msg;
        if (!on_msg(msg, reply)) {
          break;
        }
        cout << reply;
      }
    }
    unique_lock<std::mutex> lock(mutex);
    exited = true;
    exited_set.notify_one();
  } catch (...) {
    unique_lock<std::mutex> lock(mutex);
    ex_ptr = current_exception();
    exited = true;
    exited_set.notify_one();
  }
}
