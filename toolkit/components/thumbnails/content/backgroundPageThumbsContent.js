



(function () { 

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PageThumbs.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const backgroundPageThumbsContent = {

  init: function () {
    Services.obs.addObserver(this, "document-element-inserted", true);

    
    
    this._webNav.QueryInterface(Ci.nsIDocumentLoader).
      loadGroup.QueryInterface(Ci.nsISupportsPriority).
      priority = Ci.nsISupportsPriority.PRIORITY_LOWEST;

    docShell.allowMedia = false;
    docShell.allowPlugins = false;
    docShell.allowContentRetargeting = false;
    let defaultFlags = Ci.nsIRequest.LOAD_ANONYMOUS |
                       Ci.nsIRequest.LOAD_BYPASS_CACHE |
                       Ci.nsIRequest.INHIBIT_CACHING |
                       Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_HISTORY;
    docShell.defaultLoadFlags = defaultFlags;

    addMessageListener("BackgroundPageThumbs:capture",
                       this._onCapture.bind(this));
    docShell.
      QueryInterface(Ci.nsIInterfaceRequestor).
      getInterface(Ci.nsIWebProgress).
      addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);
  },

  observe: function (subj, topic, data) {
    
    
    
    
    if (subj == content.document) {
      content.
        QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils).
        preventFurtherDialogs();
    }
  },

  get _webNav() {
    return docShell.QueryInterface(Ci.nsIWebNavigation);
  },

  _onCapture: function (msg) {
    this._webNav.loadURI(msg.json.url,
                         Ci.nsIWebNavigation.LOAD_FLAGS_STOP_CONTENT,
                         null, null, null);
    
    
    this._requestID = msg.json.id;
    this._requestDate = new Date();
  },

  onStateChange: function (webProgress, req, flags, status) {
    if (!webProgress.isTopLevel ||
        !(flags & Ci.nsIWebProgressListener.STATE_STOP) ||
        req.name == "about:blank")
      return;

    let requestID = this._requestID;
    let pageLoadTime = new Date() - this._requestDate;
    delete this._requestID;

    let canvas = PageThumbs._createCanvas(content);
    let captureDate = new Date();
    PageThumbs._captureToCanvas(content, canvas);
    let captureTime = new Date() - captureDate;

    let finalURL = this._webNav.currentURI.spec;
    let fileReader = Cc["@mozilla.org/files/filereader;1"].
                     createInstance(Ci.nsIDOMFileReader);
    fileReader.onloadend = () => {
      sendAsyncMessage("BackgroundPageThumbs:didCapture", {
        id: requestID,
        imageData: fileReader.result,
        finalURL: finalURL,
        telemetry: {
          CAPTURE_PAGE_LOAD_TIME_MS: pageLoadTime,
          CAPTURE_CANVAS_DRAW_TIME_MS: captureTime,
        },
      });
    };
    canvas.toBlob(blob => fileReader.readAsArrayBuffer(blob));

    
    
    
    Services.tm.mainThread.dispatch(() => {
      if (!("_requestID" in this))
        this._webNav.loadURI("about:blank",
                             Ci.nsIWebNavigation.LOAD_FLAGS_STOP_CONTENT,
                             null, null, null);
    }, Ci.nsIEventTarget.DISPATCH_NORMAL);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

backgroundPageThumbsContent.init();

})();
