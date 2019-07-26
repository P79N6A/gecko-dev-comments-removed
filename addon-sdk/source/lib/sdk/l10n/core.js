


"use strict";

const json = require("./json/core");
const properties = require("./properties/core");

exports.get = json.usingJSON ? json.get : properties.get;
