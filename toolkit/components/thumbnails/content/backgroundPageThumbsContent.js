



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.importGlobalProperties(['Blob']);

Cu.import("resource://gre/modules/PageThumbUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const STATE_LOADING = 1;
const STATE_CAPTURING = 2;
const STATE_CANCELED = 3;

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
    
    
    
    
    if (content && subj == content.document) {
      content.
        QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils).
        disableDialogs();
    }
  },

  get _webNav() {
    return docShell.QueryInterface(Ci.nsIWebNavigation);
  },

  _onCapture: function (msg) {
    this._nextCapture = {
      id: msg.data.id,
      url: msg.data.url,
    };
    if (this._currentCapture) {
      if (this._state == STATE_LOADING) {
        
        this._state = STATE_CANCELED;
        this._loadAboutBlank();
      }
      
      
      return;
    }
    this._startNextCapture();
  },

  _startNextCapture: function () {
    if (!this._nextCapture)
      return;
    this._currentCapture = this._nextCapture;
    delete this._nextCapture;
    this._state = STATE_LOADING;
    this._currentCapture.pageLoadStartDate = new Date();

    try {
      this._webNav.loadURI(this._currentCapture.url,
                           Ci.nsIWebNavigation.LOAD_FLAGS_STOP_CONTENT,
                           null, null, null);
    } catch (e) {
      this._failCurrentCapture("BAD_URI");
      delete this._currentCapture;
      this._startNextCapture();
    }
  },

  onStateChange: function (webProgress, req, flags, status) {
    if (webProgress.isTopLevel &&
        (flags & Ci.nsIWebProgressListener.STATE_STOP) &&
        this._currentCapture) {
      if (req.name == "about:blank") {
        if (this._state == STATE_CAPTURING) {
          
          this._finishCurrentCapture();
          delete this._currentCapture;
          this._startNextCapture();
        }
        else if (this._state == STATE_CANCELED) {
          
          
          delete this._currentCapture;
          this._startNextCapture();
        }
      }
      else if (this._state == STATE_LOADING) {
        
        this._state = STATE_CAPTURING;
        this._captureCurrentPage();
      }
    }
  },

  _captureCurrentPage: function () {
    let capture = this._currentCapture;
    capture.finalURL = this._webNav.currentURI.spec;
    capture.pageLoadTime = new Date() - capture.pageLoadStartDate;

    let canvasDrawDate = new Date();

    let canvas = PageThumbUtils.createCanvas(content);
    let [sw, sh, scale] = PageThumbUtils.determineCropSize(content, canvas);

    let ctx = canvas.getContext("2d");
    ctx.save();
    ctx.scale(scale, scale);
    ctx.drawWindow(content, 0, 0, sw, sh,
                   PageThumbUtils.THUMBNAIL_BG_COLOR,
                   ctx.DRAWWINDOW_DO_NOT_FLUSH);
    ctx.restore();

    capture.canvasDrawTime = new Date() - canvasDrawDate;

    canvas.toBlob(blob => {
      capture.imageBlob = new Blob([blob]);
      
      this._loadAboutBlank();
    });
  },

  _finishCurrentCapture: function () {
    let capture = this._currentCapture;
    let fileReader = Cc["@mozilla.org/files/filereader;1"].
                     createInstance(Ci.nsIDOMFileReader);
    fileReader.onloadend = () => {
      sendAsyncMessage("BackgroundPageThumbs:didCapture", {
        id: capture.id,
        imageData: fileReader.result,
        finalURL: capture.finalURL,
        telemetry: {
          CAPTURE_PAGE_LOAD_TIME_MS: capture.pageLoadTime,
          CAPTURE_CANVAS_DRAW_TIME_MS: capture.canvasDrawTime,
        },
      });
    };
    fileReader.readAsArrayBuffer(capture.imageBlob);
  },

  _failCurrentCapture: function (reason) {
    let capture = this._currentCapture;
    sendAsyncMessage("BackgroundPageThumbs:didCapture", {
      id: capture.id,
      failReason: reason,
    });
  },

  
  
  
  _loadAboutBlank: function _loadAboutBlank() {
    this._webNav.loadURI("about:blank",
                         Ci.nsIWebNavigation.LOAD_FLAGS_STOP_CONTENT,
                         null, null, null);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

backgroundPageThumbsContent.init();
