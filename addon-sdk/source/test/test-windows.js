



"use strict";

const app = require("sdk/system/xul-app");

if (app.is("Firefox")) {
  module.exports = require("./windows/test-firefox-windows");
}
else if (app.is("Fennec")) {
  module.exports = require("./windows/test-fennec-windows");
}
else {
  require("test").run({
    "test Unsupported Application": function Unsupported (assert) {
      assert.pass(
        "The windows module currently supports only Firefox and Fennec." +
        "In the future we would like it to support other applications, however."
      );
    }
  });
}
