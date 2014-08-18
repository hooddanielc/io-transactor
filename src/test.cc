#include <node.h>
#include <v8.h>
#include <io-transactor.h>
#include <iostream>

using namespace v8;
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

  virtual json_t on_request(json_t &request) override {
    return request;
  }
};

void init(Handle<Object> exports) {
  foo_t foo;
  foo.wait_until_exited();
}

NODE_MODULE(io_transactor_test, init);
