



(function () { 

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");

const backgroundPageThumbsContent = {

  init: function () {
    
    
    this._webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);
    addMessageListener("BackgroundPageThumbs:capture",
                       this._onCapture.bind(this));
  },

  get _webNav() {
    return docShell.QueryInterface(Ci.nsIWebNavigation);
  },

  _onCapture: function (msg) {
    if (this._onLoad) {
      this._webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);
      removeEventListener("load", this._onLoad, true);
    }

    this._onLoad = function onLoad(event) {
      if (event.target != content.document)
        return;
      removeEventListener("load", this._onLoad, true);
      delete this._onLoad;

      
      this._sizeViewport();
      let canvas = PageThumbs._createCanvas(content);
      PageThumbs._captureToCanvas(content, canvas);

      let finalURL = this._webNav.currentURI.spec;
      let fileReader = Cc["@mozilla.org/files/filereader;1"].
                       createInstance(Ci.nsIDOMFileReader);
      fileReader.onloadend = function onArrayBufferLoad() {
        sendAsyncMessage("BackgroundPageThumbs:didCapture", {
          id: msg.json.id,
          imageData: fileReader.result,
          finalURL: finalURL,
        });
      };
      canvas.toBlob(blob => fileReader.readAsArrayBuffer(blob));
    }.bind(this);

    addEventListener("load", this._onLoad, true);
    this._webNav.loadURI(msg.json.url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                         null, null, null);
  },

  _sizeViewport: function () {
    let width = {};
    let height = {};
    Cc["@mozilla.org/gfx/screenmanager;1"].
      getService(Ci.nsIScreenManager).
      primaryScreen.
      GetRect({}, {}, width, height);
    content.
      QueryInterface(Ci.nsIInterfaceRequestor).
      getInterface(Ci.nsIDOMWindowUtils).
      setCSSViewport(width.value, height.value);
  },
};

backgroundPageThumbsContent.init();

})();
