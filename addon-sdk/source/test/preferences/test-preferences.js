


"use strict";

var extend = require("lodash").extend

var e10sOff = require("./e10s-off.json");
var commonPrefs = require("./common.json");
var testPrefs = require("./test.json");
var fxPrefs = require("./firefox.json");
var disconnectionPrefs = require("./no-connections.json");

var prefs = extend({}, e10sOff, commonPrefs, testPrefs, disconnectionPrefs, fxPrefs);
module.exports = prefs;
