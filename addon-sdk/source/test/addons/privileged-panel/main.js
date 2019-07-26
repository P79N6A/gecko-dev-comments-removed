



"use strict";

const { Panel } = require("sdk/panel")
const { data } = require("sdk/self")

exports["test addon global"] = function(assert, done) {
  let panel = Panel({
    contentURL: 
                data.url("./index.html"),
    onMessage: function(message) {
      assert.pass("got message from panel script");
      panel.destroy();
      done();
    },
    onError: function(error) {
      asser.fail(Error("failed to recieve message"));
      done();
    }
  });
};

require("sdk/test/runner").runTestsFromModule(module);
