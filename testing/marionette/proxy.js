



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("chrome://marionette/content/modal.js");

this.EXPORTED_SYMBOLS = ["proxy"];

const MARIONETTE_OK = "Marionette:ok";
const MARIONETTE_DONE = "Marionette:done";
const MARIONETTE_ERROR = "Marionette:error";

const logger = Log.repository.getLogger("Marionette");
const uuidgen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);

this.proxy = {};



















proxy.toListener = function(mmFn, sendAsyncFn) {
  let sender = new ContentSender(mmFn, sendAsyncFn);
  let handler = {
    get: (obj, prop) => {
      if (obj.hasOwnProperty(prop)) {
        return obj[prop];
      }
      return (...args) => obj.send(prop, args);
    }
  };
  return new Proxy(sender, handler);
};


















let ContentSender = function(mmFn, sendAsyncFn) {
  this.curId = null;
  this.sendAsync = sendAsyncFn;
  this.mmFn_ = mmFn;
};

Object.defineProperty(ContentSender.prototype, "mm", {
  get: function() { return this.mmFn_(); }
});
















ContentSender.prototype.send = function(name, args) {
  this.curId = uuidgen.generateUUID().toString();

  let proxy = new Promise((resolve, reject) => {
    let removeListeners = (n, fn) => {
      let rmFn = msg => {
        if (this.curId !== msg.json.command_id) {
          logger.warn("Skipping out-of-sync response from listener: " +
              `Expected response to \`${name}' with ID ${this.curId}, ` +
              "but got: " + msg.name + msg.json.toSource());
          return;
        }

        listeners.remove();
        modal.removeHandler(handleDialog);

        fn(msg);
        this.curId = null;
      };

      listeners.push([n, rmFn]);
      return rmFn;
    };

    let listeners = [];
    listeners.add = () => {
      this.mm.addMessageListener(MARIONETTE_OK, removeListeners(MARIONETTE_OK, okListener));
      this.mm.addMessageListener(MARIONETTE_DONE, removeListeners(MARIONETTE_DONE, valListener));
      this.mm.addMessageListener(MARIONETTE_ERROR, removeListeners(MARIONETTE_ERROR, errListener));
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

    this.sendAsync(name, msg, this.curId);
  });

  return proxy;
};

proxy.ContentSender = ContentSender;
