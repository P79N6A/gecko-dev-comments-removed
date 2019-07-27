


"use strict";

var Promise = require("promise");
var Mocha = require("mocha");
var mocha = new Mocha({
  ui: "bdd",
  reporter: "spec",
  timeout: 900000
});

exports.run = function(type) {
  return new Promise(function(resolve) {
    type = type || "";
    [
      (/^(modules)?$/.test(type)) && require.resolve("../bin/node-scripts/test.modules"),
      (/^(addons)?$/.test(type)) && require.resolve("../bin/node-scripts/test.addons"),
      (/^(examples)?$/.test(type)) && require.resolve("../bin/node-scripts/test.examples"),
    ].sort().forEach(function(filepath) {
      filepath && mocha.addFile(filepath);
    })

    mocha.run(function(failures) {
      resolve(failures);
    });
  });
}
