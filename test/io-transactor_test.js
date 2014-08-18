var expect = require("chai").expect;
var should = require("chai").should;

describe("io-transactor", function () {

  var ioTransactor = require("./..");
  var process;

  beforeEach(function () {
    process = ioTransactor.spawn("./build/Release/io_transactor_test");
  });

  afterEach(function () {
    process.stop();
  });

  it("does exist", function () {
    expect(typeof ioTransactor).to.equal("object");
  });

  it("can spawn a process", function () {
    expect(typeof process).to.equal("object");
  });

  it("can stop the process", function (done) {
    expect(!process._process.pid).to.equal(false);

    process.on("exit", function () {
      done();
    });

    process.stop();
  });

  it("can send a request and receive a callback response", function (done) {
    expect(!process._process.pid).to.equal(false);
    var isdone = 0;

    for (var i = 0; i < 100; ++i) {
      process.request({ sup: "buddy 420" }, function (e) {
        expect(e.sup).to.equal("buddy 420");
        if (++isdone == 100) {
          done();
        }
      });
    }
  });
});