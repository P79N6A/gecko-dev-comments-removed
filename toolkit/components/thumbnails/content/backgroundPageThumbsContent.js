



(function () { 

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");

const backgroundPageThumbsContent = {

  init: function () {
    
    
    let dwu = content.
                QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils);
    dwu.preventFurtherDialogs();

    
    
    this._webNav.QueryInterface(Ci.nsIDocumentLoader).
      loadGroup.QueryInterface(Ci.nsISupportsPriority).
      priority = Ci.nsISupportsPriority.PRIORITY_LOWEST;

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
      let pageLoadTime = new Date() - loadDate;
      removeEventListener("load", this._onLoad, true);
      delete this._onLoad;

      let canvas = PageThumbs._createCanvas(content);
      let captureDate = new Date();
      PageThumbs._captureToCanvas(content, canvas);
      let captureTime = new Date() - captureDate;

      let finalURL = this._webNav.currentURI.spec;
      let fileReader = Cc["@mozilla.org/files/filereader;1"].
                       createInstance(Ci.nsIDOMFileReader);
      fileReader.onloadend = function onArrayBufferLoad() {
        sendAsyncMessage("BackgroundPageThumbs:didCapture", {
          id: msg.json.id,
          imageData: fileReader.result,
          finalURL: finalURL,
          telemetry: {
            CAPTURE_PAGE_LOAD_TIME_MS: pageLoadTime,
            CAPTURE_CANVAS_DRAW_TIME_MS: captureTime,
          },
        });
      };
      canvas.toBlob(blob => fileReader.readAsArrayBuffer(blob));

      
      
      this._webNav.loadURI("about:blank", Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                           null, null, null);
    }.bind(this);

    addEventListener("load", this._onLoad, true);
    this._webNav.loadURI(msg.json.url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                         null, null, null);
    let loadDate = new Date();
  },
};

backgroundPageThumbsContent.init();

})();
