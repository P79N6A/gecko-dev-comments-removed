





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

let EventEmitter = require("devtools/toolkit/event-emitter");

loader.lazyGetter(this, "StorageFront",
  () => require("devtools/server/actors/storage").StorageFront);

loader.lazyGetter(this, "StorageUI",
  () => require("devtools/storage/ui").StorageUI);

this.StoragePanel = function StoragePanel(panelWin, toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;
  this._target = toolbox.target;
  this._panelWin = panelWin;

  this.destroy = this.destroy.bind(this);
}

exports.StoragePanel = StoragePanel;

StoragePanel.prototype = {
  get target() this._toolbox.target,

  get panelWindow() this._panelWin,

  


  open: function() {
    let targetPromise;
    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = Promise.resolve(this.target);
    }

    return targetPromise.then(() => {
      this.target.on("close", this.destroy);
      this._front = new StorageFront(this.target.client, this.target.form);

      this.UI = new StorageUI(this._front, this._target, this._panelWin);
      this.isReady = true;
      this.emit("ready");
      return this;
    }, console.error);
  },

  


  destroy: function() {
    if (!this._destroyed) {
      this.UI.destroy();
      this._destroyed = true;

      this._target.off("close", this.destroy);
      this._target = null;
      this._toolbox = null;
      this._panelDoc = null;
    }

    return Promise.resolve(null);
  },
}

XPCOMUtils.defineLazyGetter(StoragePanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/storage.properties");
  });
