


'use strict';

const { Cu } = require("chrome");

function gc() {
  return new Promise(resolve => Cu.schedulePreciseGC(resolve));
}
exports.gc = gc;
