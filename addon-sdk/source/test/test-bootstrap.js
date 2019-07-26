


"use strict";

const { Request } = require("sdk/request");

exports.testBootstrapExists = function (assert, done) {
  Request({
    url: "resource://gre/modules/sdk/bootstrap.js",
    onComplete: function (response) {
      if (response.text)
        assert.pass("the bootstrap file was found");
      done();
    }
  }).get();
};

require("sdk/test").run(exports);
