


'use strict';

module.metadata = {
  "stability": "stable"
};

const { search, remove, store } = require("./passwords/utils");
const { defer, delay } = require("./lang/functional");










function getCallbacks(options) {
  let value = [
    'onComplete' in options ? options.onComplete : null,
    'onError' in options ? defer(options.onError) : console.exception
  ];

  delete options.onComplete;
  delete options.onError;

  return value;
};






function createWrapperMethod(wrapped) {
  return function (options) {
    let [ onComplete, onError ] = getCallbacks(options);
    try {
      let value = wrapped(options);
      if (onComplete) {
        delay(function() {
          try {
            onComplete(value);
          } catch (exception) {
            onError(exception);
          }
        });
      }
    } catch (exception) {
      onError(exception);
    }
  };
}

exports.search = createWrapperMethod(search);
exports.store = createWrapperMethod(store);
exports.remove = createWrapperMethod(remove);
