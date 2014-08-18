#include "io_transactor.h"

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace dj;

io_transactor_t::~io_transactor_t() {
  assert(this);
  assert(!bg_thread.joinable());
  sigaction(SIGUSR1, &old_act, nullptr);
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
  struct sigaction new_act;
  memset(&new_act, 0, sizeof(new_act));
  new_act.sa_handler = [](int) {};
  if (sigaction(SIGUSR1, &new_act, &old_act) < 0) {
    throw runtime_error("sigaction");
  }
}

future_t io_transactor_t::send(json_t &&request) {
  assert(this);
  assert(&request);
  auto future = promise_keeper.make_promise();
  write_msg(op_t::request, future->get_id(), move(request));
  return future;
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
    pthread_kill(bg_thread.native_handle(), SIGUSR1);
    bg_thread.join();
  }
}

void io_transactor_t::bg_main() {
  assert(this);
  try {
    bool go = true;
    do {
      json_t msg;
      cin >> msg;
      op_t op;
      int id;
      json_t body;
      if (try_parse_msg(msg, op, id, body)) {
        switch (op) {
          case op_t::request: {
            auto reply = on_request(body);
            write_msg(op_t::response, id, on_request(body));
            break;
          }
          case op_t::response: {
            promise_keeper.keep_promise(id, move(body));
            break;
          }
          case op_t::stop: {
            go = false;
            break;
          }
        }
      } else {
        cerr << "bad msg " << msg << endl;
      }
    } while (go);
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

bool io_transactor_t::try_parse_msg(
    json_t &msg, op_t &op, int &id, json_t &body) {
  assert(&msg);
  assert(&op);
  assert(&id);
  assert(&body);
  static const map<string, op_t> ops = {
    { "request", op_t::request },
    { "response", op_t::response },
    { "stop", op_t::stop }
  };
  if (msg.get_kind() != json_t::object) {
    return false;
  }
  const auto *elem = msg.try_get_elem("op");
  if (!elem) {
    return false;
  }
  const auto *str = elem->try_get_state<json_t::string_t>();
  if (!str) {
    return false;
  }
  auto iter = ops.find(*str);
  if (iter == ops.end()) {
    return false;
  }
  op = iter->second;
  if (op == op_t::stop) {
    return true;
  }
  elem = msg.try_get_elem("id");
  if (!elem) {
    return false;
  }
  const auto *num = elem->try_get_state<json_t::number_t>();
  if (!num) {
    return false;
  }
  id = *num;
  elem = msg.try_get_elem("body");
  if (!elem) {
    return false;
  }
  body = move(*elem);
  return true;
}

void io_transactor_t::write_msg(op_t op, int id, json_t &&body) {
  assert(&body);
  static const map<op_t, string> ops = {
    { op_t::request, "request" },
    { op_t::response, "response" },
    { op_t::stop, "stop" }
  };
  cout << json_t(json_t::object_t({
    { "op", ops.find(op)->second },
    { "id", id },
    { "body", move(body) }
  })) << endl;
}