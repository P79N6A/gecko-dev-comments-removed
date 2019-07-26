



"use strict";

module.metadata = {
  "stability": "unstable"
};

const {
  exit, version, stdout, stderr, platform, architecture
} = require("../system");





exports.stdout = stdout;
exports.stderr = stderr;
exports.version = version;
exports.versions = {};
exports.config = {};
exports.arch = architecture;
exports.platform = platform;
exports.exit = exit;







exports.nextTick = (callback) => setTimeout(callback, 0);





exports.maxTickDepth = 1000;
exports.pid = 0;
exports.title = "";
exports.stdin = {};
exports.argv = [];
exports.execPath = "";
exports.execArgv = [];
exports.abort = function () {};
exports.chdir = function () {};
exports.cwd = function () {};
exports.env = {};
exports.getgid = function () {};
exports.setgid = function () {};
exports.getuid = function () {};
exports.setuid = function () {};
exports.getgroups = function () {};
exports.setgroups = function () {};
exports.initgroups = function () {};
exports.kill = function () {};
exports.memoryUsage = function () {};
exports.umask = function () {};
exports.uptime = function () {};
exports.hrtime = function () {};
