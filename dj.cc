#include <iostream>

#include "io_transactor.h"

using namespace std;
using namespace dj;

class foo_t final
    : public io_transactor_t {
  public:

  foo_t() {
    start();
  }

  virtual ~foo_t() {
    stop();
  }

  private:

  virtual bool on_msg(json_t &msg, json_t &reply) override {
    if (msg["cmd"] == "exit") {
      return false;
    }
    reply = msg;
    return true;
  }

};

int main(int, char *[]) {
  foo_t foo;
  for (;;) {
    if (foo.has_exited()) {
      break;
    }
    handle_ui();
  }
}
