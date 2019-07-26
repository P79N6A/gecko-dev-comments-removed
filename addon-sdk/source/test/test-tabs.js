


'use strict';

const app = require("sdk/system/xul-app");

if (app.is("Firefox")) {
  module.exports = require("./tabs/test-firefox-tabs");
}
else if (app.is("Fennec")) {
  module.exports = require("./tabs/test-fennec-tabs");
}
else {
  require("test").run({
    "test Unsupported Application": function Unsupported (assert) {
      assert.pass(
        "The tabs module currently supports only Firefox and Fennec." +
        "In the future we would like it to support other applications, however."
      );
    }
  });
}
