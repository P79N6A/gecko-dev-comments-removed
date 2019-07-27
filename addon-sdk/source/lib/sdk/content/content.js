


"use strict";

module.metadata = {
  "stability": "deprecated"
};

const { deprecateUsage } = require('../util/deprecate');

Object.defineProperty(exports, "Worker", {
  get: function() {
    deprecateUsage('`sdk/content/content` is deprecated. Please use `sdk/content/worker` directly.');
    return require('./worker').Worker;
  }
});
