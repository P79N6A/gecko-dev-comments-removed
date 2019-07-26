



(function () { 

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");

const backgroundPageThumbsContent = {

  init: function () {
    
    
    let dwu = content.
                QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils);
    dwu.preventFurtherDialogs();

    docShell.allowMedia = false;
    docShell.allowPlugins = false;

    addMessageListener("BackgroundPageThumbs:capture",
                       this._onCapture.bind(this));
  },

  get _webNav() {
    return docShell.QueryInterface(Ci.nsIWebNavigation);
  },

  _onCapture: function (msg) {
    this._webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);
    if (this._onLoad)
      removeEventListener("load", this._onLoad, true);

    this._onLoad = function onLoad(event) {
      if (event.target != content.document)
        return;
      removeEventListener("load", this._onLoad, true);
      delete this._onLoad;

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

      
      
      this._webNav.loadURI("about:blank", Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                           null, null, null);
    }.bind(this);

    addEventListener("load", this._onLoad, true);
    this._webNav.loadURI(msg.json.url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                         null, null, null);
  },
};

backgroundPageThumbsContent.init();

})();
