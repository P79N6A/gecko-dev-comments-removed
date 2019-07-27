


'use strict';

var foo = require("foo");
var coolTabs = require("cool-tabs");

exports.foo = foo.fs;
exports.bar = foo.bar;
exports.fs = require("sdk/io/fs");
exports.extra = require("fs-extra").extra;
exports.overload = require("overload");
exports.overloadLib = require("overload/lib/foo.js");
exports.internal = require("internal").internal;
exports.Tabs = require("sdk/tabs").Tabs;
exports.CoolTabs = coolTabs.Tabs;
exports.CoolTabsLib = coolTabs.TabsLib;
exports.ignore = require("./lib/ignore").foo;
