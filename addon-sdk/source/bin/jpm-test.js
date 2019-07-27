


"use strict";

var BLACKLIST = [];
var readParam = require("./node-scripts/utils").readParam;
var path = require("path");
var Mocha = require("mocha");
var mocha = new Mocha({
  ui: "bdd",
  reporter: "spec",
  timeout: 900000
});

var type = readParam("type");

[
  (!type || type == "modules") && require.resolve("../bin/node-scripts/test.modules"),
  (!type || type == "addons") && require.resolve("../bin/node-scripts/test.addons"),
  (!type || type == "examples") && require.resolve("../bin/node-scripts/test.examples"),
].sort().forEach(function(filepath) {
  filepath && mocha.addFile(filepath);
})

mocha.run(function (failures) {
  process.exit(failures);
});
