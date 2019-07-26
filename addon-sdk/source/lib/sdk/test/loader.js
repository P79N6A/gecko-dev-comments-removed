



"use strict";

const { Loader, resolveURI, Require,
        unload, override, descriptor  } = require('../loader/cuddlefish');
const addonWindow = require('../addon/window');
const { PlainTextConsole } = require("sdk/console/plain-text");

function CustomLoader(module, globals, packaging) {
  let options = packaging || require("@loader/options");
  options = override(options, {
    globals: override(require('../system/globals'), globals || {}),
    modules: override(options.modules || {}, {
      'sdk/addon/window': addonWindow
     })
  });

  let loader = Loader(options);
  return Object.create(loader, descriptor({
    require: Require(loader, module),
    sandbox: function(id) {
      let requirement = loader.resolve(id, module.id);
      let uri = resolveURI(requirement, loader.mapping);
      return loader.sandboxes[uri];
    },
    unload: function(reason) {
      unload(loader, reason);
    }
  }));
};
exports.Loader = CustomLoader;




exports.LoaderWithHookedConsole = function (module, callback) {
  let messages = [];
  function hook(msg) {
    messages.push({type: this, msg: msg});
    if (callback)
      callback(this, msg);
  }
  return {
    loader: CustomLoader(module, {
      console: {
        log: hook.bind("log"),
        info: hook.bind("info"),
        warn: hook.bind("warn"),
        error: hook.bind("error"),
        debug: hook.bind("debug"),
        exception: hook.bind("exception"),
        __exposedProps__: {
          log: "rw", info: "rw", warn: "rw", error: "rw", debug: "rw",
          exception: "rw"
        }
      }
    }),
    messages: messages
  };
}



exports.LoaderWithHookedConsole2 = function (module, callback) {
  let messages = [];
  return {
    loader: CustomLoader(module, {
      console: new PlainTextConsole(function (msg) {
        messages.push(msg);
        if (callback)
          callback(msg);
      })
    }),
    messages: messages
  };
}
