











































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["HUDService", "ConsoleUtils"];

XPCOMUtils.defineLazyServiceGetter(this, "scriptError",
                                   "@mozilla.org/scripterror;1",
                                   "nsIScriptError");

XPCOMUtils.defineLazyServiceGetter(this, "activityDistributor",
                                   "@mozilla.org/network/http-activity-distributor;1",
                                   "nsIHttpActivityDistributor");

XPCOMUtils.defineLazyServiceGetter(this, "sss",
                                   "@mozilla.org/content/style-sheet-service;1",
                                   "nsIStyleSheetService");

XPCOMUtils.defineLazyServiceGetter(this, "mimeService",
                                   "@mozilla.org/mime;1",
                                   "nsIMIMEService");

XPCOMUtils.defineLazyGetter(this, "NetUtil", function () {
  var obj = {};
  Cu.import("resource://gre/modules/NetUtil.jsm", obj);
  return obj.NetUtil;
});

XPCOMUtils.defineLazyGetter(this, "PropertyPanel", function () {
  var obj = {};
  try {
    Cu.import("resource://gre/modules/PropertyPanel.jsm", obj);
  } catch (err) {
    Cu.reportError(err);
  }
  return obj.PropertyPanel;
});

XPCOMUtils.defineLazyGetter(this, "namesAndValuesOf", function () {
  var obj = {};
  Cu.import("resource://gre/modules/PropertyPanel.jsm", obj);
  return obj.namesAndValuesOf;
});

function LogFactory(aMessagePrefix)
{
  function log(aMessage) {
    var _msg = aMessagePrefix + " " + aMessage + "\n";
    dump(_msg);
  }
  return log;
}

let log = LogFactory("*** HUDService:");

const HUD_STYLESHEET_URI = "chrome://global/skin/webConsole.css";
const HUD_STRINGS_URI = "chrome://global/locale/headsUpDisplay.properties";

XPCOMUtils.defineLazyGetter(this, "stringBundle", function () {
  return Services.strings.createBundle(HUD_STRINGS_URI);
});



const NEW_GROUP_DELAY = 5000;



const SEARCH_DELAY = 200;




const DEFAULT_LOG_LIMIT = 200;


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;


const RESPONSE_BODY_LIMIT = 1048576; 

const ERRORS = { LOG_MESSAGE_MISSING_ARGS:
                 "Missing arguments: aMessage, aConsoleNode and aMessageNode are required.",
                 CANNOT_GET_HUD: "Cannot getHeads Up Display with provided ID",
                 MISSING_ARGS: "Missing arguments",
                 LOG_OUTPUT_FAILED: "Log Failure: Could not append messageNode to outputNode",
};
















function ResponseListener(aHttpActivity) {
  this.receivedData = "";
  this.httpActivity = aHttpActivity;
}

ResponseListener.prototype =
{
  


  originalListener: null,

  


  httpActivity: null,

  


  receivedData: null,

  




  setResponseHeader: function RL_setResponseHeader(aRequest)
  {
    let httpActivity = this.httpActivity;
    
    if (!httpActivity.response.header) {
      if (aRequest instanceof Ci.nsIHttpChannel) {
      httpActivity.response.header = {};
        try {
        aRequest.visitResponseHeaders({
          visitHeader: function(aName, aValue) {
            httpActivity.response.header[aName] = aValue;
          }
        });
      }
        
        
        
        
        catch (ex) {
          delete httpActivity.response.header;
        }
      }
    }
  },

  











  onDataAvailable: function RL_onDataAvailable(aRequest, aContext, aInputStream,
                                                aOffset, aCount)
  {
    this.setResponseHeader(aRequest);

    let StorageStream = Components.Constructor("@mozilla.org/storagestream;1",
                                                "nsIStorageStream",
                                                "init");
    let BinaryOutputStream = Components.Constructor("@mozilla.org/binaryoutputstream;1",
                                                      "nsIBinaryOutputStream",
                                                      "setOutputStream");

    storageStream = new StorageStream(8192, aCount, null);
    binaryOutputStream = new BinaryOutputStream(storageStream.getOutputStream(0));

    let data = NetUtil.readInputStreamToString(aInputStream, aCount);

    if (HUDService.saveRequestAndResponseBodies &&
        this.receivedData.length < RESPONSE_BODY_LIMIT) {
      this.receivedData += data;
    }

    binaryOutputStream.writeBytes(data, aCount);

    let newInputStream = storageStream.newInputStream(0);
    try {
    this.originalListener.onDataAvailable(aRequest, aContext,
        newInputStream, aOffset, aCount);
    }
    catch(ex) {
      aRequest.cancel(ex);
    }
  },

  






  onStartRequest: function RL_onStartRequest(aRequest, aContext)
  {
    try {
    this.originalListener.onStartRequest(aRequest, aContext);
    }
    catch(ex) {
      aRequest.cancel(ex);
    }
  },

  












  onStopRequest: function RL_onStopRequest(aRequest, aContext, aStatusCode)
  {
    try {
    this.originalListener.onStopRequest(aRequest, aContext, aStatusCode);
    }
    catch (ex) { }

    this.setResponseHeader(aRequest);

    if (HUDService.saveRequestAndResponseBodies) {
      this.httpActivity.response.body = this.receivedData;
    }
    else {
      this.httpActivity.response.bodyDiscarded = true;
    }

    if (HUDService.lastFinishedRequestCallback) {
      HUDService.lastFinishedRequestCallback(this.httpActivity);
    }

    
    this.httpActivity.panels.forEach(function(weakRef) {
      let panel = weakRef.get();
      if (panel) {
        panel.update();
      }
    });
    this.httpActivity.response.isDone = true;
    this.httpActivity.response.listener = null;
    this.httpActivity = null;
    this.receivedData = "";
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIStreamListener,
    Ci.nsISupports
  ])
}





























































