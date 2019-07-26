



"use strict";

const app = require("sdk/system/xul-app");

exports["test addon globa"] = app.is("Firefox") ? testAddonGlobal : unsupported;

function testAddonGlobal (assert, done) {
  const { Panel } = require("sdk/panel")
  const { data } = require("sdk/self")

  let panel = Panel({
    contentURL: 
                data.url("./index.html"),
    onMessage: function(message) {
      assert.pass("got message from panel script");
      panel.destroy();
      done();
    },
    onError: function(error) {
      assert.fail(Error("failed to recieve message"));
      done();
    }
  });
};

function unsupported (assert) {
  assert.pass("privileged-panel unsupported on platform");
}

require("sdk/test/runner").runTestsFromModule(module);
