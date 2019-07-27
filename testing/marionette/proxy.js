



"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("chrome://marionette/content/modal.js");

this.EXPORTED_SYMBOLS = ["proxy"];

const logger = Log.repository.getLogger("Marionette");

this.proxy = {};



















proxy.toListener = function(mmFn, sendAsyncFn) {
  let sender = new ContentSender(mmFn, sendAsyncFn);
  let handler = {
    set: (obj, prop, val) => { obj[prop] = val; return true; },
    get: (obj, prop) => (...args) => obj.send(prop, args),
  };
  return new Proxy(sender, handler);
};


















let ContentSender = function(mmFn, sendAsyncFn) {
  this.curCmdId = null;
  this.sendAsync = sendAsyncFn;
  this.mmFn_ = mmFn;
};

Object.defineProperty(ContentSender.prototype, "mm", {
  get: function() { return this.mmFn_(); }
});
















ContentSender.prototype.send = function(name, args) {
  const ok = "Marionette:ok";
  const val = "Marionette:done";
  const err = "Marionette:error";

  let proxy = new Promise((resolve, reject) => {
    let removeListeners = (name, fn) => {
      let rmFn = msg => {
        if (this.isOutOfSync(msg.json.command_id)) {
          logger.warn("Skipping out-of-sync response from listener: " +
              msg.name + msg.json.toSource());
          return;
        }

        listeners.remove();
        modal.removeHandler(handleDialog);

        fn(msg);
        this.curCmdId = null;
      };

      listeners.push([name, rmFn]);
      return rmFn;
    };

    let listeners = [];
    listeners.add = () => {
      this.mm.addMessageListener(ok, removeListeners(ok, okListener));
      this.mm.addMessageListener(val, removeListeners(val, valListener));
      this.mm.addMessageListener(err, removeListeners(err, errListener));
    };
    listeners.remove = () =>
        listeners.map(l => this.mm.removeMessageListener(l[0], l[1]));

    let okListener = () => resolve();
    let valListener = msg => resolve(msg.json.value);
    let errListener = msg => reject(msg.objects.error);

    let handleDialog = function(subject, topic) {
      listeners.remove();
      modal.removeHandler(handleDialog);
      this.sendAsync("cancelRequest");
      resolve();
    }.bind(this);

    
    
    listeners.add();
    modal.addHandler(handleDialog);

    
    
    let msg = args;
    if (args.length == 1 && typeof args[0] == "object") {
      msg = args[0];
    }

    this.sendAsync(name, msg, this.curCmdId);
  });

  return proxy;
};

ContentSender.prototype.isOutOfSync = function(id) {
  return this.curCmdId !== id;
};

proxy.ContentSender = ContentSender;