var NetworkHelper =
{
  









  convertToUnicode: function NH_convertToUnicode(aText, aCharset)
  {
    let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
               createInstance(Ci.nsIScriptableUnicodeConverter);
    conv.charset = aCharset || "UTF-8";
    return conv.ConvertToUnicode(aText);
  },

  







  readAndConvertFromStream: function NH_readAndConvertFromStream(aStream, aCharset)
  {
    let text = null;
    try {
      text = NetUtil.readInputStreamToString(aStream, aStream.available())
      return this.convertToUnicode(text, aCharset);
    }
    catch (err) {
      return text;
    }
  },

   








  readPostTextFromRequest: function NH_readPostTextFromRequest(aRequest, aBrowser)
  {
    if (aRequest instanceof Ci.nsIUploadChannel) {
      let iStream = aRequest.uploadStream;

      let isSeekableStream = false;
      if (iStream instanceof Ci.nsISeekableStream) {
        isSeekableStream = true;
      }

      let prevOffset;
      if (isSeekableStream) {
        prevOffset = iStream.tell();
        iStream.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
      }

      
      let charset = aBrowser.contentWindow.document.characterSet;
      let text = this.readAndConvertFromStream(iStream, charset);

      
      
      
      if (isSeekableStream && prevOffset == 0) {
        iStream.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
      }
      return text;
    }
    return null;
  },

  







  readPostTextFromPage: function NH_readPostTextFromPage(aBrowser)
  {
    let webNav = aBrowser.webNavigation;
    if (webNav instanceof Ci.nsIWebPageDescriptor) {
      let descriptor = webNav.currentDescriptor;

      if (descriptor instanceof Ci.nsISHEntry && descriptor.postData &&
          descriptor instanceof Ci.nsISeekableStream) {
        descriptor.seek(NS_SEEK_SET, 0);

        let charset = browser.contentWindow.document.characterSet;
        return this.readAndConvertFromStream(descriptor, charset);
      }
    }
    return null;
  },

  





  getWindowForRequest: function NH_getWindowForRequest(aRequest)
  {
    let loadContext = this.getRequestLoadContext(aRequest);
    if (loadContext) {
      return loadContext.associatedWindow;
    }
    return null;
  },

  





  getRequestLoadContext: function NH_getRequestLoadContext(aRequest)
  {
    if (aRequest && aRequest.notificationCallbacks) {
      try {
        return aRequest.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    if (aRequest && aRequest.loadGroup
                 && aRequest.loadGroup.notificationCallbacks) {
      try {
        return aRequest.loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    return null;
  },

  











  loadFromCache: function NH_loadFromCache(aUrl, aCharset, aCallback)
  {
    let channel = NetUtil.newChannel(aUrl);

    
    channel.loadFlags = Ci.nsIRequest.LOAD_FROM_CACHE |
      Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
      Ci.nsICachingChannel.LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

    NetUtil.asyncFetch(channel, function (aInputStream, aStatusCode, aRequest) {
      if (!Components.isSuccessCode(aStatusCode)) {
        aCallback(null);
        return;
      }

      
      
      let aChannel = aRequest.QueryInterface(Ci.nsIChannel);
      let contentCharset = aChannel.contentCharset || aCharset;

      
      aCallback(NetworkHelper.readAndConvertFromStream(aInputStream,
                                                       contentCharset));
    });
  },

  
  
  mimeCategoryMap: {
    "text/plain": "txt",
    "text/html": "html",
    "text/xml": "xml",
    "text/xsl": "txt",
    "text/xul": "txt",
    "text/css": "css",
    "text/sgml": "txt",
    "text/rtf": "txt",
    "text/x-setext": "txt",
    "text/richtext": "txt",
    "text/javascript": "js",
    "text/jscript": "txt",
    "text/tab-separated-values": "txt",
    "text/rdf": "txt",
    "text/xif": "txt",
    "text/ecmascript": "js",
    "text/vnd.curl": "txt",
    "text/x-json": "json",
    "text/x-js": "txt",
    "text/js": "txt",
    "text/vbscript": "txt",
    "view-source": "txt",
    "view-fragment": "txt",
    "application/xml": "xml",
    "application/xhtml+xml": "xml",
    "application/atom+xml": "xml",
    "application/rss+xml": "xml",
    "application/vnd.mozilla.maybe.feed": "xml",
    "application/vnd.mozilla.xul+xml": "xml",
    "application/javascript": "js",
    "application/x-javascript": "js",
    "application/x-httpd-php": "txt",
    "application/rdf+xml": "xml",
    "application/ecmascript": "js",
    "application/http-index-format": "txt",
    "application/json": "json",
    "application/x-js": "txt",
    "multipart/mixed": "txt",
    "multipart/x-mixed-replace": "txt",
    "image/svg+xml": "svg",
    "application/octet-stream": "bin",
    "image/jpeg": "image",
    "image/jpg": "image",
    "image/gif": "image",
    "image/png": "image",
    "image/bmp": "image",
    "application/x-shockwave-flash": "flash",
    "video/x-flv": "flash",
    "audio/mpeg3": "media",
    "audio/x-mpeg-3": "media",
    "video/mpeg": "media",
    "video/x-mpeg": "media",
    "audio/ogg": "media",
    "application/ogg": "media",
    "application/x-ogg": "media",
    "application/x-midi": "media",
    "audio/midi": "media",
    "audio/x-mid": "media",
    "audio/x-midi": "media",
    "music/crescendo": "media",
    "audio/wav": "media",
    "audio/x-wav": "media",
    "text/json": "json",
    "application/x-json": "json",
    "application/json-rpc": "json"
  }
}



















function createElement(aDocument, aTag, aAttributes)
{
  let node = aDocument.createElement(aTag);
  for (var attr in aAttributes) {
    node.setAttribute(attr, aAttributes[attr]);
  }
  return node;
}













function createAndAppendElement(aParent, aTag, aAttributes)
{
  let node = createElement(aParent.ownerDocument, aTag, aAttributes);
  aParent.appendChild(node);
  return node;
}












function NetworkPanel(aParent, aHttpActivity)
{
  let doc = aParent.ownerDocument;
  this.httpActivity = aHttpActivity;

  
  this.panel = createElement(doc, "panel", {
    label: HUDService.getStr("NetworkPanel.label"),
    titlebar: "normal",
    noautofocus: "true",
    noautohide: "true",
    close: "true"
  });

  
  this.iframe = createAndAppendElement(this.panel, "iframe", {
    src: "chrome://browser/content/NetworkPanel.xhtml",
    type: "content",
    flex: "1"
  });

  let self = this;

  
  this.panel.addEventListener("popuphidden", function onPopupHide() {
    self.panel.removeEventListener("popuphidden", onPopupHide, false);
    self.panel.parentNode.removeChild(self.panel);
    self.panel = null;
    self.iframe = null;
    self.document = null;
    self.httpActivity = null;

    if (self.linkNode) {
      self.linkNode._panelOpen = false;
      self.linkNode = null;
    }
  }, false);

  
  this.panel.addEventListener("load", function onLoad() {
    self.panel.removeEventListener("load", onLoad, true)
    self.document = self.iframe.contentWindow.document;
    self.update();
  }, true);

  
  let footer = createElement(doc, "hbox", { align: "end" });
  createAndAppendElement(footer, "spacer", { flex: 1 });

  createAndAppendElement(footer, "resizer", { dir: "bottomend" });
  this.panel.appendChild(footer);

  aParent.appendChild(this.panel);
}

NetworkPanel.prototype =
{
  



  isDoneCallback: null,

  


  _state: 0,

  


  _INIT: 0,
  _DISPLAYED_REQUEST_HEADER: 1,
  _DISPLAYED_REQUEST_BODY: 2,
  _DISPLAYED_RESPONSE_HEADER: 3,
  _TRANSITION_CLOSED: 4,

  _fromDataRegExp: /Content-Type\:\s*application\/x-www-form-urlencoded/,

  











  _format: function NP_format(aName, aArray)
  {
    return HUDService.getFormatStr("NetworkPanel." + aName, aArray);
  },

  







  get _contentType()
  {
    let response = this.httpActivity.response;
    let contentTypeValue = null;

    if (response.header && response.header["Content-Type"]) {
      let types = response.header["Content-Type"].split(/,|;/);
      for (let i = 0; i < types.length; i++) {
        let type = NetworkHelper.mimeCategoryMap[types[i]];
        if (type) {
          return types[i];
        }
      }
    }

    
    let uri = NetUtil.newURI(this.httpActivity.url);
    let mimeType = null;
    if ((uri instanceof Ci.nsIURL) && uri.fileExtension) {
      try {
        mimeType = mimeService.getTypeFromExtension(uri.fileExtension);
      } catch(e) {
        
        Cu.reportError(e);
        
        return "";
      }
    }
    return mimeType;
  },

  




  get _responseIsImage()
  {
    return NetworkHelper.mimeCategoryMap[this._contentType] == "image";
  },

  




  get _isResponseBodyTextData()
  {
    let contentType = this._contentType;

    if (!contentType)
      return false;

    if (contentType.indexOf("text/") == 0) {
      return true;
    }

    switch (NetworkHelper.mimeCategoryMap[contentType]) {
      case "txt":
      case "js":
      case "json":
      case "css":
      case "html":
      case "svg":
      case "xml":
        return true;

      default:
        return false;
    }
  },

  





  get _isResponseCached()
  {
    return this.httpActivity.response.status.indexOf("304") != -1;
  },

  




  get _isRequestBodyFormData()
  {
    let requestBody = this.httpActivity.request.body;
    return this._fromDataRegExp.test(requestBody);
  },

  






  _appendTextNode: function NP_appendTextNode(aId, aValue)
  {
    let textNode = this.document.createTextNode(aValue);
    this.document.getElementById(aId).appendChild(textNode);
  },

  











  _appendList: function NP_appendList(aParentId, aList, aIgnoreCookie)
  {
    let parent = this.document.getElementById(aParentId);
    let doc = this.document;

    let sortedList = {};
    Object.keys(aList).sort().forEach(function(aKey) {
      sortedList[aKey] = aList[aKey];
    });

    for (let key in sortedList) {
      if (aIgnoreCookie && key == "Cookie") {
        continue;
      }

      







      let textNode = doc.createTextNode(key + ":");
      let span = doc.createElement("span");
      span.setAttribute("class", "property-name");
      span.appendChild(textNode);
      parent.appendChild(span);

      textNode = doc.createTextNode(sortedList[key]);
      span = doc.createElement("span");
      span.setAttribute("class", "property-value");
      span.appendChild(textNode);
      parent.appendChild(span);

      parent.appendChild(doc.createElement("br"));
    }
  },

  





  _displayNode: function NP_displayNode(aId)
  {
    this.document.getElementById(aId).style.display = "block";
  },

  








  _displayRequestHeader: function NP_displayRequestHeader()
  {
    let timing = this.httpActivity.timing;
    let request = this.httpActivity.request;

    this._appendTextNode("headUrl", this.httpActivity.url);
    this._appendTextNode("headMethod", this.httpActivity.method);

    this._appendTextNode("requestHeadersInfo",
      ConsoleUtils.timestampString(timing.REQUEST_HEADER/1000));

    this._appendList("requestHeadersContent", request.header, true);

    if ("Cookie" in request.header) {
      this._displayNode("requestCookie");

      let cookies = request.header.Cookie.split(";");
      let cookieList = {};
      let cookieListSorted = {};
      cookies.forEach(function(cookie) {
        let name, value;
        [name, value] = cookie.trim().split("=");
        cookieList[name] = value;
      });
      this._appendList("requestCookieContent", cookieList);
    }
  },

  





  _displayRequestBody: function NP_displayRequestBody() {
    this._displayNode("requestBody");
    this._appendTextNode("requestBodyContent", this.httpActivity.request.body);
  },

  





  _displayRequestForm: function NP_processRequestForm() {
    let requestBodyLines = this.httpActivity.request.body.split("\n");
    let formData = requestBodyLines[requestBodyLines.length - 1].
                      replace(/\+/g, " ").split("&");

    function unescapeText(aText)
    {
      try {
        return decodeURIComponent(aText);
      }
      catch (ex) {
        return decodeURIComponent(unescape(aText));
      }
    }

    let formDataObj = {};
    for (let i = 0; i < formData.length; i++) {
      let data = formData[i];
      let idx = data.indexOf("=");
      let key = data.substring(0, idx);
      let value = data.substring(idx + 1);
      formDataObj[unescapeText(key)] = unescapeText(value);
    }

    this._appendList("requestFormDataContent", formDataObj);
    this._displayNode("requestFormData");
  },

  






  _displayResponseHeader: function NP_displayResponseHeader()
  {
    let timing = this.httpActivity.timing;
    let response = this.httpActivity.response;

    this._appendTextNode("headStatus", response.status);

    let deltaDuration =
      Math.round((timing.RESPONSE_HEADER - timing.REQUEST_HEADER) / 1000);
    this._appendTextNode("responseHeadersInfo",
      this._format("durationMS", [deltaDuration]));

    this._displayNode("responseContainer");
    this._appendList("responseHeadersContent", response.header);
  },

  







  _displayResponseImage: function NP_displayResponseImage()
  {
    let self = this;
    let timing = this.httpActivity.timing;
    let response = this.httpActivity.response;
    let cached = "";

    if (this._isResponseCached) {
      cached = "Cached";
    }

    let imageNode = this.document.getElementById("responseImage" + cached +"Node");
    imageNode.setAttribute("src", this.httpActivity.url);

    
    function setImageInfo() {
      let deltaDuration =
        Math.round((timing.RESPONSE_COMPLETE - timing.RESPONSE_HEADER) / 1000);
      self._appendTextNode("responseImage" + cached + "Info",
        self._format("imageSizeDeltaDurationMS", [
          imageNode.width, imageNode.height, deltaDuration
        ]
      ));
    }

    
    if (imageNode.width != 0) {
      setImageInfo();
    }
    else {
      
      imageNode.addEventListener("load", function imageNodeLoad() {
        imageNode.removeEventListener("load", imageNodeLoad, false);
        setImageInfo();
      }, false);
    }

    this._displayNode("responseImage" + cached);
  },

  









  _displayResponseBody: function NP_displayResponseBody(aCachedContent)
  {
    let timing = this.httpActivity.timing;
    let response = this.httpActivity.response;
    let cached =  "";
    if (aCachedContent) {
      cached = "Cached";
    }

    let deltaDuration =
      Math.round((timing.RESPONSE_COMPLETE - timing.RESPONSE_HEADER) / 1000);
    this._appendTextNode("responseBody" + cached + "Info",
      this._format("durationMS", [deltaDuration]));

    this._displayNode("responseBody" + cached);
    this._appendTextNode("responseBody" + cached + "Content",
                            aCachedContent || response.body);
  },

  





  _displayResponseBodyUnknownType: function NP_displayResponseBodyUnknownType()
  {
    let timing = this.httpActivity.timing;

    this._displayNode("responseBodyUnknownType");
    let deltaDuration =
      Math.round((timing.RESPONSE_COMPLETE - timing.RESPONSE_HEADER) / 1000);
    this._appendTextNode("responseBodyUnknownTypeInfo",
      this._format("durationMS", [deltaDuration]));

    this._appendTextNode("responseBodyUnknownTypeContent",
      this._format("responseBodyUnableToDisplay.content", [this._contentType]));
  },

  





  _displayNoResponseBody: function NP_displayNoResponseBody()
  {
    let timing = this.httpActivity.timing;

    this._displayNode("responseNoBody");
    let deltaDuration =
      Math.round((timing.RESPONSE_COMPLETE - timing.RESPONSE_HEADER) / 1000);
    this._appendTextNode("responseNoBodyInfo",
      this._format("durationMS", [deltaDuration]));
  },

  


  _callIsDone: function() {
    if (this.isDoneCallback) {
      this.isDoneCallback();
    }
  },

  




  update: function NP_update()
  {
    




    if (!this.document) {
      return;
    }

    let timing = this.httpActivity.timing;
    let request = this.httpActivity.request;
    let response = this.httpActivity.response;

    switch (this._state) {
      case this._INIT:
        this._displayRequestHeader();
        this._state = this._DISPLAYED_REQUEST_HEADER;
        

      case this._DISPLAYED_REQUEST_HEADER:
        
        if (!request.bodyDiscarded && request.body) {
          
          if (this._isRequestBodyFormData) {
            this._displayRequestForm();
          }
          else {
            this._displayRequestBody();
          }
          this._state = this._DISPLAYED_REQUEST_BODY;
        }
        

      case this._DISPLAYED_REQUEST_BODY:
        
        
        
        if (!response.header) {
          break
        }
        this._displayResponseHeader();
        this._state = this._DISPLAYED_RESPONSE_HEADER;
        

      case this._DISPLAYED_RESPONSE_HEADER:
        
        if (timing.TRANSACTION_CLOSE && response.isDone) {
          if (response.bodyDiscarded) {
            this._callIsDone();
          }
          else if (this._responseIsImage) {
            this._displayResponseImage();
            this._callIsDone();
          }
          else if (!this._isResponseBodyTextData) {
            this._displayResponseBodyUnknownType();
            this._callIsDone();
          }
          else if (response.body) {
            this._displayResponseBody();
            this._callIsDone();
          }
          else if (this._isResponseCached) {
            let self = this;
            NetworkHelper.loadFromCache(this.httpActivity.url,
                                        this.httpActivity.charset,
                                        function(aContent) {
              
              
              if (aContent) {
                self._displayResponseBody(aContent);
                self._callIsDone();
              }
              
              else {
                self._displayNoResponseBody();
                self._callIsDone();
              }
            });
          }
          else {
            this._displayNoResponseBody();
            this._callIsDone();
          }
          this._state = this._TRANSITION_CLOSED;
        }
        break;
    }
  }
}












function pruneConsoleOutputIfNecessary(aConsoleNode)
{
  let logLimit;
  try {
    let prefBranch = Services.prefs.getBranch("devtools.hud.");
    logLimit = prefBranch.getIntPref("loglimit");
  } catch (e) {
    logLimit = DEFAULT_LOG_LIMIT;
  }

  let messageNodes = aConsoleNode.querySelectorAll(".hud-msg-node");
  for (let i = 0; i < messageNodes.length - logLimit; i++) {
    let messageNode = messageNodes[i];
    let groupNode = messageNode.parentNode;
    if (!groupNode.classList.contains("hud-group")) {
      throw new Error("pruneConsoleOutputIfNecessary: message node not in a " +
                      "HUD group");
    }

    groupNode.removeChild(messageNode);

    
    if (!groupNode.querySelector(".hud-msg-node")) {
      groupNode.parentNode.removeChild(groupNode);
    }
  }
}




function HUD_SERVICE()
{
  
  if (appName() == "FIREFOX") {
    var mixins = new FirefoxApplicationHooks();
  }
  else {
    throw new Error("Unsupported Application");
  }

  this.mixins = mixins;
  this.storage = new ConsoleStorage();
  this.defaultFilterPrefs = this.storage.defaultDisplayPrefs;
  this.defaultGlobalConsolePrefs = this.storage.defaultGlobalConsolePrefs;

  
  
  this.onTabClose = this.onTabClose.bind(this);
  this.onWindowUnload = this.onWindowUnload.bind(this);

  
  var uri = Services.io.newURI(HUD_STYLESHEET_URI, null, null);
  sss.loadAndRegisterSheet(uri, sss.AGENT_SHEET);

  
  this.startHTTPObservation();
};

HUD_SERVICE.prototype =
{
  





  getStr: function HS_getStr(aName)
  {
    return stringBundle.GetStringFromName(aName);
  },

  





  getFormatStr: function HS_getFormatStr(aName, aArray)
  {
    return stringBundle.formatStringFromName(aName, aArray, aArray.length);
  },

  




  get consoleUI() {
    return HeadsUpDisplayUICommands;
  },

  



  activatedContexts: [],

  


  _headsUpDisplays: {},

  


  displayRegistry: {},

  


  windowRegistry: {},

  


  uriRegistry: {},

  



  sequencer: null,

  


  filterPrefs: {},

  



  saveRequestAndResponseBodies: false,

  










  setOnErrorHandler: function HS_setOnErrorHandler(aWindow) {
    var self = this;
    var window = aWindow.wrappedJSObject;
    var console = window.console;
    var origOnerrorFunc = window.onerror;
    window.onerror = function windowOnError(aErrorMsg, aURL, aLineNumber)
    {
      if (aURL && !(aURL in self.uriRegistry)) {
        var lineNum = "";
        if (aLineNumber) {
          lineNum = self.getFormatStr("errLine", [aLineNumber]);
        }
        console.error(aErrorMsg + " @ " + aURL + " " + lineNum);
      }

      if (origOnerrorFunc) {
        origOnerrorFunc(aErrorMsg, aURL, aLineNumber);
      }

      return false;
    };
  },

  






  registerActiveContext: function HS_registerActiveContext(aContextDOMId)
  {
    this.activatedContexts.push(aContextDOMId);
  },

  




  currentContext: function HS_currentContext() {
    return this.mixins.getCurrentContext();
  },

  





  unregisterActiveContext: function HS_deregisterActiveContext(aContextDOMId)
  {
    var domId = aContextDOMId.split("_")[1];
    var idx = this.activatedContexts.indexOf(domId);
    if (idx > -1) {
      this.activatedContexts.splice(idx, 1);
    }
  },

  





  canActivateContext: function HS_canActivateContext(aContextDOMId)
  {
    var domId = aContextDOMId.split("_")[1];
    for (var idx in this.activatedContexts) {
      if (this.activatedContexts[idx] == domId){
        return true;
      }
    }
    return false;
  },

  





  activateHUDForContext: function HS_activateHUDForContext(aContext)
  {
    var window = aContext.linkedBrowser.contentWindow;
    var id = aContext.linkedBrowser.parentNode.parentNode.getAttribute("id");
    this.registerActiveContext(id);
    HUDService.windowInitializer(window);
  },

  





  deactivateHUDForContext: function HS_deactivateHUDForContext(aContext)
  {
    let window = aContext.linkedBrowser.contentWindow;
    let nBox = aContext.ownerDocument.defaultView.
      getNotificationBox(window);
    let hudId = "hud_" + nBox.id;
    let displayNode = nBox.querySelector("#" + hudId);

    if (hudId in this.displayRegistry && displayNode) {
    this.unregisterActiveContext(hudId);
      this.unregisterDisplay(displayNode);
    window.focus();
    }
  },

  







  clearDisplay: function HS_clearDisplay(aHUD)
  {
    if (typeof(aHUD) === "string") {
      aHUD = this.getOutputNodeById(aHUD);
    }

    var outputNode = aHUD.querySelector(".hud-output-node");

    while (outputNode.firstChild) {
      outputNode.removeChild(outputNode.firstChild);
    }

    outputNode.lastTimestamp = 0;
  },

  




  sequenceId: function HS_sequencerId()
  {
    if (!this.sequencer) {
      this.sequencer = this.createSequencer(-1);
    }
    return this.sequencer.next();
  },

  





  getDefaultFilterPrefs: function HS_getDefaultFilterPrefs(aHUDId) {
    return this.filterPrefs[aHUDId];
  },

  





  getFilterPrefs: function HS_getFilterPrefs(aHUDId) {
    return this.filterPrefs[aHUDId];
  },

  






  getFilterState: function HS_getFilterState(aHUDId, aToggleType)
  {
    if (!aHUDId) {
      return false;
    }
    try {
      var bool = this.filterPrefs[aHUDId][aToggleType];
      return bool;
    }
    catch (ex) {
      return false;
    }
  },

  







  setFilterState: function HS_setFilterState(aHUDId, aToggleType, aState)
  {
    this.filterPrefs[aHUDId][aToggleType] = aState;
    this.adjustVisibilityForMessageType(aHUDId, aToggleType, aState);
  },

  











  liftNode: function(aNode, aCallback) {
    let parentNode = aNode.parentNode;
    let siblingNode = aNode.nextSibling;
    parentNode.removeChild(aNode);
    aCallback();
    parentNode.insertBefore(aNode, siblingNode);
  },

  












  adjustVisibilityForMessageType:
  function HS_adjustVisibilityForMessageType(aHUDId, aMessageType, aState)
  {
    let displayNode = this.getOutputNodeById(aHUDId);
    let outputNode = displayNode.querySelector(".hud-output-node");
    let doc = outputNode.ownerDocument;

    this.liftNode(outputNode, function() {
      let xpath = ".//*[contains(@class, 'hud-msg-node') and " +
        "contains(@class, 'hud-" + aMessageType + "')]";
      let result = doc.evaluate(xpath, outputNode, null,
        Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
      for (let i = 0; i < result.snapshotLength; i++) {
        if (aState) {
          result.snapshotItem(i).classList.remove("hud-filtered-by-type");
        } else {
          result.snapshotItem(i).classList.add("hud-filtered-by-type");
        }
      }
    });
  },

  






  buildXPathFunctionForString: function HS_buildXPathFunctionForString(aStr)
  {
    let words = aStr.split(/\s+/), results = [];
    for (let i = 0; i < words.length; i++) {
      let word = words[i];
      if (word === "") {
        continue;
      }

      let result;
      if (word.indexOf('"') === -1) {
        result = '"' + word + '"';
      }
      else if (word.indexOf("'") === -1) {
        result = "'" + word + "'";
      }
      else {
        result = 'concat("' + word.replace(/"/g, "\", '\"', \"") + '")';
      }

      results.push("contains(translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), " + result.toLowerCase() + ")");
    }

    return (results.length === 0) ? "true()" : results.join(" and ");
  },

  









  adjustVisibilityOnSearchStringChange:
  function HS_adjustVisibilityOnSearchStringChange(aHUDId, aSearchString)
  {
    let fn = this.buildXPathFunctionForString(aSearchString);
    let displayNode = this.getOutputNodeById(aHUDId);
    let outputNode = displayNode.querySelector(".hud-output-node");
    let doc = outputNode.ownerDocument;
    this.liftNode(outputNode, function() {
      let xpath = './/*[contains(@class, "hud-msg-node") and ' +
        'not(contains(@class, "hud-filtered-by-string")) and not(' + fn + ')]';
      let result = doc.evaluate(xpath, outputNode, null,
        Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
      for (let i = 0; i < result.snapshotLength; i++) {
        result.snapshotItem(i).classList.add("hud-filtered-by-string");
      }

      xpath = './/*[contains(@class, "hud-msg-node") and contains(@class, ' +
        '"hud-filtered-by-string") and ' + fn + ']';
      result = doc.evaluate(xpath, outputNode, null,
        Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
      for (let i = 0; i < result.snapshotLength; i++) {
        result.snapshotItem(i).classList.remove("hud-filtered-by-string");
      }
    });
  },

  








  adjustVisibilityForNewlyInsertedNode:
  function HS_adjustVisibilityForNewlyInsertedNode(aHUDId, aNewNode) {
    
    let searchString = this.getFilterStringByHUDId(aHUDId);
    let xpath = ".[" + this.buildXPathFunctionForString(searchString) + "]";
    let doc = aNewNode.ownerDocument;
    let result = doc.evaluate(xpath, aNewNode, null,
      Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
    if (result.snapshotLength === 0) {
      
      aNewNode.classList.add("hud-filtered-by-string");
    }

    
    let classes = aNewNode.classList;
    let msgType = null;
    for (let i = 0; i < classes.length; i++) {
      let klass = classes.item(i);
      if (klass !== "hud-msg-node" && klass.indexOf("hud-") === 0) {
        msgType = klass.substring(4);   
        break;
      }
    }
    if (msgType !== null && !this.getFilterState(aHUDId, msgType)) {
      
      aNewNode.classList.add("hud-filtered-by-type");
    }
  },

  



  hudWeakReferences: {},

  






  registerHUDWeakReference:
  function HS_registerHUDWeakReference(aHUDRef, aHUDId)
  {
    this.hudWeakReferences[aHUDId] = aHUDRef;
  },

  





  deleteHeadsUpDisplay: function HS_deleteHeadsUpDisplay(aHUDId)
  {
    delete this.hudWeakReferences[aHUDId].get();
  },

  






  registerDisplay: function HS_registerDisplay(aHUDId, aContentWindow)
  {
    

    if (!aHUDId || !aContentWindow){
      throw new Error(ERRORS.MISSING_ARGS);
    }
    var URISpec = aContentWindow.document.location.href
    this.filterPrefs[aHUDId] = this.defaultFilterPrefs;
    this.displayRegistry[aHUDId] = URISpec;
    this._headsUpDisplays[aHUDId] = { id: aHUDId, };
    this.registerActiveContext(aHUDId);
    
    this.storage.createDisplay(aHUDId);

    var huds = this.uriRegistry[URISpec];
    var foundHUDId = false;

    if (huds) {
      var len = huds.length;
      for (var i = 0; i < len; i++) {
        if (huds[i] == aHUDId) {
          foundHUDId = true;
          break;
        }
      }
      if (!foundHUDId) {
        this.uriRegistry[URISpec].push(aHUDId);
      }
    }
    else {
      this.uriRegistry[URISpec] = [aHUDId];
    }

    var windows = this.windowRegistry[aHUDId];
    if (!windows) {
      this.windowRegistry[aHUDId] = [aContentWindow];
    }
    else {
      windows.push(aContentWindow);
    }
  },

  







  unregisterDisplay: function HS_unregisterDisplay(aHUD)
  {
    
    
    
    HUDService.clearDisplay(aHUD);

    var id, outputNode;
    if (typeof(aHUD) === "string") {
      id = aHUD;
      outputNode = this.mixins.getOutputNodeById(aHUD);
    }
    else {
      id = aHUD.getAttribute("id");
      outputNode = aHUD;
    }

    
    
    var parent = outputNode.parentNode;
    var splitters = parent.querySelectorAll("splitter");
    var len = splitters.length;
    for (var i = 0; i < len; i++) {
      if (splitters[i].getAttribute("class") == "hud-splitter") {
        splitters[i].parentNode.removeChild(splitters[i]);
        break;
      }
    }
    
    parent.removeChild(outputNode);

    this.windowRegistry[id].forEach(function(aContentWindow) {
      if (aContentWindow.wrappedJSObject.console instanceof HUDConsole) {
        delete aContentWindow.wrappedJSObject.console;
      }
    });

    
    delete this._headsUpDisplays[id];
    
    this.deleteHeadsUpDisplay(id);
    
    this.storage.removeDisplay(id);
    
    delete this.windowRegistry[id];

    let displays = this.displays();

    var uri  = this.displayRegistry[id];
    var specHudArr = this.uriRegistry[uri];

    for (var i = 0; i < specHudArr.length; i++) {
      if (specHudArr[i] == id) {
        specHudArr.splice(i, 1);
      }
    }
    delete displays[id];
    delete this.displayRegistry[id];
    delete this.uriRegistry[uri];
  },

  




  shutdown: function HS_shutdown()
  {
    for (var displayId in this._headsUpDisplays) {
      this.unregisterDisplay(displayId);
    }
    
    delete this.storage;
  },

  





  getDisplayByURISpec: function HS_getDisplayByURISpec(aURISpec)
  {
    
    var hudIds = this.uriRegistry[aURISpec];
    if (hudIds.length == 1) {
      
      return this.getHeadsUpDisplay(hudIds[0]);
    }
    else {
      
      
      return this.getHeadsUpDisplay(hudIds[0]);
    }
  },

  






  getHudIdByWindow: function HS_getHudIdByWindow(aContentWindow)
  {
    for (let hudId in this.windowRegistry) {
      if (this.windowRegistry[hudId] &&
          this.windowRegistry[hudId].indexOf(aContentWindow) != -1) {
        return hudId;
      }
    }
    return null;
  },

  





  getHeadsUpDisplay: function HS_getHeadsUpDisplay(aId)
  {
    return this.mixins.getOutputNodeById(aId);
  },

  





  getOutputNodeById: function HS_getOutputNodeById(aId)
  {
    return this.mixins.getOutputNodeById(aId);
  },

  




  displays: function HS_displays() {
    return this._headsUpDisplays;
  },

  





  getHUDIdsForURISpec: function HS_getHUDIdsForURISpec(aURISpec)
  {
    if (this.uriRegistry[aURISpec]) {
      return this.uriRegistry[aURISpec];
    }
    return [];
  },

  



  displaysIndex: function HS_displaysIndex()
  {
    var props = [];
    for (var prop in this._headsUpDisplays) {
      props.push(prop);
    }
    return props;
  },

  





  getFilterStringByHUDId: function HS_getFilterStringbyHUDId(aHUDId) {
    var hud = this.getHeadsUpDisplay(aHUDId);
    var filterStr = hud.querySelectorAll(".hud-filter-box")[0].value;
    return filterStr;
  },

  






  updateFilterText: function HS_updateFiltertext(aTextBoxNode)
  {
    var hudId = aTextBoxNode.getAttribute("hudId");
    this.adjustVisibilityOnSearchStringChange(hudId, aTextBoxNode.value);
  },

  










  logHUDMessage: function HS_logHUDMessage(aMessage,
                                           aConsoleNode,
                                           aMessageNode)
  {
    if (!aMessage) {
      throw new Error(ERRORS.MISSING_ARGS);
    }

    let lastGroupNode = this.appendGroupIfNecessary(aConsoleNode,
                                                    aMessage.timestamp);

    lastGroupNode.appendChild(aMessageNode);
    ConsoleUtils.scrollToVisible(aMessageNode);

    
    this.storage.recordEntry(aMessage.hudId, aMessage);

    pruneConsoleOutputIfNecessary(aConsoleNode);
  },

  








  logConsoleMessage: function HS_logConsoleMessage(aMessage,
                                                   aConsoleNode,
                                                   aMessageNode)
  {
    aConsoleNode.appendChild(aMessageNode);
    ConsoleUtils.scrollToVisible(aMessageNode);

    
    this.storage.recordEntry(aMessage.hudId, aMessage);
  },

  









  logMessage: function HS_logMessage(aMessage, aConsoleNode, aMessageNode)
  {
    if (!aMessage) {
      throw new Error(ERRORS.MISSING_ARGS);
    }

    var hud = this.getHeadsUpDisplay(aMessage.hudId);
    switch (aMessage.origin) {
      case "network":
      case "HUDConsole":
      case "console-listener":
        this.logHUDMessage(aMessage, aConsoleNode, aMessageNode);
        break;
      default:
        
        break;
    }
  },

  





  getConsoleOutputNode: function HS_getConsoleOutputNode(aId)
  {
    let displayNode = this.getHeadsUpDisplay(aId);
    return displayNode.querySelector(".hud-output-node");
  },

  






  logWarningAboutReplacedAPI:
  function HS_logWarningAboutReplacedAPI(aHUDId)
  {
    let domId = "hud-log-node-" + this.sequenceId();
    let outputNode = this.getConsoleOutputNode(aHUDId);

    let msgFormat = {
      logLevel: "error",
      activityObject: {},
      hudId: aHUDId,
      origin: "console-listener",
      domId: domId,
      message: this.getStr("ConsoleAPIDisabled"),
    };

    let messageObject =
    this.messageFactory(msgFormat, "error", outputNode, msgFormat.activityObject);
    this.logMessage(messageObject.messageObject, outputNode, messageObject.messageNode);
  },

  




  reportConsoleServiceMessage:
  function HS_reportConsoleServiceMessage(aConsoleMessage)
  {
    this.logActivity("console-listener", null, aConsoleMessage);
  },

  




  reportConsoleServiceContentScriptError:
  function HS_reportConsoleServiceContentScriptError(aScriptError)
  {
    try {
      var uri = Services.io.newURI(aScriptError.sourceName, null, null);
    }
    catch(ex) {
      var uri = { spec: "" };
    }
    this.logActivity("console-listener", uri, aScriptError);
  },

  






  generateConsoleMessage:
  function HS_generateConsoleMessage(aMessage, flag)
  {
    let message = scriptError; 
    message.init(aMessage.message, null, null, 0, 0, flag,
                 "HUDConsole");
    return message;
  },

  




  registerApplicationHooks:
  function HS_registerApplications(aAppName, aHooksObject)
  {
    switch(aAppName) {
      case "FIREFOX":
        this.applicationHooks = aHooksObject;
        return;
      default:
        throw new Error("MOZ APPLICATION UNSUPPORTED");
    }
  },

  




  applicationHooks: null,

  getChromeWindowFromContentWindow:
  function HS_getChromeWindowFromContentWindow(aContentWindow)
  {
    if (!aContentWindow) {
      throw new Error("Cannot get contentWindow via nsILoadContext");
    }
    var win = aContentWindow.QueryInterface(Ci.nsIDOMWindow)
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem)
      .rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindow)
      .QueryInterface(Ci.nsIDOMChromeWindow);
    return win;
  },

  


  openRequests: {},

  



  lastFinishedRequestCallback: null,

  









  openNetworkPanel: function (aNode, aHttpActivity) {
    let doc = aNode.ownerDocument;
    let parent = doc.getElementById("mainPopupSet");
    let netPanel = new NetworkPanel(parent, aHttpActivity);
    netPanel.linkNode = aNode;

    let panel = netPanel.panel;
    panel.openPopup(aNode, "after_pointer", 0, 0, false, false);
    panel.sizeTo(350, 400);
    aHttpActivity.panels.push(Cu.getWeakReference(netPanel));
    return netPanel;
  },

  




  startHTTPObservation: function HS_httpObserverFactory()
  {
    
    var self = this;
    var httpObserver = {
      observeActivity :
      function (aChannel, aActivityType, aActivitySubtype,
                aTimestamp, aExtraSizeData, aExtraStringData)
      {
        if (aActivityType ==
              activityDistributor.ACTIVITY_TYPE_HTTP_TRANSACTION ||
            aActivityType ==
              activityDistributor.ACTIVITY_TYPE_SOCKET_TRANSPORT) {

          aChannel = aChannel.QueryInterface(Ci.nsIHttpChannel);

          let transCodes = this.httpTransactionCodes;
          let hudId;

          if (aActivitySubtype ==
              activityDistributor.ACTIVITY_SUBTYPE_REQUEST_HEADER ) {
            
            let win = NetworkHelper.getWindowForRequest(aChannel);
            if (!win) {
              return;
            }

            
            hudId = self.getHudIdByWindow(win);
            if (!hudId) {
              return;
            }

            
            
            let httpActivity = {
              id: self.sequenceId(),
              hudId: hudId,
              url: aChannel.URI.spec,
              method: aChannel.requestMethod,
              channel: aChannel,
              charset: win.document.characterSet,

              panels: [],
              request: {
                header: { }
              },
              response: {
                header: null
              },
              timing: {
                "REQUEST_HEADER": aTimestamp
              }
            };

            
            let loggedNode =
              self.logActivity("network", aChannel.URI, httpActivity);

            
            
            if (!loggedNode) {
              return;
            }

            
            let newListener = new ResponseListener(httpActivity);
            aChannel.QueryInterface(Ci.nsITraceableChannel);
            newListener.originalListener = aChannel.setNewListener(newListener);
            httpActivity.response.listener = newListener;

            
            aChannel.visitRequestHeaders({
              visitHeader: function(aName, aValue) {
                httpActivity.request.header[aName] = aValue;
              }
            });

            
            httpActivity.messageObject = loggedNode;
            self.openRequests[httpActivity.id] = httpActivity;

            
            let linkNode = loggedNode.messageNode;
            linkNode.setAttribute("aria-haspopup", "true");
            linkNode.addEventListener("mousedown", function(aEvent) {
              this._startX = aEvent.clientX;
              this._startY = aEvent.clientY;
            }, false);

            linkNode.addEventListener("click", function(aEvent) {
              if (aEvent.detail != 1 || aEvent.button != 0 ||
                  (this._startX != aEvent.clientX &&
                   this._startY != aEvent.clientY)) {
                return;
              }

              if (!this._panelOpen) {
                self.openNetworkPanel(this, httpActivity);
                this._panelOpen = true;
              }
            }, false);
          }
          else {
            
            
            let httpActivity = null;
            for each (var item in self.openRequests) {
              if (item.channel !== aChannel) {
                continue;
              }
              httpActivity = item;
              break;
            }

            if (!httpActivity) {
              return;
            }

            let msgObject, updatePanel = false;
            let data, textNode;
            
            httpActivity.timing[transCodes[aActivitySubtype]] = aTimestamp;

            switch (aActivitySubtype) {
              case activityDistributor.ACTIVITY_SUBTYPE_REQUEST_BODY_SENT:
                if (!self.saveRequestAndResponseBodies) {
                  httpActivity.request.bodyDiscarded = true;
                  break;
                }

                let gBrowser = HUDService.currentContext().gBrowser;

                let sentBody = NetworkHelper.readPostTextFromRequest(
                                aChannel, gBrowser);
                if (!sentBody) {
                  
                  
                  
                  
                  
                  
                  
                  if (httpActivity.url == gBrowser.contentWindow.location.href) {
                    sentBody = NetworkHelper.readPostTextFromPage(gBrowser);
                  }
                  if (!sentBody) {
                    sentBody = "";
                  }
                }
                httpActivity.request.body = sentBody;
                break;

              case activityDistributor.ACTIVITY_SUBTYPE_RESPONSE_HEADER:
                msgObject = httpActivity.messageObject;

                
                
                
                
                
                
                
                
                httpActivity.response.status =
                  aExtraStringData.split(/\r\n|\n|\r/)[0];

                
                
                textNode = msgObject.messageNode.firstChild;
                textNode.parentNode.removeChild(textNode);

                data = [ httpActivity.url,
                         httpActivity.response.status ];

                msgObject.messageNode.appendChild(
                  msgObject.textFactory(
                    msgObject.prefix +
                    self.getFormatStr("networkUrlWithStatus", data) + "\n"));

                break;

              case activityDistributor.ACTIVITY_SUBTYPE_TRANSACTION_CLOSE:
                msgObject = httpActivity.messageObject;


                let timing = httpActivity.timing;
                let requestDuration =
                  Math.round((timing.RESPONSE_COMPLETE -
                                timing.REQUEST_HEADER) / 1000);

                
                
                textNode = msgObject.messageNode.firstChild;
                textNode.parentNode.removeChild(textNode);

                data = [ httpActivity.url,
                         httpActivity.response.status,
                         requestDuration ];

                msgObject.messageNode.appendChild(
                  msgObject.textFactory(
                    msgObject.prefix +
                    self.getFormatStr("networkUrlWithStatusAndDuration",
                                      data) + "\n"));

                delete self.openRequests[item.id];
                updatePanel = true;
                break;
            }

            if (updatePanel) {
              httpActivity.panels.forEach(function(weakRef) {
                let panel = weakRef.get();
                if (panel) {
                  panel.update();
                }
              });
            }
          }
        }
      },

      httpTransactionCodes: {
        0x5001: "REQUEST_HEADER",
        0x5002: "REQUEST_BODY_SENT",
        0x5003: "RESPONSE_START",
        0x5004: "RESPONSE_HEADER",
        0x5005: "RESPONSE_COMPLETE",
        0x5006: "TRANSACTION_CLOSE",

        0x804b0003: "STATUS_RESOLVING",
        0x804b0007: "STATUS_CONNECTING_TO",
        0x804b0004: "STATUS_CONNECTED_TO",
        0x804b0005: "STATUS_SENDING_TO",
        0x804b000a: "STATUS_WAITING_FOR",
        0x804b0006: "STATUS_RECEIVING_FROM"
      }
    };

    activityDistributor.addObserver(httpObserver);
  },

  






  logNetActivity: function HS_logNetActivity(aType, aURI, aActivityObject)
  {
    var outputNode, hudId;
    try {
      hudId = aActivityObject.hudId;
      outputNode = this.getHeadsUpDisplay(hudId).
                                  querySelector(".hud-output-node");

      
      
      var domId = "hud-log-node-" + this.sequenceId();

      var message = { logLevel: aType,
                      activityObj: aActivityObject,
                      hudId: hudId,
                      origin: "network",
                      domId: domId,
                    };
      var msgType = this.getStr("typeNetwork");
      var msg = msgType + " " +
        aActivityObject.method +
        " " +
        aActivityObject.url;
      message.message = msg;

      var messageObject =
        this.messageFactory(message, aType, outputNode, aActivityObject);

      var timestampedMessage = messageObject.timestampedMessage;
      var urlIdx = timestampedMessage.indexOf(aActivityObject.url);
      messageObject.prefix = timestampedMessage.substring(0, urlIdx);

      messageObject.messageNode.classList.add("hud-clickable");
      messageObject.messageNode.setAttribute("crop", "end");

      this.logMessage(messageObject.messageObject, outputNode, messageObject.messageNode);
      return messageObject;
    }
    catch (ex) {
      Cu.reportError(ex);
    }
  },

  






  logConsoleActivity: function HS_logConsoleActivity(aURI, aActivityObject)
  {
    var displayNode, outputNode, hudId;
    try {
        var hudIds = this.uriRegistry[aURI.spec];
        hudId = hudIds[0];
    }
    catch (ex) {
      
      
      
      if (!displayNode) {
        return;
      }
    }

    var _msgLogLevel = this.scriptMsgLogLevel[aActivityObject.flags];
    var msgLogLevel = this.getStr(_msgLogLevel);

    var logLevel = "warn";

    if (aActivityObject.flags in this.scriptErrorFlags) {
      logLevel = this.scriptErrorFlags[aActivityObject.flags];
    }

    
    
    var message = {
      activity: aActivityObject,
      origin: "console-listener",
      hudId: hudId,
    };

    var lineColSubs = [aActivityObject.lineNumber,
                       aActivityObject.columnNumber];
    var lineCol = this.getFormatStr("errLineCol", lineColSubs);

    var errFileSubs = [aActivityObject.sourceName];
    var errFile = this.getFormatStr("errFile", errFileSubs);

    var msgCategory = this.getStr("msgCategory");

    message.logLevel = logLevel;
    message.level = logLevel;

    message.message = msgLogLevel + " " +
                      aActivityObject.errorMessage + " " +
                      errFile + " " +
                      lineCol + " " +
                      msgCategory + " " + aActivityObject.category;

    displayNode = this.getHeadsUpDisplay(hudId);
    outputNode = displayNode.querySelectorAll(".hud-output-node")[0];

    var messageObject =
    this.messageFactory(message, message.level, outputNode, aActivityObject);

    this.logMessage(messageObject.messageObject, outputNode, messageObject.messageNode);
  },

  










  logActivity: function HS_logActivity(aType, aURI, aActivityObject)
  {
    var displayNode, outputNode, hudId;

    if (aType == "network") {
      return this.logNetActivity(aType, aURI, aActivityObject);
    }
    else if (aType == "console-listener") {
      this.logConsoleActivity(aURI, aActivityObject);
    }
  },

  











  appendGroupIfNecessary:
  function HS_appendGroupIfNecessary(aConsoleNode, aTimestamp)
  {
    let hudBox = aConsoleNode;
    while (hudBox != null && hudBox.getAttribute("class") !== "hud-box") {
      hudBox = hudBox.parentNode;
    }

    let lastTimestamp = hudBox.lastTimestamp;
    let delta = aTimestamp - lastTimestamp;
    hudBox.lastTimestamp = aTimestamp;
    if (delta < NEW_GROUP_DELAY) {
      
      
      let lastGroupNode = aConsoleNode.querySelector(".hud-group:last-child");
      if (lastGroupNode != null) {
        return lastGroupNode;
      }
    }

    let chromeDocument = aConsoleNode.ownerDocument;
    let groupNode = chromeDocument.createElement("vbox");
    groupNode.setAttribute("class", "hud-group");

    aConsoleNode.appendChild(groupNode);
    return groupNode;
  },

  






  getActivityOutputNode: function HS_getActivityOutputNode(aURI)
  {
    
    var display = this.getDisplayByURISpec(aURI.spec);
    if (display) {
      return this.getOutputNodeById(display);
    }
    else {
      throw new Error("Cannot get outputNode by hudId");
    }
  },

  








  messageFactory:
  function messageFactory(aMessage, aLevel, aOutputNode, aActivityObject)
  {
    
    return new LogMessage(aMessage, aLevel, aOutputNode,  aActivityObject);
  },

  









  initializeJSTerm: function HS_initializeJSTerm(aContext, aParentNode, aConsole)
  {
    
    var context = Cu.getWeakReference(aContext);

    
    var firefoxMixin = new JSTermFirefoxMixin(context, aParentNode);
    var jsTerm = new JSTerm(context, aParentNode, firefoxMixin, aConsole);

    
    
  },

  





  getContentWindowFromHUDId: function HS_getContentWindowFromHUDId(aHUDId)
  {
    var hud = this.getHeadsUpDisplay(aHUDId);
    var nodes = hud.parentNode.childNodes;

    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i];

      if (node.localName == "stack" &&
          node.firstChild &&
          node.firstChild.contentWindow) {
        return node.firstChild.contentWindow;
      }
    }
    throw new Error("HS_getContentWindowFromHUD: Cannot get contentWindow");
  },

  





  createSequencer: function HS_createSequencer(aInt)
  {
    function sequencer(aInt)
    {
      while(1) {
        aInt++;
        yield aInt;
      }
    }
    return sequencer(aInt);
  },

  scriptErrorFlags: {
    0: "error",
    1: "warn",
    2: "exception",
    4: "strict"
  },

  


  scriptMsgLogLevel: {
    0: "typeError",
    1: "typeWarning",
    2: "typeException",
    4: "typeStrict",
  },

  






  closeConsoleOnTab: function HS_closeConsoleOnTab(aTab)
  {
    let xulDocument = aTab.ownerDocument;
    let xulWindow = xulDocument.defaultView;
    let gBrowser = xulWindow.gBrowser;
    let linkedBrowser = aTab.linkedBrowser;
    let notificationBox = gBrowser.getNotificationBox(linkedBrowser);
    let hudId = "hud_" + notificationBox.getAttribute("id");
    let outputNode = xulDocument.getElementById(hudId);
    if (outputNode != null) {
      this.unregisterDisplay(outputNode);
    }
  },

  





  onTabClose: function HS_onTabClose(aEvent)
  {
    this.closeConsoleOnTab(aEvent.target);
  },

  







  onWindowUnload: function HS_onWindowUnload(aEvent)
  {
    let gBrowser = aEvent.target.defaultView.gBrowser;
    let tabContainer = gBrowser.tabContainer;

    let tab = tabContainer.firstChild;
    while (tab != null) {
      this.closeConsoleOnTab(tab);
      tab = tab.nextSibling;
    }
  },

  





  windowInitializer: function HS_WindowInitalizer(aContentWindow)
  {
    var xulWindow = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
                      .QueryInterface(Ci.nsIDocShell)
                      .chromeEventHandler.ownerDocument.defaultView;

    let xulWindow = XPCNativeWrapper.unwrap(xulWindow);

    let docElem = xulWindow.document.documentElement;
    if (!docElem || docElem.getAttribute("windowtype") != "navigator:browser" ||
        !xulWindow.gBrowser) {
      
      
      return;
    }

    if (aContentWindow.document.location.href == "about:blank" &&
        HUDWindowObserver.initialConsoleCreated == false) {
      
      
      return;
    }

    xulWindow.addEventListener("unload", this.onWindowUnload, false);

    let gBrowser = xulWindow.gBrowser;


    var container = gBrowser.tabContainer;
    container.addEventListener("TabClose", this.onTabClose, false);

    if (gBrowser && !HUDWindowObserver.initialConsoleCreated) {
      HUDWindowObserver.initialConsoleCreated = true;
    }

    let _browser = gBrowser.
      getBrowserForDocument(aContentWindow.top.document.wrappedJSObject);
    let nBox = gBrowser.getNotificationBox(_browser);
    let nBoxId = nBox.getAttribute("id");
    let hudId = "hud_" + nBoxId;

    if (!this.canActivateContext(hudId)) {
      return;
    }

    this.registerDisplay(hudId, aContentWindow);

    let hudNode;
    let childNodes = nBox.childNodes;

    for (let i = 0; i < childNodes.length; i++) {
      let id = childNodes[i].getAttribute("id");
      
      if (id.split("_")[0] == "hud") {
        hudNode = childNodes[i];
        break;
      }
    }

    let hud;
    
    if (!hudNode) {
      
      let config = { parentNode: nBox,
                     contentWindow: aContentWindow,
                   };

      hud = new HeadsUpDisplay(config);

      let hudWeakRef = Cu.getWeakReference(hud);
      HUDService.registerHUDWeakReference(hudWeakRef, hudId);
    }
    else {
      hud = this.hudWeakReferences[hudId].get();
      hud.reattachConsole(aContentWindow.top);
    }

    
    
    if (aContentWindow.wrappedJSObject.console) {
      this.logWarningAboutReplacedAPI(hudId);
    }
    else {
      aContentWindow.wrappedJSObject.console = hud.console;
    }

    
    this.setOnErrorHandler(aContentWindow);

    
    this.createController(xulWindow);
  },

  






  createController: function HUD_createController(aWindow)
  {
    if (aWindow.commandController == null) {
      aWindow.commandController = new CommandController(aWindow);
      aWindow.controllers.insertControllerAt(0, aWindow.commandController);
    }
  }
};










function HeadsUpDisplay(aConfig)
{
  
  
  
  
  
  
  
  
  

  this.HUDBox = null;

  if (aConfig.parentNode) {
    
    
    
    
    
    this.parentNode = aConfig.parentNode;
    this.notificationBox = aConfig.parentNode;
    this.chromeDocument = aConfig.parentNode.ownerDocument;
    this.contentWindow = aConfig.contentWindow;
    this.uriSpec = aConfig.contentWindow.location.href;
    this.hudId = "hud_" + aConfig.parentNode.getAttribute("id");
  }
  else {
    
    
    
    let windowEnum = Services.wm.getEnumerator("navigator:browser");
    let parentNode;
    let contentDocument;
    let contentWindow;
    let chromeDocument;

    
    

    while (windowEnum.hasMoreElements()) {
      let window = windowEnum.getNext();
      try {
        let gBrowser = window.gBrowser;
        let _browsers = gBrowser.browsers;
        let browserLen = _browsers.length;

        for (var i = 0; i < browserLen; i++) {
          var _notificationBox = gBrowser.getNotificationBox(_browsers[i]);
          this.notificationBox = _notificationBox;

          if (_notificationBox.getAttribute("id") == aConfig.parentNodeId) {
            this.parentNodeId = _notificationBox.getAttribute("id");
            this.hudId = "hud_" + this.parentNodeId;

            parentNode = _notificationBox;

            this.contentDocument =
              _notificationBox.childNodes[0].contentDocument;
            this.contentWindow =
              _notificationBox.childNodes[0].contentWindow;
            this.uriSpec = aConfig.contentWindow.location.href;

            this.chromeDocument =
              _notificationBox.ownerDocument;

            break;
          }
        }
      }
      catch (ex) {
        Cu.reportError(ex);
      }

      if (parentNode) {
        break;
      }
    }
    if (!parentNode) {
      throw new Error(this.ERRORS.PARENTNODE_NOT_FOUND);
    }
    this.parentNode = parentNode;
  }

  
  this.textFactory = NodeFactory("text", "xul", this.chromeDocument);

  this.chromeWindow = HUDService.getChromeWindowFromContentWindow(this.contentWindow);

  
  let hudBox = this.createHUD();

  let splitter = this.chromeDocument.createElement("splitter");
  splitter.setAttribute("class", "hud-splitter");

  this.notificationBox.insertBefore(splitter,
                                    this.notificationBox.childNodes[1]);

  this.HUDBox.lastTimestamp = 0;

  
  this._console = this.createConsole();

  
  try {
    this.createConsoleInput(this.contentWindow, this.consoleWrap, this.outputNode);
    this.HUDBox.querySelectorAll(".jsterm-input-node")[0].focus();
  }
  catch (ex) {
    Cu.reportError(ex);
  }
}

HeadsUpDisplay.prototype = {
  





  getStr: function HUD_getStr(aName)
  {
    return stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function HUD_getFormatStr(aName, aArray)
  {
    return stringBundle.formatStringFromName(aName, aArray, aArray.length);
  },

  



  jsterm: null,

  





  createConsoleInput:
  function HUD_createConsoleInput(aWindow, aParentNode, aExistingConsole)
  {
    var context = Cu.getWeakReference(aWindow);

    if (appName() == "FIREFOX") {
      let outputCSSClassOverride = "hud-msg-node";
      let mixin = new JSTermFirefoxMixin(context, aParentNode, aExistingConsole, outputCSSClassOverride);
      this.jsterm = new JSTerm(context, aParentNode, mixin, this.console);
    }
    else {
      throw new Error("Unsupported Gecko Application");
    }
  },

  





  reattachConsole: function HUD_reattachConsole(aContentWindow)
  {
    this.contentWindow = aContentWindow;
    this.contentDocument = this.contentWindow.document;
    this.uriSpec = this.contentWindow.location.href;

    if (!this._console) {
      this._console = this.createConsole();
    }

    if (!this.jsterm) {
      this.createConsoleInput(this.contentWindow, this.consoleWrap, this.outputNode);
    }
    else {
      this.jsterm.context = Cu.getWeakReference(this.contentWindow);
      this.jsterm.console = this.console;
      this.jsterm.createSandbox();
    }
  },

  





  makeXULNode:
  function HUD_makeXULNode(aTag)
  {
    return this.chromeDocument.createElement(aTag);
  },

  




  makeHUDNodes: function HUD_makeHUDNodes()
  {
    let self = this;
    this.HUDBox = this.makeXULNode("vbox");
    this.HUDBox.setAttribute("id", this.hudId);
    this.HUDBox.setAttribute("class", "hud-box");

    var height = Math.ceil((this.contentWindow.innerHeight * .33)) + "px";
    var style = "height: " + height + ";";
    this.HUDBox.setAttribute("style", style);

    let outerWrap = this.makeXULNode("vbox");
    outerWrap.setAttribute("class", "hud-outer-wrapper");
    outerWrap.setAttribute("flex", "1");

    let consoleCommandSet = this.makeXULNode("commandset");
    outerWrap.appendChild(consoleCommandSet);

    let consoleWrap = this.makeXULNode("vbox");
    this.consoleWrap = consoleWrap;
    consoleWrap.setAttribute("class", "hud-console-wrapper");
    consoleWrap.setAttribute("flex", "1");

    this.outputNode = this.makeXULNode("scrollbox");
    this.outputNode.setAttribute("class", "hud-output-node");
    this.outputNode.setAttribute("flex", "1");
    this.outputNode.setAttribute("orient", "vertical");
    this.outputNode.setAttribute("context", this.hudId + "-output-contextmenu");

    this.outputNode.addEventListener("DOMNodeInserted", function(ev) {
      
      
      
      
      let node = ev.target;
      if (node.nodeType === node.ELEMENT_NODE &&
          node.classList.contains("hud-msg-node")) {
        HUDService.adjustVisibilityForNewlyInsertedNode(self.hudId, ev.target);
      }
    }, false);

    this.filterSpacer = this.makeXULNode("spacer");
    this.filterSpacer.setAttribute("flex", "1");

    this.filterBox = this.makeXULNode("textbox");
    this.filterBox.setAttribute("class", "compact hud-filter-box");
    this.filterBox.setAttribute("hudId", this.hudId);
    this.filterBox.setAttribute("placeholder", this.getStr("stringFilter"));
    this.filterBox.setAttribute("type", "search");

    this.setFilterTextBoxEvents();

    this.createConsoleMenu(this.consoleWrap);

    this.filterPrefs = HUDService.getDefaultFilterPrefs(this.hudId);

    let consoleFilterToolbar = this.makeFilterToolbar();
    consoleFilterToolbar.setAttribute("id", "viewGroup");
    this.consoleFilterToolbar = consoleFilterToolbar;
    consoleWrap.appendChild(consoleFilterToolbar);

    consoleWrap.appendChild(this.outputNode);

    outerWrap.appendChild(consoleWrap);

    this.HUDBox.lastTimestamp = 0;

    this.jsTermParentNode = outerWrap;
    this.HUDBox.appendChild(outerWrap);
    return this.HUDBox;
  },


  




  setFilterTextBoxEvents: function HUD_setFilterTextBoxEvents()
  {
    var filterBox = this.filterBox;
    function onChange()
    {
      
      

      if (this.timer == null) {
        let timerClass = Cc["@mozilla.org/timer;1"];
        this.timer = timerClass.createInstance(Ci.nsITimer);
      } else {
        this.timer.cancel();
      }

      let timerEvent = {
        notify: function setFilterTextBoxEvents_timerEvent_notify() {
          HUDService.updateFilterText(filterBox);
        }
      };

      this.timer.initWithCallback(timerEvent, SEARCH_DELAY,
        Ci.nsITimer.TYPE_ONE_SHOT);
    }

    filterBox.addEventListener("command", onChange, false);
    filterBox.addEventListener("input", onChange, false);
  },

  




  makeFilterToolbar: function HUD_makeFilterToolbar()
  {
    let buttons = ["Network", "CSSParser", "Exception", "Error",
                   "Info", "Warn", "Log",];

    const pageButtons = [
      { prefKey: "network", name: "PageNet" },
      { prefKey: "cssparser", name: "PageCSS" },
      { prefKey: "exception", name: "PageJS" }
    ];
    const consoleButtons = [
      { prefKey: "error", name: "ConsoleErrors" },
      { prefKey: "warn", name: "ConsoleWarnings" },
      { prefKey: "info", name: "ConsoleInfo" },
      { prefKey: "log", name: "ConsoleLog" }
    ];

    let toolbar = this.makeXULNode("toolbar");
    toolbar.setAttribute("class", "hud-console-filter-toolbar");
    toolbar.setAttribute("mode", "text");

    let pageCategoryTitle = this.getStr("categoryPage");
    this.addButtonCategory(toolbar, pageCategoryTitle, pageButtons);

    let separator = this.makeXULNode("separator");
    separator.setAttribute("orient", "vertical");
    toolbar.appendChild(separator);

    let consoleCategoryTitle = this.getStr("categoryConsole");
    this.addButtonCategory(toolbar, consoleCategoryTitle, consoleButtons);

    toolbar.appendChild(this.filterSpacer);
    toolbar.appendChild(this.filterBox);
    return toolbar;
  },

  







  createConsoleMenu: function HUD_createConsoleMenu(aConsoleWrapper) {
    let menuPopup = this.makeXULNode("menupopup");
    let id = this.hudId + "-output-contextmenu";
    menuPopup.setAttribute("id", id);

    let saveBodiesItem = this.makeXULNode("menuitem");
    saveBodiesItem.setAttribute("label", this.getStr("saveBodies.label"));
    saveBodiesItem.setAttribute("accesskey",
                                 this.getStr("saveBodies.accesskey"));
    saveBodiesItem.setAttribute("type", "checkbox");
    saveBodiesItem.setAttribute("buttonType", "saveBodies");
    saveBodiesItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    menuPopup.appendChild(saveBodiesItem);

    menuPopup.appendChild(this.makeXULNode("menuseparator"));

    let copyItem = this.makeXULNode("menuitem");
    copyItem.setAttribute("label", this.getStr("copyCmd.label"));
    copyItem.setAttribute("accesskey", this.getStr("copyCmd.accesskey"));
    copyItem.setAttribute("key", "key_copy");
    copyItem.setAttribute("command", "cmd_copy");
    menuPopup.appendChild(copyItem);

    let selectAllItem = this.makeXULNode("menuitem");
    selectAllItem.setAttribute("label", this.getStr("selectAllCmd.label"));
    selectAllItem.setAttribute("accesskey",
                               this.getStr("selectAllCmd.accesskey"));
    selectAllItem.setAttribute("hudId", this.hudId);
    selectAllItem.setAttribute("buttonType", "selectAll");
    selectAllItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    menuPopup.appendChild(selectAllItem);

    menuPopup.appendChild(this.makeXULNode("menuseparator"));

    let clearItem = this.makeXULNode("menuitem");
    clearItem.setAttribute("label", this.getStr("clearConsoleCmd.label"));
    clearItem.setAttribute("accesskey",
                           this.getStr("clearConsoleCmd.accesskey"));
    clearItem.setAttribute("hudId", this.hudId);
    clearItem.setAttribute("buttonType", "clear");
    clearItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    menuPopup.appendChild(clearItem);

    aConsoleWrapper.appendChild(menuPopup);
    aConsoleWrapper.setAttribute("context", id);
  },

  makeButton: function HUD_makeButton(aName, aPrefKey, aType)
  {
    var self = this;
    let prefKey = aPrefKey;

    let btn;
    if (aType == "checkbox") {
      btn = this.makeXULNode("checkbox");
      btn.setAttribute("type", aType);
    } else {
      btn = this.makeXULNode("toolbarbutton");
    }

    btn.setAttribute("hudId", this.hudId);
    btn.setAttribute("buttonType", prefKey);
    btn.setAttribute("class", "hud-filter-btn");
    let key = "btn" + aName;
    btn.setAttribute("label", this.getStr(key));
    key = "tip" + aName;
    btn.setAttribute("tooltip", this.getStr(key));

    if (aType == "checkbox") {
      btn.setAttribute("checked", this.filterPrefs[prefKey]);
      function toggle(btn) {
        self.consoleFilterCommands.toggle(btn);
      };

      btn.setAttribute("oncommand", "HUDConsoleUI.toggleFilter(this);");
    }
    else {
      var command = "HUDConsoleUI.command(this)";
      btn.setAttribute("oncommand", command);
    }
    return btn;
  },

  











  addButtonCategory: function(aToolbar, aTitle, aButtons) {
    let lbl = this.makeXULNode("label");
    lbl.setAttribute("class", "hud-filter-cat");
    lbl.setAttribute("value", aTitle);
    aToolbar.appendChild(lbl);

    for (let i = 0; i < aButtons.length; i++) {
      let btn = aButtons[i];
      aToolbar.appendChild(this.makeButton(btn.name, btn.prefKey, "checkbox"));
    }
  },

  createHUD: function HUD_createHUD()
  {
    let self = this;
    if (this.HUDBox) {
      return this.HUDBox;
    }
    else  {
      this.makeHUDNodes();

      let nodes = this.notificationBox.insertBefore(this.HUDBox,
        this.notificationBox.childNodes[0]);

      return this.HUDBox;
    }
  },

  get console() {
    if (!this._console) {
      this._console = this.createConsole();
    }

    return this._console;
  },

  getLogCount: function HUD_getLogCount()
  {
    return this.outputNode.childNodes.length;
  },

  getLogNodes: function HUD_getLogNodes()
  {
    return this.outputNode.childNodes;
  },

  






  createConsole: function HUD_createConsole()
  {
    return new HUDConsole(this);
  },

  ERRORS: {
    HUD_BOX_DOES_NOT_EXIST: "Heads Up Display does not exist",
    TAB_ID_REQUIRED: "Tab DOM ID is required",
    PARENTNODE_NOT_FOUND: "parentNode element not found"
  }
};












function HUDConsole(aHeadsUpDisplay)
{
  let hud = aHeadsUpDisplay;
  let hudId = hud.hudId;
  let outputNode = hud.outputNode;
  let chromeDocument = hud.chromeDocument;

  let sendToHUDService = function console_send(aLevel, aArguments)
  {
    let ts = ConsoleUtils.timestamp();
    let messageNode = hud.makeXULNode("label");

    let klass = "hud-msg-node hud-" + aLevel;

    messageNode.setAttribute("class", klass);

    let argumentArray = [];
    for (var i = 0; i < aArguments.length; i++) {
      argumentArray.push(aArguments[i]);
    }

    let message = argumentArray.join(' ');
    let timestampedMessage = ConsoleUtils.timestampString(ts) + ": " +
      message + "\n";

    messageNode.appendChild(chromeDocument.createTextNode(timestampedMessage));

    
    let messageObject = {
      logLevel: aLevel,
      hudId: hud.hudId,
      message: message,
      timestamp: ts,
      origin: "HUDConsole",
    };

    HUDService.logMessage(messageObject, hud.outputNode, messageNode);
  }

  
  
  this.log = function console_log()
  {
    sendToHUDService("log", arguments);
  },

  this.info = function console_info()
  {
    sendToHUDService("info", arguments);
  },

  this.warn = function console_warn()
  {
    sendToHUDService("warn", arguments);
  },

  this.error = function console_error()
  {
    sendToHUDService("error", arguments);
  },

  this.exception = function console_exception()
  {
    sendToHUDService("exception", arguments);
  }
};









function NodeFactory(aFactoryType, ignored, aDocument)
{
  
  if (aFactoryType == "text") {
    return function factory(aText)
    {
      return aDocument.createTextNode(aText);
    }
  }
  else if (aFactoryType == "xul") {
    return function factory(aTag)
    {
      return aDocument.createElement(aTag);
    }
  }
  else {
    throw new Error('NodeFactory: Unknown factory type: ' + aFactoryType);
  }
}





const STATE_NORMAL = 0;
const STATE_QUOTE = 2;
const STATE_DQUOTE = 3;

const OPEN_BODY = '{[('.split('');
const CLOSE_BODY = '}])'.split('');
const OPEN_CLOSE_BODY = {
  '{': '}',
  '[': ']',
  '(': ')'
};




















function findCompletionBeginning(aStr)
{
  let bodyStack = [];

  let state = STATE_NORMAL;
  let start = 0;
  let c;
  for (let i = 0; i < aStr.length; i++) {
    c = aStr[i];

    switch (state) {
      
      case STATE_NORMAL:
        if (c == '"') {
          state = STATE_DQUOTE;
        }
        else if (c == '\'') {
          state = STATE_QUOTE;
        }
        else if (c == ';') {
          start = i + 1;
        }
        else if (c == ' ') {
          start = i + 1;
        }
        else if (OPEN_BODY.indexOf(c) != -1) {
          bodyStack.push({
            token: c,
            start: start
          });
          start = i + 1;
        }
        else if (CLOSE_BODY.indexOf(c) != -1) {
          var last = bodyStack.pop();
          if (OPEN_CLOSE_BODY[last.token] != c) {
            return {
              err: "syntax error"
            };
          }
          if (c == '}') {
            start = i + 1;
          }
          else {
            start = last.start;
          }
        }
        break;

      
      case STATE_DQUOTE:
        if (c == '\\') {
          i ++;
        }
        else if (c == '\n') {
          return {
            err: "unterminated string literal"
          };
        }
        else if (c == '"') {
          state = STATE_NORMAL;
        }
        break;

      
      case STATE_QUOTE:
        if (c == '\\') {
          i ++;
        }
        else if (c == '\n') {
          return {
            err: "unterminated string literal"
          };
          return;
        }
        else if (c == '\'') {
          state = STATE_NORMAL;
        }
        break;
    }
  }

  return {
    state: state,
    startPos: start
  };
}




















function JSPropertyProvider(aScope, aInputValue)
{
  let obj = XPCNativeWrapper.unwrap(aScope);

  
  
  let beginning = findCompletionBeginning(aInputValue);

  
  if (beginning.err) {
    return null;
  }

  
  
  if (beginning.state != STATE_NORMAL) {
    return null;
  }

  let completionPart = aInputValue.substring(beginning.startPos);

  
  if (completionPart.trim() == "") {
    return null;
  }

  let properties = completionPart.split('.');
  let matchProp;
  if (properties.length > 1) {
      matchProp = properties[properties.length - 1].trimLeft();
      properties.pop();
      for each (var prop in properties) {
        prop = prop.trim();

        
        
        if (typeof obj === "undefined" || obj === null) {
          return null;
        }

        
        
        if (obj.__lookupGetter__(prop)) {
          return null;
        }
        obj = obj[prop];
      }
  }
  else {
    matchProp = properties[0].trimLeft();
  }

  
  
  if (typeof obj === "undefined" || obj === null) {
    return null;
  }

  let matches = [];
  for (var prop in obj) {
    matches.push(prop);
  }

  matches = matches.filter(function(item) {
    return item.indexOf(matchProp) == 0;
  }).sort();

  return {
    matchProp: matchProp,
    matches: matches
  };
}













function JSTermHelper(aJSTerm)
{
  return {
    






    $: function JSTH_$(aId)
    {
      try {
        return aJSTerm._window.document.getElementById(aId);
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
    },

    






    $$: function JSTH_$$(aSelector)
    {
      try {
        return aJSTerm._window.document.querySelectorAll(aSelector);
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
    },

    








    $x: function JSTH_$x(aXPath, aContext)
    {
      let nodes = [];
      let doc = aJSTerm._window.wrappedJSObject.document;
      let aContext = aContext || doc;

      try {
        let results = doc.evaluate(aXPath, aContext, null,
                                    Ci.nsIDOMXPathResult.ANY_TYPE, null);

        let node;
        while (node = results.iterateNext()) {
          nodes.push(node);
        }
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }

      return nodes;
    },

    


    clear: function JSTH_clear()
    {
      aJSTerm.clearOutput();
    },

    






    keys: function JSTH_keys(aObject)
    {
      try {
        return Object.keys(XPCNativeWrapper.unwrap(aObject));
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
    },

    






    values: function JSTH_values(aObject)
    {
      let arrValues = [];
      let obj = XPCNativeWrapper.unwrap(aObject);

      try {
        for (let prop in obj) {
          arrValues.push(obj[prop]);
        }
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
      return arrValues;
    },

    






    inspect: function JSTH_inspect(aObject)
    {
      let obj = XPCNativeWrapper.unwrap(aObject);
      aJSTerm.openPropertyPanel(null, obj);
    },

    






    pprint: function JSTH_pprint(aObject)
    {
      if (aObject === null || aObject === undefined || aObject === true || aObject === false) {
        aJSTerm.console.error(HUDService.getStr("helperFuncUnsupportedTypeError"));
        return;
      }
      let output = [];
      if (typeof aObject != "string") {
        aObject = XPCNativeWrapper.unwrap(aObject);
      }
      let pairs = namesAndValuesOf(aObject);

      pairs.forEach(function(pair) {
        output.push("  " + pair.display);
      });

      aJSTerm.writeOutput(output.join("\n"));
    }
  }
}




















function JSTerm(aContext, aParentNode, aMixin, aConsole)
{
  

  this.application = appName();
  this.context = aContext;
  this.parentNode = aParentNode;
  this.mixins = aMixin;
  this.console = aConsole;

  this.xulElementFactory =
    NodeFactory("xul", "xul", aParentNode.ownerDocument);

  this.textFactory = NodeFactory("text", "xul", aParentNode.ownerDocument);

  this.setTimeout = aParentNode.ownerDocument.defaultView.setTimeout;

  this.historyIndex = 0;
  this.historyPlaceHolder = 0;  
  this.log = LogFactory("*** JSTerm:");
  this.init();
}

JSTerm.prototype = {

  propertyProvider: JSPropertyProvider,

  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,

  init: function JST_init()
  {
    this.createSandbox();
    this.inputNode = this.mixins.inputNode;
    let eventHandlerKeyDown = this.keyDown();
    this.inputNode.addEventListener('keypress', eventHandlerKeyDown, false);
    let eventHandlerInput = this.inputEventHandler();
    this.inputNode.addEventListener('input', eventHandlerInput, false);
    this.outputNode = this.mixins.outputNode;
    if (this.mixins.cssClassOverride) {
      this.cssClassOverride = this.mixins.cssClassOverride;
    }
  },

  get codeInputString()
  {
    
    
    return this.inputNode.value;
  },

  generateUI: function JST_generateUI()
  {
    this.mixins.generateUI();
  },

  attachUI: function JST_attachUI()
  {
    this.mixins.attachUI();
  },

  createSandbox: function JST_setupSandbox()
  {
    
    this.sandbox = new Cu.Sandbox(this._window);
    this.sandbox.window = this._window;
    this.sandbox.console = this.console;
    this.sandbox.__helperFunctions__ = JSTermHelper(this);
    this.sandbox.__proto__ = this._window.wrappedJSObject;
  },

  get _window()
  {
    return this.context.get().QueryInterface(Ci.nsIDOMWindowInternal);
  },
  








  evalInSandbox: function JST_evalInSandbox(aString)
  {
    let execStr = "with(__helperFunctions__) { with(window) {" + aString + "} }";
    return Cu.evalInSandbox(execStr,  this.sandbox, "default", "HUD Console", 1);
  },


  execute: function JST_execute(aExecuteString)
  {
    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      this.console.log("no value to execute");
      return;
    }

    this.writeOutput(aExecuteString, true);

    try {
      var result = this.evalInSandbox(aExecuteString);

      if (result || result === false) {
        this.writeOutputJS(aExecuteString, result);
      }
      else if (result === undefined) {
        this.writeOutput("undefined", false);
      }
      else if (result === null) {
        this.writeOutput("null", false);
      }
    }
    catch (ex) {
      this.console.error(ex);
    }

    this.history.push(aExecuteString);
    this.historyIndex++;
    this.historyPlaceHolder = this.history.length;
    this.setInputValue("");
  },

  













  openPropertyPanel: function JST_openPropertyPanel(aEvalString, aOutputObject,
                                                    aAnchor)
  {
    let self = this;
    let propPanel;
    
    
    
    
    let buttons = [];

    
    
    
    if (aEvalString !== null) {
      buttons.push({
        label: HUDService.getStr("update.button"),
        accesskey: HUDService.getStr("update.accesskey"),
        oncommand: function () {
          try {
            var result = self.evalInSandbox(aEvalString);

            if (result !== undefined) {
              
              
              
              propPanel.treeView.data = result;
            }
          }
          catch (ex) {
            self.console.error(ex);
          }
        }
      });
    }

    buttons.push({
      label: HUDService.getStr("close.button"),
      accesskey: HUDService.getStr("close.accesskey"),
      class: "jsPropertyPanelCloseButton",
      oncommand: function () {
        propPanel.destroy();
        aAnchor._panelOpen = false;
      }
    });

    let doc = self.parentNode.ownerDocument;
    let parent = doc.getElementById("mainPopupSet");
    let title = (aEvalString
        ? HUDService.getFormatStr("jsPropertyInspectTitle", [aEvalString])
        : HUDService.getStr("jsPropertyTitle"));
    propPanel = new PropertyPanel(parent, doc, title, aOutputObject, buttons);

    let panel = propPanel.panel;
    panel.openPopup(aAnchor, "after_pointer", 0, 0, false, false);
    panel.sizeTo(200, 400);
    return propPanel;
  },

  









  writeOutputJS: function JST_writeOutputJS(aEvalString, aOutputObject)
  {
    let lastGroupNode = HUDService.appendGroupIfNecessary(this.outputNode,
                                                      Date.now());

    var self = this;
    var node = this.xulElementFactory("label");
    node.setAttribute("class", "jsterm-output-line hud-clickable");
    node.setAttribute("aria-haspopup", "true");
    node.setAttribute("crop", "end");

    node.addEventListener("mousedown", function(aEvent) {
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    node.addEventListener("click", function(aEvent) {
      if (aEvent.detail != 1 || aEvent.button != 0 ||
          (this._startX != aEvent.clientX &&
           this._startY != aEvent.clientY)) {
        return;
      }

      if (!this._panelOpen) {
        self.openPropertyPanel(aEvalString, aOutputObject, this);
        this._panelOpen = true;
      }
    }, false);

    
    
    
    let textNode = this.textFactory(aOutputObject + "\n");
    node.appendChild(textNode);

    lastGroupNode.appendChild(node);
    ConsoleUtils.scrollToVisible(node);
    pruneConsoleOutputIfNecessary(this.outputNode);
  },

  










  writeOutput: function JST_writeOutput(aOutputMessage, aIsInput)
  {
    let lastGroupNode = HUDService.appendGroupIfNecessary(this.outputNode,
                                                          Date.now());

    var node = this.xulElementFactory("label");
    if (aIsInput) {
      node.setAttribute("class", "jsterm-input-line");
      aOutputMessage = "> " + aOutputMessage;
    }
    else {
      node.setAttribute("class", "jsterm-output-line");
    }

    if (this.cssClassOverride) {
      let classes = this.cssClassOverride.split(" ");
      for (let i = 0; i < classes.length; i++) {
        node.classList.add(classes[i]);
      }
    }

    var textNode = this.textFactory(aOutputMessage + "\n");
    node.appendChild(textNode);

    lastGroupNode.appendChild(node);
    ConsoleUtils.scrollToVisible(node);
    pruneConsoleOutputIfNecessary(this.outputNode);
  },

  clearOutput: function JST_clearOutput()
  {
    let outputNode = this.outputNode;

    while (outputNode.firstChild) {
      outputNode.removeChild(outputNode.firstChild);
    }

    outputNode.lastTimestamp = 0;
  },

  




  resizeInput: function JST_resizeInput()
  {
    let inputNode = this.inputNode;

    
    
    inputNode.style.height = "auto";

    
    let scrollHeight = inputNode.inputField.scrollHeight;
    if (scrollHeight > 0) {
      inputNode.style.height = scrollHeight + "px";
    }
  },

  








  setInputValue: function JST_setInputValue(aNewValue)
  {
    this.inputNode.value = aNewValue;
    this.resizeInput();
  },

  inputEventHandler: function JSTF_inputEventHandler()
  {
    var self = this;
    function handleInputEvent(aEvent) {
      self.resizeInput();
    }
    return handleInputEvent;
  },

  keyDown: function JSTF_keyDown(aEvent)
  {
    var self = this;
    function handleKeyDown(aEvent) {
      
      var setTimeout = aEvent.target.ownerDocument.defaultView.setTimeout;
      var target = aEvent.target;
      var tmp;

      if (aEvent.ctrlKey) {
        switch (aEvent.charCode) {
          case 97:
            
            tmp = self.codeInputString;
            setTimeout(function() {
              self.setInputValue(tmp);
              self.inputNode.setSelectionRange(0, 0);
            }, 0);
            break;
          case 101:
            
            tmp = self.codeInputString;
            self.setInputValue("");
            setTimeout(function(){
              self.setInputValue(tmp);
            }, 0);
            break;
          default:
            return;
        }
        return;
      }
      else if (aEvent.shiftKey && aEvent.keyCode == 13) {
        
        
        return;
      }
      else {
        switch(aEvent.keyCode) {
          case 13:
            
            self.execute();
            aEvent.preventDefault();
            break;
          case 38:
            
            if (self.caretAtStartOfInput()) {
              let updated = self.historyPeruse(HISTORY_BACK);
              if (updated && aEvent.cancelable) {
                self.inputNode.setSelectionRange(0, 0);
                aEvent.preventDefault();
              }
            }
            break;
          case 40:
            
            if (self.caretAtEndOfInput()) {
              let updated = self.historyPeruse(HISTORY_FORWARD);
              if (updated && aEvent.cancelable) {
                let inputEnd = self.inputNode.value.length;
                self.inputNode.setSelectionRange(inputEnd, inputEnd);
                aEvent.preventDefault();
              }
            }
            break;
          case 9:
            
            
            
            
            var completionResult;
            if (aEvent.shiftKey) {
              completionResult = self.complete(self.COMPLETE_BACKWARD);
            }
            else {
              completionResult = self.complete(self.COMPLETE_FORWARD);
            }
            if (completionResult) {
              if (aEvent.cancelable) {
              aEvent.preventDefault();
            }
            aEvent.target.focus();
            }
            break;
          case 8:
            
          case 46:
            
            
            break;
          default:
            
            
            
            
            
            var value = self.inputNode.value;
            setTimeout(function() {
              if (self.inputNode.value !== value) {
                self.complete(self.COMPLETE_HINT_ONLY);
              }
            }, 0);
            break;
        }
        return;
      }
    }
    return handleKeyDown;
  },

  








  historyPeruse: function JST_historyPeruse(aDirection)
  {
    if (!this.history.length) {
      return false;
    }

    
    if (aDirection == HISTORY_BACK) {
      if (this.historyPlaceHolder <= 0) {
        return false;
      }

      let inputVal = this.history[--this.historyPlaceHolder];
      if (inputVal){
        this.setInputValue(inputVal);
      }
    }
    
    else if (aDirection == HISTORY_FORWARD) {
      if (this.historyPlaceHolder == this.history.length - 1) {
        this.historyPlaceHolder ++;
        this.setInputValue("");
      }
      else if (this.historyPlaceHolder >= (this.history.length)) {
        return false;
      }
      else {
        let inputVal = this.history[++this.historyPlaceHolder];
        if (inputVal){
          this.setInputValue(inputVal);
        }
      }
    }
    else {
      throw new Error("Invalid argument 0");
    }

    return true;
  },

  refocus: function JSTF_refocus()
  {
    this.inputNode.blur();
    this.inputNode.focus();
  },

  





  caretAtStartOfInput: function JST_caretAtStartOfInput()
  {
    return this.inputNode.selectionStart == this.inputNode.selectionEnd &&
        this.inputNode.selectionStart == 0;
  },

  





  caretAtEndOfInput: function JST_caretAtEndOfInput()
  {
    return this.inputNode.selectionStart == this.inputNode.selectionEnd &&
        this.inputNode.selectionStart == this.inputNode.value.length;
  },

  history: [],

  
  lastCompletion: null,

  


























  complete: function JSTF_complete(type)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    
    if (!inputValue) {
      return false;
    }
    let selStart = inputNode.selectionStart, selEnd = inputNode.selectionEnd;

    
    if (selStart > selEnd) {
      let newSelEnd = selStart;
      selStart = selEnd;
      selEnd = newSelEnd;
    }

    
    if (selEnd != inputValue.length) {
      this.lastCompletion = null;
      return false;
    }

    
    inputValue = inputValue.substring(0, selStart);

    let matches;
    let matchIndexToUse;
    let matchOffset;
    let completionStr;

    
    
    if (this.lastCompletion && inputValue == this.lastCompletion.value) {
      matches = this.lastCompletion.matches;
      matchOffset = this.lastCompletion.matchOffset;
      if (type === this.COMPLETE_BACKWARD) {
        this.lastCompletion.index --;
      }
      else if (type === this.COMPLETE_FORWARD) {
        this.lastCompletion.index ++;
      }
      matchIndexToUse = this.lastCompletion.index;
    }
    else {
      
      let completion = this.propertyProvider(this.sandbox.window, inputValue);
      if (!completion) {
        return false;
      }
      matches = completion.matches;
      matchIndexToUse = 0;
      matchOffset = completion.matchProp.length
      
      this.lastCompletion = {
        index: 0,
        value: inputValue,
        matches: matches,
        matchOffset: matchOffset
      };
    }

    if (matches.length != 0) {
      
      if (matchIndexToUse < 0) {
        matchIndexToUse = matches.length + (matchIndexToUse % matches.length);
        if (matchIndexToUse == matches.length) {
          matchIndexToUse = 0;
        }
      }
      else {
        matchIndexToUse = matchIndexToUse % matches.length;
      }

      completionStr = matches[matchIndexToUse].substring(matchOffset);
      this.setInputValue(inputValue + completionStr);

      selEnd = inputValue.length + completionStr.length;

      
      
      
      if (matches.length > 1 || type === this.COMPLETE_HINT_ONLY) {
        inputNode.setSelectionRange(selStart, selEnd);
      }
      else {
        inputNode.setSelectionRange(selEnd, selEnd);
      }

      return completionStr ? true : false;
    }

    return false;
  }
};







function
JSTermFirefoxMixin(aContext,
                   aParentNode,
                   aExistingConsole,
                   aCSSClassOverride)
{
  
  
  
  this.cssClassOverride = aCSSClassOverride;
  this.context = aContext;
  this.parentNode = aParentNode;
  this.existingConsoleNode = aExistingConsole;
  this.setTimeout = aParentNode.ownerDocument.defaultView.setTimeout;

  if (aParentNode.ownerDocument) {
    this.xulElementFactory =
      NodeFactory("xul", "xul", aParentNode.ownerDocument);

    this.textFactory = NodeFactory("text", "xul", aParentNode.ownerDocument);
    this.generateUI();
    this.attachUI();
  }
  else {
    throw new Error("aParentNode should be a DOM node with an ownerDocument property ");
  }
}

JSTermFirefoxMixin.prototype = {
  





  generateUI: function JSTF_generateUI()
  {
    let inputContainer = this.xulElementFactory("hbox");
    inputContainer.setAttribute("class", "jsterm-input-container");

    let inputNode = this.xulElementFactory("textbox");
    inputNode.setAttribute("class", "jsterm-input-node");
    inputNode.setAttribute("flex", "1");
    inputNode.setAttribute("multiline", "true");
    inputNode.setAttribute("rows", "1");
    inputContainer.appendChild(inputNode);

    let closeButton = this.xulElementFactory("button");
    closeButton.setAttribute("class", "jsterm-close-button");
    inputContainer.appendChild(closeButton);
    closeButton.addEventListener("command", HeadsUpDisplayUICommands.toggleHUD,
                                 false);

    if (this.existingConsoleNode == undefined) {
      
      let term = this.xulElementFactory("vbox");
      term.setAttribute("class", "jsterm-wrapper-node");
      term.setAttribute("flex", "1");

      let outputNode = this.xulElementFactory("vbox");
      outputNode.setAttribute("class", "jsterm-output-node");

      
      term.appendChild(outputNode);
      term.appendChild(inputNode);

      this.outputNode = outputNode;
      this.inputNode = inputNode;
      this.term = term;
    }
    else {
      this.inputNode = inputNode;
      this.term = inputContainer;
      this.outputNode = this.existingConsoleNode;
    }
  },

  get inputValue()
  {
    return this.inputNode.value;
  },

  attachUI: function JSTF_attachUI()
  {
    this.parentNode.appendChild(this.term);
  }
};




function LogMessage(aMessage, aLevel, aOutputNode, aActivityObject)
{
  if (!aOutputNode || !aOutputNode.ownerDocument) {
    throw new Error("aOutputNode is required and should be type nsIDOMNode");
  }
  if (!aMessage.origin) {
    throw new Error("Cannot create and log a message without an origin");
  }
  this.message = aMessage;
  if (aMessage.domId) {
    
    
    this.domId = aMessage.domId;
  }
  this.activityObject = aActivityObject;
  this.outputNode = aOutputNode;
  this.level = aLevel;
  this.origin = aMessage.origin;

  this.xulElementFactory =
  NodeFactory("xul", "xul", aOutputNode.ownerDocument);

  this.textFactory = NodeFactory("text", "xul", aOutputNode.ownerDocument);

  this.createLogNode();
}

LogMessage.prototype = {

  




  createLogNode: function LM_createLogNode()
  {
    this.messageNode = this.xulElementFactory("label");

    var ts = ConsoleUtils.timestamp();
    this.timestampedMessage = ConsoleUtils.timestampString(ts) + ": " +
      this.message.message;
    var messageTxtNode = this.textFactory(this.timestampedMessage + "\n");

    this.messageNode.appendChild(messageTxtNode);

    this.messageNode.classList.add("hud-msg-node");
    this.messageNode.classList.add("hud-" + this.level);

    if (this.activityObject.category == "CSS Parser") {
      this.messageNode.classList.add("hud-cssparser");
    }

    var self = this;

    var messageObject = {
      logLevel: self.level,
      message: self.message,
      timestamp: ts,
      activity: self.activityObject,
      origin: self.origin,
      hudId: self.message.hudId,
    };

    this.messageObject = messageObject;
  }
};







function FirefoxApplicationHooks()
{ }

FirefoxApplicationHooks.prototype = {

  


  get chromeWindows()
  {
    var windows = [];
    var enumerator = Services.ww.getWindowEnumerator(null);
    while (enumerator.hasMoreElements()) {
      windows.push(enumerator.getNext());
    }
    return windows;
  },

  





  getOutputNodeById: function FAH_getOutputNodeById(aId)
  {
    if (!aId) {
      throw new Error("FAH_getOutputNodeById: id is null!!");
    }
    var enumerator = Services.ww.getWindowEnumerator(null);
    while (enumerator.hasMoreElements()) {
      let window = enumerator.getNext();
      let node = window.document.getElementById(aId);
      if (node) {
        return node;
      }
    }
    throw new Error("Cannot get outputNode by id");
  },

  




  getCurrentContext: function FAH_getCurrentContext()
  {
    return Services.wm.getMostRecentWindow("navigator:browser");
  }
};










ConsoleUtils = {

  




  timestamp: function ConsoleUtils_timestamp()
  {
    return Date.now();
  },

  






  timestampString: function ConsoleUtils_timestampString(ms)
  {
    var d = new Date(ms ? ms : null);
    let hours = d.getHours(), minutes = d.getMinutes();
    let seconds = d.getSeconds(), milliseconds = d.getMilliseconds();
    let parameters = [ hours, minutes, seconds, milliseconds ];
    return HUDService.getFormatStr("timestampFormat", parameters);
  },

  







  scrollToVisible: function ConsoleUtils_scrollToVisible(aNode) {
    let scrollBoxNode = aNode.parentNode;
    while (scrollBoxNode.tagName !== "scrollbox") {
      scrollBoxNode = scrollBoxNode.parentNode;
    }

    let boxObject = scrollBoxNode.boxObject;
    let nsIScrollBoxObject = boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    nsIScrollBoxObject.ensureElementIsVisible(aNode);
  }
};





HeadsUpDisplayUICommands = {
  toggleHUD: function UIC_toggleHUD() {
    var window = HUDService.currentContext();
    var gBrowser = window.gBrowser;
    var linkedBrowser = gBrowser.selectedTab.linkedBrowser;
    var tabId = gBrowser.getNotificationBox(linkedBrowser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var hud = gBrowser.selectedTab.ownerDocument.getElementById(hudId);
    if (hud) {
      HUDService.deactivateHUDForContext(gBrowser.selectedTab);
    }
    else {
      HUDService.activateHUDForContext(gBrowser.selectedTab);
    }
  },

  toggleFilter: function UIC_toggleFilter(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    var state = HUDService.getFilterState(hudId, filter);
    if (state) {
      HUDService.setFilterState(hudId, filter, false);
      aButton.setAttribute("checked", false);
    }
    else {
      HUDService.setFilterState(hudId, filter, true);
      aButton.setAttribute("checked", true);
    }
  },

  command: function UIC_command(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    switch (filter) {
      case "clear":
        HUDService.clearDisplay(hudId);
        break;
      case "selectAll":
        let outputNode = HUDService.getOutputNodeById(hudId);
        let chromeWindow = outputNode.ownerDocument.defaultView;
        let commandController = chromeWindow.commandController;
        commandController.selectAll(outputNode);
        break;
      case "saveBodies": {
        let checked = aButton.getAttribute("checked") === "true";
        HUDService.saveRequestAndResponseBodies = checked;
        break;
      }
    }
  },

};





var prefs = Services.prefs;

const GLOBAL_STORAGE_INDEX_ID = "GLOBAL_CONSOLE";
const PREFS_BRANCH_PREF = "devtools.hud.display.filter";
const PREFS_PREFIX = "devtools.hud.display.filter.";
const PREFS = { network: PREFS_PREFIX + "network",
                cssparser: PREFS_PREFIX + "cssparser",
                exception: PREFS_PREFIX + "exception",
                error: PREFS_PREFIX + "error",
                info: PREFS_PREFIX + "info",
                warn: PREFS_PREFIX + "warn",
                log: PREFS_PREFIX + "log",
                global: PREFS_PREFIX + "global",
              };

function ConsoleStorage()
{
  this.sequencer = null;
  this.consoleDisplays = {};
  
  this.displayIndexes = {};
  this.globalStorageIndex = [];
  this.globalDisplay = {};
  this.createDisplay(GLOBAL_STORAGE_INDEX_ID);
  
  

  
  this.displayPrefs = {};

  
  let filterPrefs;
  let defaultDisplayPrefs;

  try {
    filterPrefs = prefs.getBoolPref(PREFS_BRANCH_PREF);
  }
  catch (ex) {
    filterPrefs = false;
  }

  
  
  

  if (filterPrefs) {
    defaultDisplayPrefs = {
      network: (prefs.getBoolPref(PREFS.network) ? true: false),
      cssparser: (prefs.getBoolPref(PREFS.cssparser) ? true: false),
      exception: (prefs.getBoolPref(PREFS.exception) ? true: false),
      error: (prefs.getBoolPref(PREFS.error) ? true: false),
      info: (prefs.getBoolPref(PREFS.info) ? true: false),
      warn: (prefs.getBoolPref(PREFS.warn) ? true: false),
      log: (prefs.getBoolPref(PREFS.log) ? true: false),
      global: (prefs.getBoolPref(PREFS.global) ? true: false),
    };
  }
  else {
    prefs.setBoolPref(PREFS_BRANCH_PREF, false);
    
    prefs.setBoolPref(PREFS.network, true);
    prefs.setBoolPref(PREFS.cssparser, true);
    prefs.setBoolPref(PREFS.exception, true);
    prefs.setBoolPref(PREFS.error, true);
    prefs.setBoolPref(PREFS.info, true);
    prefs.setBoolPref(PREFS.warn, true);
    prefs.setBoolPref(PREFS.log, true);
    prefs.setBoolPref(PREFS.global, false);

    defaultDisplayPrefs = {
      network: prefs.getBoolPref(PREFS.network),
      cssparser: prefs.getBoolPref(PREFS.cssparser),
      exception: prefs.getBoolPref(PREFS.exception),
      error: prefs.getBoolPref(PREFS.error),
      info: prefs.getBoolPref(PREFS.info),
      warn: prefs.getBoolPref(PREFS.warn),
      log: prefs.getBoolPref(PREFS.log),
      global: prefs.getBoolPref(PREFS.global),
    };
  }
  this.defaultDisplayPrefs = defaultDisplayPrefs;
}

ConsoleStorage.prototype = {

  updateDefaultDisplayPrefs:
  function CS_updateDefaultDisplayPrefs(aPrefsObject) {
    prefs.setBoolPref(PREFS.network, (aPrefsObject.network ? true : false));
    prefs.setBoolPref(PREFS.cssparser, (aPrefsObject.cssparser ? true : false));
    prefs.setBoolPref(PREFS.exception, (aPrefsObject.exception ? true : false));
    prefs.setBoolPref(PREFS.error, (aPrefsObject.error ? true : false));
    prefs.setBoolPref(PREFS.info, (aPrefsObject.info ? true : false));
    prefs.setBoolPref(PREFS.warn, (aPrefsObject.warn ? true : false));
    prefs.setBoolPref(PREFS.log, (aPrefsObject.log ? true : false));
    prefs.setBoolPref(PREFS.global, (aPrefsObject.global ? true : false));
  },

  sequenceId: function CS_sequencerId()
  {
    if (!this.sequencer) {
      this.sequencer = this.createSequencer();
    }
    return this.sequencer.next();
  },

  createSequencer: function CS_createSequencer()
  {
    function sequencer(aInt) {
      while(1) {
        aInt++;
        yield aInt;
      }
    }
    return sequencer(-1);
  },

  globalStore: function CS_globalStore(aIndex)
  {
    return this.displayStore(GLOBAL_CONSOLE_DOM_NODE_ID);
  },

  displayStore: function CS_displayStore(aId)
  {
    var self = this;
    var idx = -1;
    var id = aId;
    var aLength = self.displayIndexes[id].length;

    function displayStoreGenerator(aInt, aLength)
    {
      
      
      while(1) {
        
        aInt++;
        var indexIt = self.displayIndexes[id];
        var index = indexIt[aInt];
        if (aLength < aInt) {
          
          var newLength = self.displayIndexes[id].length;
          if (newLength > aLength) {
            aLength = newLength;
          }
          else {
            throw new StopIteration();
          }
        }
        var entry = self.consoleDisplays[id][index];
        yield entry;
      }
    }

    return displayStoreGenerator(-1, aLength);
  },

  recordEntries: function CS_recordEntries(aHUDId, aConfigArray)
  {
    var len = aConfigArray.length;
    for (var i = 0; i < len; i++){
      this.recordEntry(aHUDId, aConfigArray[i]);
    }
  },


  recordEntry: function CS_recordEntry(aHUDId, aConfig)
  {
    var id = this.sequenceId();

    this.globalStorageIndex[id] = { hudId: aHUDId };

    var displayStorage = this.consoleDisplays[aHUDId];

    var displayIndex = this.displayIndexes[aHUDId];

    if (displayStorage && displayIndex) {
      var entry = new ConsoleEntry(aConfig, id);
      displayIndex.push(entry.id);
      displayStorage[entry.id] = entry;
      return entry;
    }
    else {
      throw new Error("Cannot get displayStorage or index object for id " + aHUDId);
    }
  },

  getEntry: function CS_getEntry(aId)
  {
    var display = this.globalStorageIndex[aId];
    var storName = display.hudId;
    return this.consoleDisplays[storName][aId];
  },

  updateEntry: function CS_updateEntry(aUUID)
  {
    
    
  },

  createDisplay: function CS_createdisplay(aId)
  {
    if (!this.consoleDisplays[aId]) {
      this.consoleDisplays[aId] = {};
      this.displayIndexes[aId] = [];
    }
  },

  removeDisplay: function CS_removeDisplay(aId)
  {
    try {
      delete this.consoleDisplays[aId];
      delete this.displayIndexes[aId];
    }
    catch (ex) {
      Cu.reportError("Could not remove console display for id " + aId);
    }
  }
};









function ConsoleEntry(aConfig, id)
{
  if (!aConfig.logLevel && aConfig.message) {
    throw new Error("Missing Arguments when creating a console entry");
  }

  this.config = aConfig;
  this.id = id;
  for (var prop in aConfig) {
    if (!(typeof aConfig[prop] == "function")){
      this[prop] = aConfig[prop];
    }
  }

  if (aConfig.logLevel == "network") {
    this.transactions = { };
    if (aConfig.activity) {
      this.transactions[aConfig.activity.stage] = aConfig.activity;
    }
  }

}

ConsoleEntry.prototype = {

  updateTransaction: function CE_updateTransaction(aActivity) {
    this.transactions[aActivity.stage] = aActivity;
  }
};





HUDWindowObserver = {
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.nsIObserver,]
  ),

  init: function HWO_init()
  {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, "content-document-global-created", false);
  },

  observe: function HWO_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "content-document-global-created") {
      HUDService.windowInitializer(aSubject);
    }
    else if (aTopic == "xpcom-shutdown") {
      this.uninit();
    }
  },

  uninit: function HWO_uninit()
  {
    Services.obs.removeObserver(this, "content-document-global-created");
    HUDService.shutdown();
  },

  



  initialConsoleCreated: false,
};









function CommandController(aWindow) {
  this.window = aWindow;
}

CommandController.prototype = {
  






  _getFocusedOutputNode: function CommandController_getFocusedOutputNode()
  {
    let anchorNode = this.window.getSelection().anchorNode;
    while (!(anchorNode.nodeType === anchorNode.ELEMENT_NODE &&
             anchorNode.classList.contains("hud-output-node"))) {
      anchorNode = anchorNode.parentNode;
    }
    return anchorNode;
  },

  






  selectAll: function CommandController_selectAll(aOutputNode)
  {
    let selection = this.window.getSelection();
    selection.removeAllRanges();
    selection.selectAllChildren(aOutputNode);
  },

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    return aCommand === "cmd_selectAll" &&
           this._getFocusedOutputNode() != null;
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    return aCommand === "cmd_selectAll";
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    this.selectAll(this._getFocusedOutputNode());
  }
};











HUDConsoleObserver = {
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.nsIObserver]
  ),

  init: function HCO_init()
  {
    Services.console.registerListener(this);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  observe: function HCO_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      Services.console.unregisterListener(this);
    }

    if (aSubject instanceof Ci.nsIScriptError) {
      switch (aSubject.category) {
        
        
        case "XPConnect JavaScript":
          
          
        case "component javascript":
        case "chrome javascript":
          
        case "chrome registration":
          
        case "XBL":
          
        case "XBL Prototype Handler":
          
        case "XBL Content Sink":
          
        case "xbl javascript":
          
        case "FrameConstructor":
          
          return;

        
        case "HUDConsole":
        case "CSS Parser":
          
        case "CSS Loader":
          
        case "content javascript":
          
        case "DOM Events":
          
          
          
          
          
        case "DOM:HTML":
          
        case "DOM Window":
          
          
          
        case "SVG":
          
          
        case "ImageMap":
          
        case "HTML":
          
        case "Canvas":
          
          
          
        case "DOM3 Load":
          
          
          
          
          
        case "DOM":
          
          
          
          
        case "malformed-xml":
          
          
          
          
        case "DOM Worker javascript":
          
          
          
          
          
          HUDService.reportConsoleServiceContentScriptError(aSubject);
          return;
        default:
          HUDService.reportConsoleServiceMessage(aSubject);
          return;
      }
    }
  }
};










function appName()
{
  let APP_ID = Services.appinfo.QueryInterface(Ci.nsIXULRuntime).ID;

  let APP_ID_TABLE = {
    "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}": "FIREFOX" ,
    "{3550f703-e582-4d05-9a08-453d09bdfdc6}": "THUNDERBIRD",
    "{a23983c0-fd0e-11dc-95ff-0800200c9a66}": "FENNEC" ,
    "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}": "SEAMONKEY",
  };

  let name = APP_ID_TABLE[APP_ID];

  if (name){
    return name;
  }
  throw new Error("appName: UNSUPPORTED APPLICATION UUID");
}





try {
  
  
  
  var HUDService = new HUD_SERVICE();
  HUDWindowObserver.init();
  HUDConsoleObserver.init();
}
catch (ex) {
  Cu.reportError("HUDService failed initialization.\n" + ex);
  
  
}
