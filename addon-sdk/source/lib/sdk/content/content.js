


"use strict";

module.metadata = {
  "stability": "deprecated"
};

const { deprecateFunction } = require('../util/deprecate');

exports.Loader = deprecateFunction(require('./loader').Loader,
  '`sdk/content/content` is deprecated. Please use `sdk/content/loader` directly.');
exports.Symbiont = deprecateFunction(require('../deprecated/symbiont').Symbiont,
  'Both `sdk/content/content` and `sdk/deprecated/symbiont` are deprecated. ' +
  '`sdk/core/heritage` supersedes Symbiont for inheritance.');
exports.Worker = deprecateFunction(require('./worker').Worker,
  '`sdk/content/content` is deprecated. Please use `sdk/content/worker` directly.');

