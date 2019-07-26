


'use strict';

const { validateOptions } = require('../deprecated/api-utils');

function Options(options) {
  if ('string' === typeof options)
    options = { url: options };

  return validateOptions(options, {
    url: { is: ["string"] },
    inBackground: {
      map: function(v) !!v,
      is: ["undefined", "boolean"]
    },
    isPinned: { is: ["undefined", "boolean"] },
    isPrivate: { is: ["undefined", "boolean"] },
    onOpen: { is: ["undefined", "function"] },
    onClose: { is: ["undefined", "function"] },
    onReady: { is: ["undefined", "function"] },
    onActivate: { is: ["undefined", "function"] },
    onDeactivate: { is: ["undefined", "function"] }
  });
}
exports.Options = Options;
