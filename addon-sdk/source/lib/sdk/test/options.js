


"use strict";

module.metadata = {
  "stability": "unstable"
};

const options = require("@test/options");
const { id } = require("../self");
const { get } = require("../preferences/service");

const readPref = (key) => get("extensions." + id + ".sdk." + key);

exports.iterations = readPref("test.iterations") || options.iterations;
exports.filter = readPref("test.filter") || options.filter;
exports.profileMemory = readPref("profile.memory") || options.profileMemory,
exports.stopOnError = readPref("test.stop") || options.stopOnError,
exports.verbose = (readPref("output.logLevel") == "verbose") || options.verbose;
exports.parseable = (readPref("output.format") == "tbpl") || options.parseable;
exports.checkMemory = readPref("profile.leaks") || options.check_memory;
