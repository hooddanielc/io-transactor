"use strict";

var util = require("util");
var EventEmitter = require("events").EventEmitter;
var spawn = require("child_process").spawn;

function Worker(path) {
  var self = this;
  var _process = spawn("node", [path]);
  var _event_queue = {};
  var _num_events = 0;
  var _running = true;
  this._process = _process;

  _process.on("exit", function () {
    _running = false;
  });

  this.send = function (obj) {
    _process.stdin.write(JSON.stringify(obj));
  };

  this.stop = function () {
    if (_running) {
      this.send({
        op: "stop"
      });
    }
  };

  this.request = function (obj, fn) {
    if (++_num_events === Number.MAX_VALUE) {
      _num_events = 1;
    }

    var id = _num_events;
    _event_queue[id] = fn;

    self.send({
      op: "request",
      body: obj,
      id: id
    });
  };

  _process.stdout.on("data", function (data) {
    self.emit("stdout", data);
    var res = data.toString().split("\n");
    res.pop();


    for (var i in res) {
      try {
        var msg = JSON.parse(res[i]);

        if (msg.op === "response") {
          if (_event_queue[msg.id]) {
            _event_queue[msg.id](msg.body);
            delete _event_queue[msg.id];
          }
        } else if (msg.op === "request") {
          // TODO
        }
      } catch(e) {
        self.emit("error", "failed to parse a message");
      }
    }
  });

  _process.stderr.on("data", function (data) {
    self.emit("stderr", data);
  });

  _process.on("exit", function () {
    self.emit("exit");
  });
}

util.inherits(Worker, EventEmitter);
module.exports.Worker = Worker;

module.exports.spawn = function (path) {
  return new Worker(path);
};
