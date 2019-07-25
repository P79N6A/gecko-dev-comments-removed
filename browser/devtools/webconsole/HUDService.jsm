













































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const CONSOLEAPI_CLASS_ID = "{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/NetworkHelper.jsm");
Cu.import("resource:///modules/PropertyPanel.jsm");

var EXPORTED_SYMBOLS = ["HUDService", "ConsoleUtils"];

XPCOMUtils.defineLazyServiceGetter(this, "scriptError",
                                   "@mozilla.org/scripterror;1",
                                   "nsIScriptError");

XPCOMUtils.defineLazyServiceGetter(this, "activityDistributor",
                                   "@mozilla.org/network/http-activity-distributor;1",
                                   "nsIHttpActivityDistributor");

XPCOMUtils.defineLazyServiceGetter(this, "mimeService",
                                   "@mozilla.org/mime;1",
                                   "nsIMIMEService");

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

XPCOMUtils.defineLazyGetter(this, "gcli", function () {
  var obj = {};
  Cu.import("resource:///modules/gcli.jsm", obj);
  return obj.gcli;
});

XPCOMUtils.defineLazyGetter(this, "GcliCommands", function () {
  var obj = {};
  Cu.import("resource:///modules/GcliCommands.jsm", obj);
  return obj.GcliCommands;
});

XPCOMUtils.defineLazyGetter(this, "StyleInspector", function () {
  var obj = {};
  Cu.import("resource:///modules/devtools/StyleInspector.jsm", obj);
  return obj.StyleInspector;
});

XPCOMUtils.defineLazyGetter(this, "NetUtil", function () {
  var obj = {};
  Cu.import("resource://gre/modules/NetUtil.jsm", obj);
  return obj.NetUtil;
});

XPCOMUtils.defineLazyGetter(this, "PropertyPanel", function () {
  var obj = {};
  try {
    Cu.import("resource:///modules/PropertyPanel.jsm", obj);
  } catch (err) {
    Cu.reportError(err);
  }
  return obj.PropertyPanel;
});

XPCOMUtils.defineLazyGetter(this, "AutocompletePopup", function () {
  var obj = {};
  try {
    Cu.import("resource:///modules/AutocompletePopup.jsm", obj);
  }
  catch (err) {
    Cu.reportError(err);
  }
  return obj.AutocompletePopup;
});

XPCOMUtils.defineLazyGetter(this, "namesAndValuesOf", function () {
  var obj = {};
  Cu.import("resource:///modules/PropertyPanel.jsm", obj);
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

const HUD_STRINGS_URI = "chrome://global/locale/headsUpDisplay.properties";

XPCOMUtils.defineLazyGetter(this, "stringBundle", function () {
  return Services.strings.createBundle(HUD_STRINGS_URI);
});



const NEW_GROUP_DELAY = 5000;



const SEARCH_DELAY = 200;




const DEFAULT_LOG_LIMIT = 200;



const CATEGORY_NETWORK = 0;
const CATEGORY_CSS = 1;
const CATEGORY_JS = 2;
const CATEGORY_WEBDEV = 3;
const CATEGORY_INPUT = 4;   
const CATEGORY_OUTPUT = 5;  



const SEVERITY_ERROR = 0;
const SEVERITY_WARNING = 1;
const SEVERITY_INFO = 2;
const SEVERITY_LOG = 3;



const LEVELS = {
  error: SEVERITY_ERROR,
  warn: SEVERITY_WARNING,
  info: SEVERITY_INFO,
  log: SEVERITY_LOG,
  trace: SEVERITY_LOG,
  dir: SEVERITY_LOG,
  group: SEVERITY_LOG,
  groupCollapsed: SEVERITY_LOG,
  groupEnd: SEVERITY_LOG
};


const MIN_HTTP_ERROR_CODE = 400;

const MAX_HTTP_ERROR_CODE = 600;


const HTTP_MOVED_PERMANENTLY = 301;
const HTTP_FOUND = 302;
const HTTP_SEE_OTHER = 303;
const HTTP_TEMPORARY_REDIRECT = 307;


const HTML_NS = "http://www.w3.org/1999/xhtml";


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


const CATEGORY_CLASS_FRAGMENTS = [
  "network",
  "cssparser",
  "exception",
  "console",
  "input",
  "output",
];


const SEVERITY_CLASS_FRAGMENTS = [
  "error",
  "warn",
  "info",
  "log",
];






const MESSAGE_PREFERENCE_KEYS = [

  [ "network",    null,         null,   "networkinfo", ],  
  [ "csserror",   "cssparser",  null,   null,          ],  
  [ "exception",  "jswarn",     null,   null,          ],  
  [ "error",      "warn",       "info", "log",         ],  
  [ null,         null,         null,   null,          ],  
  [ null,         null,         null,   null,          ],  
];


const ANIMATE_OUT = 0;
const ANIMATE_IN = 1;


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;


const RESPONSE_BODY_LIMIT = 1024*1024; 


const PR_UINT32_MAX = 4294967295;


const MINIMUM_CONSOLE_HEIGHT = 150;



const MINIMUM_PAGE_HEIGHT = 50;


const DEFAULT_CONSOLE_HEIGHT = 0.33;


const TYPEOF_FUNCTION = "function";

const ERRORS = { LOG_MESSAGE_MISSING_ARGS:
                 "Missing arguments: aMessage, aConsoleNode and aMessageNode are required.",
                 CANNOT_GET_HUD: "Cannot getHeads Up Display with provided ID",
                 MISSING_ARGS: "Missing arguments",
                 LOG_OUTPUT_FAILED: "Log Failure: Could not append messageNode to outputNode",
};


const GROUP_INDENT = 12;
















function ResponseListener(aHttpActivity) {
  this.receivedData = "";
  this.httpActivity = aHttpActivity;
}

ResponseListener.prototype =
{
  



  sink: null,

  


  httpActivity: null,

  


  receivedData: null,

  


  request: null,

  




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

  













  setAsyncListener: function RL_setAsyncListener(aStream, aListener)
  {
    
    aStream.asyncWait(aListener, 0, 0, Services.tm.mainThread);
  },

  













  onDataAvailable: function RL_onDataAvailable(aRequest, aContext, aInputStream,
                                               aOffset, aCount)
  {
    this.setResponseHeader(aRequest);

    let data = NetUtil.readInputStreamToString(aInputStream, aCount);

    if (!this.httpActivity.response.bodyDiscarded &&
        this.receivedData.length < RESPONSE_BODY_LIMIT) {
      this.receivedData += NetworkHelper.
                           convertToUnicode(data, aRequest.contentCharset);
    }
  },

  






  onStartRequest: function RL_onStartRequest(aRequest, aContext)
  {
    this.request = aRequest;

    
    
    this.httpActivity.response.bodyDiscarded =
      !HUDService.saveRequestAndResponseBodies;

    
    if (!this.httpActivity.response.bodyDiscarded &&
        this.httpActivity.channel instanceof Ci.nsIHttpChannel) {
      switch (this.httpActivity.channel.responseStatus) {
        case HTTP_MOVED_PERMANENTLY:
        case HTTP_FOUND:
        case HTTP_SEE_OTHER:
        case HTTP_TEMPORARY_REDIRECT:
          this.httpActivity.response.bodyDiscarded = true;
          break;
      }
    }

    
    this.setAsyncListener(this.sink.inputStream, this);
  },

  











  onStopRequest: function RL_onStopRequest(aRequest, aContext, aStatusCode)
  {
    
    let response = null;
    for each (let item in HUDService.openResponseHeaders) {
      if (item.channel === this.httpActivity.channel) {
        response = item;
        break;
      }
    }

    if (response) {
      this.httpActivity.response.header = response.headers;
      delete HUDService.openResponseHeaders[response.id];
    }
    else {
      this.setResponseHeader(aRequest);
    }

    this.sink.outputStream.close();
  },

  






  onStreamClose: function RL_onStreamClose()
  {
    if (!this.httpActivity) {
      return;
    }

    
    this.setAsyncListener(this.sink.inputStream, null);

    if (!this.httpActivity.response.bodyDiscarded &&
        HUDService.saveRequestAndResponseBodies) {
      this.httpActivity.response.body = this.receivedData;
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
    this.httpActivity = null;
    this.receivedData = "";
    this.request = null;
    this.sink = null;
    this.inputStream = null;
  },

  








  onInputStreamReady: function RL_onInputStreamReady(aStream)
  {
    if (!(aStream instanceof Ci.nsIAsyncInputStream) || !this.httpActivity) {
      return;
    }

    let available = -1;
    try {
      
      available = aStream.available();
    }
    catch (ex) { }

    if (available != -1) {
      if (available != 0) {
        
        
        
        this.onDataAvailable(this.request, null, aStream, 0, available);
      }
      this.setAsyncListener(aStream, this);
    }
    else {
      this.onStreamClose();
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIStreamListener,
    Ci.nsIInputStreamCallback,
    Ci.nsIRequestObserver,
    Ci.nsISupports,
  ])
}

















function createElement(aDocument, aTag, aAttributes)
{
  let node = aDocument.createElement(aTag);
  if (aAttributes) {
    for (let attr in aAttributes) {
      node.setAttribute(attr, aAttributes[attr]);
    }
  }
  return node;
}













function createAndAppendElement(aParent, aTag, aAttributes)
{
  let node = createElement(aParent.ownerDocument, aTag, aAttributes);
  aParent.appendChild(node);
  return node;
}







function unwrap(aObject)
{
  try {
    return XPCNativeWrapper.unwrap(aObject);
  } catch(e) {
    return aObject;
  }
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
    self.panel.removeEventListener("load", onLoad, true);
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

      







      let row = doc.createElement("tr");
      let textNode = doc.createTextNode(key + ":");
      let th = doc.createElement("th");
      th.setAttribute("scope", "row");
      th.setAttribute("class", "property-name");
      th.appendChild(textNode);
      row.appendChild(th);

      textNode = doc.createTextNode(sortedList[key]);
      let td = doc.createElement("td");
      td.setAttribute("class", "property-value");
      td.appendChild(textNode);
      row.appendChild(td);

      parent.appendChild(row);
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















function pruneConsoleOutputIfNecessary(aHUDId, aCategory)
{
  
  let logLimit;
  try {
    let prefName = CATEGORY_CLASS_FRAGMENTS[aCategory];
    logLimit = Services.prefs.getIntPref("devtools.hud.loglimit." + prefName);
  } catch (e) {
    logLimit = DEFAULT_LOG_LIMIT;
  }

  let hudRef = HUDService.getHudReferenceById(aHUDId);
  let outputNode = hudRef.outputNode;

  let scrollBox = outputNode.scrollBoxObject.element;
  let oldScrollHeight = scrollBox.scrollHeight;
  let scrolledToBottom = ConsoleUtils.isOutputScrolledToBottom(outputNode);

  
  let messageNodes = outputNode.querySelectorAll(".webconsole-msg-" +
      CATEGORY_CLASS_FRAGMENTS[aCategory]);
  let removeNodes = messageNodes.length - logLimit;
  for (let i = 0; i < removeNodes; i++) {
    if (messageNodes[i].classList.contains("webconsole-msg-cssparser")) {
      let desc = messageNodes[i].childNodes[2].textContent;
      let location = "";
      if (messageNodes[i].childNodes[4]) {
        location = messageNodes[i].childNodes[4].getAttribute("title");
      }
      delete hudRef.cssNodes[desc + location];
    }
    else if (messageNodes[i].classList.contains("webconsole-msg-inspector")) {
      hudRef.pruneConsoleDirNode(messageNodes[i]);
      continue;
    }
    messageNodes[i].parentNode.removeChild(messageNodes[i]);
  }

  if (!scrolledToBottom && removeNodes > 0 &&
      oldScrollHeight != scrollBox.scrollHeight) {
    scrollBox.scrollTop -= oldScrollHeight - scrollBox.scrollHeight;
  }

  return logLimit;
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

  
  
  this.onTabClose = this.onTabClose.bind(this);
  this.onWindowUnload = this.onWindowUnload.bind(this);

  
  this.lastConsoleHeight = Services.prefs.getIntPref("devtools.hud.height");

  
  
  this.responsePipeSegmentSize =
    Services.prefs.getIntPref("network.buffer.cache.size");

  



  this.activatedContexts = [];

  


  this.windowIds = {};

  


  this.filterPrefs = {};

  


  this.hudReferences = {};

  


  this.openRequests = {};

  


  this.openResponseHeaders = {};
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

  



  sequencer: null,

  





  getWindowId: function HS_getWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },

  






  getWindowByWindowId: function HS_getWindowByWindowId(aId)
  {
    
    
    
    

    let someWindow = Services.wm.getMostRecentWindow(null);
    let content = null;

    if (someWindow) {
      let windowUtils = someWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIDOMWindowUtils);
      content = windowUtils.getOuterWindowWithId(aId);
    }

    return content;
  },

  



  saveRequestAndResponseBodies: false,

  






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

  






  activateHUDForContext: function HS_activateHUDForContext(aContext, aAnimated)
  {
    this.wakeup();

    let window = aContext.linkedBrowser.contentWindow;
    let nBox = aContext.ownerDocument.defaultView.
      getNotificationBox(window);
    this.registerActiveContext(nBox.id);
    this.windowInitializer(window);

    let hudId = "hud_" + nBox.id;
    let hudRef = this.hudReferences[hudId];

    if (!aAnimated || hudRef.consolePanel) {
      this.disableAnimation(hudId);
    }

    
    
    
    let procInstr = aContext.ownerDocument.gcliCssProcInstr;
    if (!procInstr) {
      procInstr = aContext.ownerDocument.createProcessingInstruction(
              "xml-stylesheet",
              "href='chrome://browser/skin/devtools/gcli.css' type='text/css'");
      procInstr.contexts = [];

      let root = aContext.ownerDocument.getElementsByTagName('window')[0];
      root.parentNode.insertBefore(procInstr, root);
      aContext.ownerDocument.gcliCssProcInstr = procInstr;
    }
    if (procInstr.contexts.indexOf(hudId) == -1) {
      procInstr.contexts.push(hudId);
    }
  },

  






  deactivateHUDForContext: function HS_deactivateHUDForContext(aContext, aAnimated)
  {
    let browser = aContext.linkedBrowser;
    let window = browser.contentWindow;
    let chromeDocument = aContext.ownerDocument;
    let nBox = chromeDocument.defaultView.getNotificationBox(window);
    let hudId = "hud_" + nBox.id;
    let displayNode = chromeDocument.getElementById(hudId);

    if (hudId in this.hudReferences && displayNode) {
      if (!aAnimated) {
        this.storeHeight(hudId);
      }

      let hud = this.hudReferences[hudId];
      browser.webProgress.removeProgressListener(hud.progressListener);
      delete hud.progressListener;

      this.unregisterDisplay(hudId);

      window.focus();
    }

    
    
    
    let procInstr = aContext.ownerDocument.gcliCssProcInstr;
    if (procInstr) {
      procInstr.contexts = procInstr.contexts.filter(function(id) {
        return id !== hudId;
      });
      if (procInstr.contexts.length == 0 && procInstr.parentNode) {
        procInstr.parentNode.removeChild(procInstr);
        delete aContext.ownerDocument.gcliCssProcInstr;
      }
    }
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

  






  regroupOutput: function HS_regroupOutput(aOutputNode)
  {
    
    

    let nodes = aOutputNode.querySelectorAll(".hud-msg-node" +
      ":not(.hud-filtered-by-string):not(.hud-filtered-by-type)");
    let lastTimestamp;
    for (let i = 0; i < nodes.length; i++) {
      let thisTimestamp = nodes[i].timestamp;
      if (lastTimestamp != null &&
          thisTimestamp >= lastTimestamp + NEW_GROUP_DELAY) {
        nodes[i].classList.add("webconsole-new-group");
      }
      else {
        nodes[i].classList.remove("webconsole-new-group");
      }
      lastTimestamp = thisTimestamp;
    }
  },

  













  adjustVisibilityForMessageType:
  function HS_adjustVisibilityForMessageType(aHUDId, aPrefKey, aState)
  {
    let outputNode = this.getHudReferenceById(aHUDId).outputNode;
    let doc = outputNode.ownerDocument;

    
    
    

    let xpath = ".//*[contains(@class, 'hud-msg-node') and " +
      "contains(concat(@class, ' '), 'hud-" + aPrefKey + " ')]";
    let result = doc.evaluate(xpath, outputNode, null,
      Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
    for (let i = 0; i < result.snapshotLength; i++) {
      let node = result.snapshotItem(i);
      if (aState) {
        node.classList.remove("hud-filtered-by-type");
      }
      else {
        node.classList.add("hud-filtered-by-type");
      }
    }

    this.regroupOutput(outputNode);
  },

  








  stringMatchesFilters: function stringMatchesFilters(aString, aFilter)
  {
    if (!aFilter || !aString) {
      return true;
    }

    let searchStr = aString.toLowerCase();
    let filterStrings = aFilter.toLowerCase().split(/\s+/);
    return !filterStrings.some(function (f) {
      return searchStr.indexOf(f) == -1;
    });
  },

  









  adjustVisibilityOnSearchStringChange:
  function HS_adjustVisibilityOnSearchStringChange(aHUDId, aSearchString)
  {
    let outputNode = this.getHudReferenceById(aHUDId).outputNode;

    let nodes = outputNode.querySelectorAll(".hud-msg-node");

    for (let i = 0; i < nodes.length; ++i) {
      let node = nodes[i];

      
      let text = node.clipboardText;

      
      if (this.stringMatchesFilters(text, aSearchString)) {
        node.classList.remove("hud-filtered-by-string");
      }
      else {
        node.classList.add("hud-filtered-by-string");
      }
    }

    this.regroupOutput(outputNode);
  },

  


  registerHUDReference:
  function HS_registerHUDReference(aHUD)
  {
    this.hudReferences[aHUD.hudId] = aHUD;

    let id = ConsoleUtils.supString(aHUD.hudId);
    Services.obs.notifyObservers(id, "web-console-created", null);
  },

  




  registerDisplay: function HS_registerDisplay(aHUDId)
  {
    
    if (!aHUDId){
      throw new Error(ERRORS.MISSING_ARGS);
    }
    this.filterPrefs[aHUDId] = this.defaultFilterPrefs;
    
    this.storage.createDisplay(aHUDId);
  },

  






  unregisterDisplay: function HS_unregisterDisplay(aHUDId)
  {
    let hud = this.getHudReferenceById(aHUDId);

    
    
    
    if (hud.jsterm) {
      hud.jsterm.clearOutput();
    }
    if (hud.gcliterm) {
      hud.gcliterm.clearOutput();
    }

    hud.destroy();

    
    
    hud.consoleWindowUnregisterOnHide = false;

    
    
    hud.HUDBox.parentNode.removeChild(hud.HUDBox);
    if (hud.consolePanel) {
      hud.consolePanel.parentNode.removeChild(hud.consolePanel);
    }

    if (hud.splitter.parentNode) {
      hud.splitter.parentNode.removeChild(hud.splitter);
    }

    if (hud.jsterm) {
      hud.jsterm.autocompletePopup.destroy();
    }

    delete this.hudReferences[aHUDId];

    
    this.storage.removeDisplay(aHUDId);

    for (let windowID in this.windowIds) {
      if (this.windowIds[windowID] == aHUDId) {
        delete this.windowIds[windowID];
      }
    }

    this.unregisterActiveContext(aHUDId);

    let popupset = hud.chromeDocument.getElementById("mainPopupSet");
    let panels = popupset.querySelectorAll("panel[hudId=" + aHUDId + "]");
    for (let i = 0; i < panels.length; i++) {
      panels[i].hidePopup();
    }
    panels = popupset.querySelectorAll("panel[hudToolId=" + aHUDId + "]");
    for (let i = 0; i < panels.length; i++) {
      panels[i].hidePopup();
    }

    let id = ConsoleUtils.supString(aHUDId);
    Services.obs.notifyObservers(id, "web-console-destroyed", null);

    if (Object.keys(this.hudReferences).length == 0) {
      let autocompletePopup = hud.chromeDocument.
                              getElementById("webConsole_autocompletePopup");
      if (autocompletePopup) {
        autocompletePopup.parentNode.removeChild(autocompletePopup);
      }

      this.suspend();
    }
  },

  





  wakeup: function HS_wakeup()
  {
    if (Object.keys(this.hudReferences).length > 0) {
      return;
    }

    this.storage = new ConsoleStorage();
    this.defaultFilterPrefs = this.storage.defaultDisplayPrefs;
    this.defaultGlobalConsolePrefs = this.storage.defaultGlobalConsolePrefs;

    
    this.startHTTPObservation();

    HUDWindowObserver.init();
    HUDConsoleObserver.init();
    ConsoleAPIObserver.init();
  },

  





  suspend: function HS_suspend()
  {
    activityDistributor.removeObserver(this.httpObserver);
    delete this.httpObserver;

    Services.obs.removeObserver(this.httpResponseExaminer,
                                "http-on-examine-response");

    this.openRequests = {};
    this.openResponseHeaders = {};

    
    delete this.storage;
    delete this.defaultFilterPrefs;
    delete this.defaultGlobalConsolePrefs;

    delete this.lastFinishedRequestCallback;

    HUDWindowObserver.uninit();
    HUDConsoleObserver.uninit();
    ConsoleAPIObserver.shutdown();
  },

  




  shutdown: function HS_shutdown()
  {
    for (let hudId in this.hudReferences) {
      this.deactivateHUDForContext(this.hudReferences[hudId].tab, false);
    }
  },

  





  getHudByWindow: function HS_getHudByWindow(aContentWindow)
  {
    let hudId = this.getHudIdByWindow(aContentWindow);
    return hudId ? this.hudReferences[hudId] : null;
  },

  






  getHudIdByWindow: function HS_getHudIdByWindow(aContentWindow)
  {
    let windowId = this.getWindowId(aContentWindow);
    return this.getHudIdByWindowId(windowId);
  },

  





  getHudReferenceById: function HS_getHudReferenceById(aId)
  {
    return aId in this.hudReferences ? this.hudReferences[aId] : null;
  },

  






  getHudIdByWindowId: function HS_getHudIdByWindowId(aWindowId)
  {
    return this.windowIds[aWindowId];
  },

  





  getFilterStringByHUDId: function HS_getFilterStringbyHUDId(aHUDId) {
    return this.getHudReferenceById(aHUDId).filterBox.value;
  },

  






  updateFilterText: function HS_updateFiltertext(aTextBoxNode)
  {
    var hudId = aTextBoxNode.getAttribute("hudId");
    this.adjustVisibilityOnSearchStringChange(hudId, aTextBoxNode.value);
  },

  









  logConsoleAPIMessage: function HS_logConsoleAPIMessage(aHUDId, aMessage)
  {
    
    let hud = HUDService.hudReferences[aHUDId];
    function formatResult(x) {
      return (typeof(x) == "string") ? x : hud.jsterm.formatResult(x);
    }

    let body = null;
    let clipboardText = null;
    let sourceURL = null;
    let sourceLine = 0;
    let level = aMessage.level;
    let args = aMessage.arguments;

    switch (level) {
      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
        let mappedArguments = Array.map(args, formatResult);
        body = Array.join(mappedArguments, " ");
        sourceURL = aMessage.filename;
        sourceLine = aMessage.lineNumber;
        break;

      case "trace":
        let filename = ConsoleUtils.abbreviateSourceURL(args[0].filename);
        let functionName = args[0].functionName ||
                           this.getStr("stacktrace.anonymousFunction");
        let lineNumber = args[0].lineNumber;

        body = this.getFormatStr("stacktrace.outputMessage",
                                 [filename, functionName, lineNumber]);

        sourceURL = args[0].filename;
        sourceLine = args[0].lineNumber;

        clipboardText = "";

        args.forEach(function(aFrame) {
          clipboardText += aFrame.filename + " :: " +
                           aFrame.functionName + " :: " +
                           aFrame.lineNumber + "\n";
        });

        clipboardText = clipboardText.trimRight();
        break;

      case "dir":
        body = unwrap(args[0]);
        clipboardText = body.toString();
        sourceURL = aMessage.filename;
        sourceLine = aMessage.lineNumber;
        break;

      case "group":
      case "groupCollapsed":
        clipboardText = body = formatResult(args);
        sourceURL = aMessage.filename;
        sourceLine = aMessage.lineNumber;
        hud.groupDepth++;
        break;

      case "groupEnd":
        if (hud.groupDepth > 0) {
          hud.groupDepth--;
        }
        return;

      default:
        Cu.reportError("Unknown Console API log level: " + level);
        return;
    }

    let node = ConsoleUtils.createMessageNode(hud.outputNode.ownerDocument,
                                              CATEGORY_WEBDEV,
                                              LEVELS[level],
                                              body,
                                              aHUDId,
                                              sourceURL,
                                              sourceLine,
                                              clipboardText,
                                              level);

    
    
    if (level == "trace") {
      node._stacktrace = args;

      let linkNode = node.querySelector(".webconsole-msg-body");
      linkNode.classList.add("hud-clickable");
      linkNode.setAttribute("aria-haspopup", "true");

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
          let propPanel = hud.jsterm.openPropertyPanel(null,
                                                       node._stacktrace,
                                                       this);
          propPanel.panel.setAttribute("hudId", aHUDId);
          this._panelOpen = true;
        }
      }, false);
    }

    ConsoleUtils.outputMessageNode(node, aHUDId);

    if (level == "dir") {
      
      
      
      let tree = node.querySelector("tree");
      tree.view = node.propertyTreeView;
    }
  },

  







  logWarningAboutReplacedAPI:
  function HS_logWarningAboutReplacedAPI(aHUDId)
  {
    let hud = this.hudReferences[aHUDId];
    let chromeDocument = hud.HUDBox.ownerDocument;
    let message = stringBundle.GetStringFromName("ConsoleAPIDisabled");
    let node = ConsoleUtils.createMessageNode(chromeDocument, CATEGORY_JS,
                                              SEVERITY_WARNING, message,
                                              aHUDId);
    ConsoleUtils.outputMessageNode(node, aHUDId);
  },

  








  reportPageError: function HS_reportPageError(aCategory, aScriptError)
  {
    if (aCategory != CATEGORY_CSS && aCategory != CATEGORY_JS) {
      throw Components.Exception("Unsupported category (must be one of CSS " +
                                 "or JS)", Cr.NS_ERROR_INVALID_ARG,
                                 Components.stack.caller);
    }

    
    
    let severity = SEVERITY_ERROR;
    if ((aScriptError.flags & aScriptError.warningFlag) ||
        (aScriptError.flags & aScriptError.strictFlag)) {
      severity = SEVERITY_WARNING;
    }

    let window = HUDService.getWindowByWindowId(aScriptError.outerWindowID);
    if (window) {
      let hudId = HUDService.getHudIdByWindow(window.top);
      if (hudId) {
        let outputNode = this.hudReferences[hudId].outputNode;
        let chromeDocument = outputNode.ownerDocument;

        let node = ConsoleUtils.createMessageNode(chromeDocument,
                                                  aCategory,
                                                  severity,
                                                  aScriptError.errorMessage,
                                                  hudId,
                                                  aScriptError.sourceName,
                                                  aScriptError.lineNumber);

        ConsoleUtils.outputMessageNode(node, hudId);
      }
    }
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

  



  lastFinishedRequestCallback: null,

  









  openNetworkPanel: function HS_openNetworkPanel(aNode, aHttpActivity)
  {
    let doc = aNode.ownerDocument;
    let parent = doc.getElementById("mainPopupSet");
    let netPanel = new NetworkPanel(parent, aHttpActivity);
    netPanel.linkNode = aNode;

    let panel = netPanel.panel;
    panel.openPopup(aNode, "after_pointer", 0, 0, false, false);
    panel.sizeTo(450, 500);
    panel.setAttribute("hudId", aHttpActivity.hudId);
    aHttpActivity.panels.push(Cu.getWeakReference(netPanel));
    return netPanel;
  },

  




  startHTTPObservation: function HS_httpObserverFactory()
  {
    
    var self = this;
    var httpObserver = {
      observeActivity :
      function HS_SHO_observeActivity(aChannel,
                                      aActivityType,
                                      aActivitySubtype,
                                      aTimestamp,
                                      aExtraSizeData,
                                      aExtraStringData)
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

            
            hudId = self.getHudIdByWindow(win.top);
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

            
            let loggedNode = self.logNetActivity(httpActivity);

            
            
            if (!loggedNode) {
              return;
            }

            aChannel.QueryInterface(Ci.nsITraceableChannel);

            
            
            
            
            let sink = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);

            
            
            sink.init(false, false, HUDService.responsePipeSegmentSize,
                      PR_UINT32_MAX, null);

            
            let newListener = new ResponseListener(httpActivity);

            
            newListener.inputStream = sink.inputStream;
            newListener.sink = sink;

            let tee = Cc["@mozilla.org/network/stream-listener-tee;1"].
                      createInstance(Ci.nsIStreamListenerTee);

            let originalListener = aChannel.setNewListener(tee);

            tee.init(originalListener, sink.outputStream, newListener);

            
            aChannel.visitRequestHeaders({
              visitHeader: function(aName, aValue) {
                httpActivity.request.header[aName] = aValue;
              }
            });

            
            let linkNode = loggedNode.querySelector(".webconsole-msg-link");

            httpActivity.messageObject = {
              messageNode: loggedNode,
              linkNode:    linkNode
            };
            self.openRequests[httpActivity.id] = httpActivity;

            
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
            for each (let item in self.openRequests) {
              if (item.channel !== aChannel) {
                continue;
              }
              httpActivity = item;
              break;
            }

            if (!httpActivity) {
              return;
            }

            hudId = httpActivity.hudId;
            let msgObject = httpActivity.messageObject;

            let updatePanel = false;
            let data;
            
            httpActivity.timing[transCodes[aActivitySubtype]] = aTimestamp;

            switch (aActivitySubtype) {
              case activityDistributor.ACTIVITY_SUBTYPE_REQUEST_BODY_SENT: {
                if (!self.saveRequestAndResponseBodies) {
                  httpActivity.request.bodyDiscarded = true;
                  break;
                }

                let gBrowser = msgObject.messageNode.ownerDocument.
                               defaultView.gBrowser;
                let HUD = HUDService.hudReferences[hudId];
                let browser = gBrowser.
                              getBrowserForDocument(HUD.contentDocument);

                let sentBody = NetworkHelper.
                               readPostTextFromRequest(aChannel, browser);
                if (!sentBody) {
                  
                  
                  
                  
                  
                  
                  
                  if (httpActivity.url == browser.contentWindow.location.href) {
                    sentBody = NetworkHelper.readPostTextFromPage(browser);
                  }
                  if (!sentBody) {
                    sentBody = "";
                  }
                }
                httpActivity.request.body = sentBody;
                break;
              }

              case activityDistributor.ACTIVITY_SUBTYPE_RESPONSE_HEADER: {
                
                
                
                
                
                
                
                
                httpActivity.response.status =
                  aExtraStringData.split(/\r\n|\n|\r/)[0];

                
                let linkNode = msgObject.linkNode;
                let statusNode = linkNode.
                  querySelector(".webconsole-msg-status");
                let statusText = "[" + httpActivity.response.status + "]";
                statusNode.setAttribute("value", statusText);

                let clipboardTextPieces =
                  [ httpActivity.method, httpActivity.url, statusText ];
                msgObject.messageNode.clipboardText =
                  clipboardTextPieces.join(" ");

                let status = parseInt(httpActivity.response.status.
                  replace(/^HTTP\/\d\.\d (\d+).+$/, "$1"));

                if (status >= MIN_HTTP_ERROR_CODE &&
                    status < MAX_HTTP_ERROR_CODE) {
                  ConsoleUtils.setMessageType(msgObject.messageNode,
                                              CATEGORY_NETWORK,
                                              SEVERITY_ERROR);
                }

                break;
              }

              case activityDistributor.ACTIVITY_SUBTYPE_TRANSACTION_CLOSE: {
                let timing = httpActivity.timing;
                let requestDuration =
                  Math.round((timing.RESPONSE_COMPLETE -
                                timing.REQUEST_HEADER) / 1000);

                
                let linkNode = msgObject.linkNode;
                let statusNode = linkNode.
                  querySelector(".webconsole-msg-status");

                let statusText = httpActivity.response.status;
                let timeText = self.getFormatStr("NetworkPanel.durationMS",
                                                 [ requestDuration ]);
                let fullStatusText = "[" + statusText + " " + timeText + "]";
                statusNode.setAttribute("value", fullStatusText);

                let clipboardTextPieces =
                  [ httpActivity.method, httpActivity.url, fullStatusText ];
                msgObject.messageNode.clipboardText =
                  clipboardTextPieces.join(" ");

                delete httpActivity.messageObject;
                delete self.openRequests[httpActivity.id];
                updatePanel = true;
                break;
              }
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
        0x804b000b: "STATUS_RESOLVED",
        0x804b0007: "STATUS_CONNECTING_TO",
        0x804b0004: "STATUS_CONNECTED_TO",
        0x804b0005: "STATUS_SENDING_TO",
        0x804b000a: "STATUS_WAITING_FOR",
        0x804b0006: "STATUS_RECEIVING_FROM"
      }
    };

    this.httpObserver = httpObserver;

    activityDistributor.addObserver(httpObserver);

    
    Services.obs.addObserver(this.httpResponseExaminer,
                             "http-on-examine-response", false);
  },

  







  httpResponseExaminer: function HS_httpResponseExaminer(aSubject, aTopic)
  {
    if (aTopic != "http-on-examine-response" ||
        !(aSubject instanceof Ci.nsIHttpChannel)) {
      return;
    }

    let channel = aSubject.QueryInterface(Ci.nsIHttpChannel);
    let win = NetworkHelper.getWindowForRequest(channel);
    if (!win) {
      return;
    }
    let hudId = HUDService.getHudIdByWindow(win);
    if (!hudId) {
      return;
    }

    let response = {
      id: HUDService.sequenceId(),
      hudId: hudId,
      channel: channel,
      headers: {},
    };

    try {
      channel.visitResponseHeaders({
        visitHeader: function(aName, aValue) {
          response.headers[aName] = aValue;
        }
      });
    }
    catch (ex) {
      delete response.headers;
    }

    if (response.headers) {
      HUDService.openResponseHeaders[response.id] = response;
    }
  },

  






  logNetActivity: function HS_logNetActivity(aActivityObject)
  {
    let hudId = aActivityObject.hudId;
    let outputNode = this.hudReferences[hudId].outputNode;

    let chromeDocument = outputNode.ownerDocument;
    let msgNode = chromeDocument.createElementNS(XUL_NS, "hbox");

    let methodNode = chromeDocument.createElementNS(XUL_NS, "label");
    methodNode.setAttribute("value", aActivityObject.method);
    methodNode.classList.add("webconsole-msg-body-piece");
    msgNode.appendChild(methodNode);

    let linkNode = chromeDocument.createElementNS(XUL_NS, "hbox");
    linkNode.setAttribute("flex", "1");
    linkNode.classList.add("webconsole-msg-body-piece");
    linkNode.classList.add("webconsole-msg-link");
    msgNode.appendChild(linkNode);

    let urlNode = chromeDocument.createElementNS(XUL_NS, "label");
    urlNode.setAttribute("crop", "center");
    urlNode.setAttribute("flex", "1");
    urlNode.setAttribute("title", aActivityObject.url);
    urlNode.setAttribute("value", aActivityObject.url);
    urlNode.classList.add("hud-clickable");
    urlNode.classList.add("webconsole-msg-body-piece");
    urlNode.classList.add("webconsole-msg-url");
    linkNode.appendChild(urlNode);

    let statusNode = chromeDocument.createElementNS(XUL_NS, "label");
    statusNode.setAttribute("value", "");
    statusNode.classList.add("hud-clickable");
    statusNode.classList.add("webconsole-msg-body-piece");
    statusNode.classList.add("webconsole-msg-status");
    linkNode.appendChild(statusNode);

    let clipboardText = aActivityObject.method + " " + aActivityObject.url;

    let messageNode = ConsoleUtils.createMessageNode(chromeDocument,
                                                     CATEGORY_NETWORK,
                                                     SEVERITY_LOG,
                                                     msgNode,
                                                     hudId,
                                                     null,
                                                     null,
                                                     clipboardText);

    ConsoleUtils.outputMessageNode(messageNode, aActivityObject.hudId);
    return messageNode;
  },

  









  initializeJSTerm: function HS_initializeJSTerm(aContext, aParentNode, aConsole)
  {
    
    var context = Cu.getWeakReference(aContext);

    
    var firefoxMixin = new JSTermFirefoxMixin(context, aParentNode);
    var jsTerm = new JSTerm(context, aParentNode, firefoxMixin, aConsole);

    
    
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
    4: "error", 
    5: "warn", 
    8: "error", 
    13: "warn", 
  },

  


  scriptMsgLogLevel: {
    0: "typeError", 
    1: "typeWarning", 
    2: "typeException", 
    4: "typeError", 
    5: "typeStrict", 
    8: "typeError", 
    13: "typeWarning", 
  },

  





  onTabClose: function HS_onTabClose(aEvent)
  {
    this.deactivateHUDForContext(aEvent.target, false);
  },

  







  onWindowUnload: function HS_onWindowUnload(aEvent)
  {
    let window = aEvent.target.defaultView;

    window.removeEventListener("unload", this.onWindowUnload, false);

    let gBrowser = window.gBrowser;
    let tabContainer = gBrowser.tabContainer;

    tabContainer.removeEventListener("TabClose", this.onTabClose, false);

    let tab = tabContainer.firstChild;
    while (tab != null) {
      this.deactivateHUDForContext(tab, false);
      tab = tab.nextSibling;
    }

    if (window.webConsoleCommandController) {
      window.controllers.removeController(window.webConsoleCommandController);
      window.webConsoleCommandController = null;
    }
  },

  





  windowInitializer: function HS_WindowInitalizer(aContentWindow)
  {
    var xulWindow = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
                      .QueryInterface(Ci.nsIDocShell)
                      .chromeEventHandler.ownerDocument.defaultView;

    let xulWindow = unwrap(xulWindow);

    let docElem = xulWindow.document.documentElement;
    if (!docElem || docElem.getAttribute("windowtype") != "navigator:browser" ||
        !xulWindow.gBrowser) {
      
      
      return;
    }

    let gBrowser = xulWindow.gBrowser;

    let _browser = gBrowser.
      getBrowserForDocument(aContentWindow.top.document);

    
    if (!_browser) {
      return;
    }

    let nBox = gBrowser.getNotificationBox(_browser);
    let nBoxId = nBox.getAttribute("id");
    let hudId = "hud_" + nBoxId;
    let windowUI = nBox.ownerDocument.getElementById("console_window_" + hudId);
    if (windowUI) {
      
      if (aContentWindow == aContentWindow.top) {
        let hud = this.hudReferences[hudId];
        hud.reattachConsole(aContentWindow);
      }
      return;
    }

    if (!this.canActivateContext(hudId)) {
      return;
    }

    xulWindow.addEventListener("unload", this.onWindowUnload, false);
    gBrowser.tabContainer.addEventListener("TabClose", this.onTabClose, false);

    this.registerDisplay(hudId);

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
                     contentWindow: aContentWindow
                   };

      hud = new HeadsUpDisplay(config);

      HUDService.registerHUDReference(hud);
      let windowId = this.getWindowId(aContentWindow.top);
      this.windowIds[windowId] = hudId;

      hud.progressListener = new ConsoleProgressListener(hudId);

      _browser.webProgress.addProgressListener(hud.progressListener,
        Ci.nsIWebProgress.NOTIFY_STATE_ALL);
    }
    else {
      hud = this.hudReferences[hudId];
      if (aContentWindow == aContentWindow.top) {
        
        hud.reattachConsole(aContentWindow);
      }
    }

    
    let consoleObject = unwrap(aContentWindow).console;
    if (!("__mozillaConsole__" in consoleObject))
      this.logWarningAboutReplacedAPI(hudId);

    
    this.createController(xulWindow);
  },

  






  createController: function HUD_createController(aWindow)
  {
    if (aWindow.webConsoleCommandController == null) {
      aWindow.webConsoleCommandController = new CommandController(aWindow);
      aWindow.controllers.insertControllerAt(0,
        aWindow.webConsoleCommandController);
    }
  },

  









  animate: function HS_animate(aHUDId, aDirection, aCallback)
  {
    let hudBox = this.getHudReferenceById(aHUDId).HUDBox;
    if (!hudBox.classList.contains("animated")) {
      if (aCallback) {
        aCallback();
      }
      return;
    }

    switch (aDirection) {
      case ANIMATE_OUT:
        hudBox.style.height = 0;
        break;
      case ANIMATE_IN:
        this.resetHeight(aHUDId);
        break;
    }

    if (aCallback) {
      hudBox.addEventListener("transitionend", aCallback, false);
    }
  },

  






  disableAnimation: function HS_disableAnimation(aHUDId)
  {
    let hudBox = HUDService.hudReferences[aHUDId].HUDBox;
    if (hudBox.classList.contains("animated")) {
      hudBox.classList.remove("animated");
      this.resetHeight(aHUDId);
    }
  },

  




  resetHeight: function HS_resetHeight(aHUDId)
  {
    let HUD = this.hudReferences[aHUDId];
    let innerHeight = HUD.contentWindow.innerHeight;
    let chromeWindow = HUD.chromeDocument.defaultView;
    if (!HUD.consolePanel) {
      let splitterStyle = chromeWindow.getComputedStyle(HUD.splitter, null);
      innerHeight += parseInt(splitterStyle.height) +
                     parseInt(splitterStyle.borderTopWidth) +
                     parseInt(splitterStyle.borderBottomWidth);
    }

    let boxStyle = chromeWindow.getComputedStyle(HUD.HUDBox, null);
    innerHeight += parseInt(boxStyle.height) +
                   parseInt(boxStyle.borderTopWidth) +
                   parseInt(boxStyle.borderBottomWidth);

    let height = this.lastConsoleHeight > 0 ? this.lastConsoleHeight :
      Math.ceil(innerHeight * DEFAULT_CONSOLE_HEIGHT);

    if ((innerHeight - height) < MINIMUM_PAGE_HEIGHT) {
      height = innerHeight - MINIMUM_PAGE_HEIGHT;
    }
    else if (height < MINIMUM_CONSOLE_HEIGHT) {
      height = MINIMUM_CONSOLE_HEIGHT;
    }

    HUD.HUDBox.style.height = height + "px";
  },

  





  storeHeight: function HS_storeHeight(aHUDId)
  {
    let hudBox = this.hudReferences[aHUDId].HUDBox;
    let window = hudBox.ownerDocument.defaultView;
    let style = window.getComputedStyle(hudBox, null);
    let height = parseInt(style.height);
    height += parseInt(style.borderTopWidth);
    height += parseInt(style.borderBottomWidth);
    this.lastConsoleHeight = height;

    let pref = Services.prefs.getIntPref("devtools.hud.height");
    if (pref > -1) {
      Services.prefs.setIntPref("devtools.hud.height", height);
    }
  },

  






  copySelectedItems: function HS_copySelectedItems(aOutputNode)
  {
    

    let strings = [];
    let newGroup = false;

    let children = aOutputNode.children;

    for (let i = 0; i < children.length; i++) {
      let item = children[i];
      if (!item.selected) {
        continue;
      }

      
      
      if (i > 0 && item.classList.contains("webconsole-new-group")) {
        newGroup = true;
      }

      
      if (!item.classList.contains("hud-filtered-by-type") &&
          !item.classList.contains("hud-filtered-by-string")) {
        let timestampString = ConsoleUtils.timestampString(item.timestamp);
        if (newGroup) {
          strings.push("--");
          newGroup = false;
        }
        strings.push("[" + timestampString + "] " + item.clipboardText);
      }
    }
    clipboardHelper.copyString(strings.join("\n"));
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
    this.notificationBox = parentNode;
  }

  
  this.textFactory = NodeFactory("text", "xul", this.chromeDocument);

  this.chromeWindow = this.chromeDocument.defaultView;

  
  this.createHUD();

  this.HUDBox.lastTimestamp = 0;
  
  try {
    this.createConsoleInput(this.contentWindow, this.consoleWrap, this.outputNode);
    if (this.jsterm) {
      this.jsterm.inputNode.focus();
    }
    if (this.gcliterm) {
      this.gcliterm.inputNode.focus();
    }
  }
  catch (ex) {
    Cu.reportError(ex);
  }

  
  this.cssNodes = {};
}

HeadsUpDisplay.prototype = {

  consolePanel: null,

  


  groupDepth: 0,

  get mainPopupSet()
  {
    return this.chromeDocument.getElementById("mainPopupSet");
  },

  


  get tab()
  {
    
    
    
    
    let tab = null;
    let id = this.notificationBox.id;
    Array.some(this.chromeDocument.defaultView.gBrowser.tabs, function(aTab) {
      if (aTab.linkedPanel == id) {
        tab = aTab;
        return true;
      }
    });

    return tab;
  },

  



  createOwnWindowPanel: function HUD_createOwnWindowPanel()
  {
    if (this.uiInOwnWindow) {
      return this.consolePanel;
    }

    let width = 0;
    try {
      width = Services.prefs.getIntPref("devtools.webconsole.width");
    }
    catch (ex) {}

    if (width < 1) {
      width = this.HUDBox.clientWidth || this.contentWindow.innerWidth;
    }

    let height = this.HUDBox.clientHeight;

    let top = 0;
    try {
      top = Services.prefs.getIntPref("devtools.webconsole.top");
    }
    catch (ex) {}

    let left = 0;
    try {
      left = Services.prefs.getIntPref("devtools.webconsole.left");
    }
    catch (ex) {}

    let panel = this.chromeDocument.createElementNS(XUL_NS, "panel");

    let config = { id: "console_window_" + this.hudId,
                   label: this.getPanelTitle(),
                   titlebar: "normal",
                   noautohide: "true",
                   norestorefocus: "true",
                   close: "true",
                   flex: "1",
                   hudId: this.hudId,
                   width: width,
                   position: "overlap",
                   top: top,
                   left: left,
                 };

    for (let attr in config) {
      panel.setAttribute(attr, config[attr]);
    }

    panel.classList.add("web-console-panel");

    let onPopupShown = (function HUD_onPopupShown() {
      panel.removeEventListener("popupshown", onPopupShown, false);

      

      let height = panel.clientHeight;

      this.HUDBox.style.height = "auto";
      this.HUDBox.setAttribute("flex", "1");

      panel.setAttribute("height", height);

      
      if (lastIndex > -1 && lastIndex < this.outputNode.getRowCount()) {
        this.outputNode.ensureIndexIsVisible(lastIndex);
      }

      if (this.jsterm) {
        this.jsterm.inputNode.focus();
      }
      if (this.gcliterm) {
        this.gcliterm.inputNode.focus();
      }
    }).bind(this);

    panel.addEventListener("popupshown", onPopupShown,false);

    let onPopupHidden = (function HUD_onPopupHidden(aEvent) {
      if (aEvent.target != panel) {
        return;
      }

      panel.removeEventListener("popuphidden", onPopupHidden, false);
      if (panel.parentNode) {
        panel.parentNode.removeChild(panel);
      }

      let width = 0;
      try {
        width = Services.prefs.getIntPref("devtools.webconsole.width");
      }
      catch (ex) { }

      if (width > 0) {
        Services.prefs.setIntPref("devtools.webconsole.width", panel.clientWidth);
      }

      





      
      
      panel.removeAttribute("hudId");

      if (this.consoleWindowUnregisterOnHide) {
        HUDService.deactivateHUDForContext(this.tab, false);
      }
      else {
        this.consoleWindowUnregisterOnHide = true;
      }

      this.consolePanel = null;
    }).bind(this);

    panel.addEventListener("popuphidden", onPopupHidden, false);

    let lastIndex = -1;

    if (this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }
    panel.appendChild(this.HUDBox);

    let space = this.chromeDocument.createElement("spacer");
    space.setAttribute("flex", "1");

    let bottomBox = this.chromeDocument.createElement("hbox");
    space.setAttribute("flex", "1");

    let resizer = this.chromeDocument.createElement("resizer");
    resizer.setAttribute("dir", "bottomend");
    resizer.setAttribute("element", config.id);

    bottomBox.appendChild(space);
    bottomBox.appendChild(resizer);

    panel.appendChild(bottomBox);

    this.mainPopupSet.appendChild(panel);

    Services.prefs.setCharPref("devtools.webconsole.position", "window");

    panel.openPopup(null, "overlay", left, top, false, false);

    this.consolePanel = panel;
    this.consoleWindowUnregisterOnHide = true;

    return panel;
  },

  





  getPanelTitle: function HUD_getPanelTitle()
  {
    return this.getFormatStr("webConsoleWindowTitleAndURL", [this.uriSpec]);
  },

  positions: {
    above: 0, 
    below: 2,
    window: null
  },

  consoleWindowUnregisterOnHide: true,

  


  positionConsole: function HUD_positionConsole(aPosition)
  {
    if (!(aPosition in this.positions)) {
      throw new Error("Incorrect argument: " + aPosition +
        ". Cannot position Web Console");
    }

    if (aPosition == "window") {
      let closeButton = this.consoleFilterToolbar.
        querySelector(".webconsole-close-button");
      closeButton.setAttribute("hidden", "true");
      this.createOwnWindowPanel();
      this.positionMenuitems.window.setAttribute("checked", true);
      if (this.positionMenuitems.last) {
        this.positionMenuitems.last.setAttribute("checked", false);
      }
      this.positionMenuitems.last = this.positionMenuitems[aPosition];
      this.uiInOwnWindow = true;
      return;
    }

    let height = this.HUDBox.clientHeight;

    
    let nodeIdx = this.positions[aPosition];
    let nBox = this.notificationBox;
    let node = nBox.childNodes[nodeIdx];

    
    if (node == this.HUDBox) {
      return;
    }

    let lastIndex = -1;

    if (this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    
    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }

    if (aPosition == "below") {
      nBox.appendChild(this.splitter);
      nBox.appendChild(this.HUDBox);
    }
    else {
      nBox.insertBefore(this.splitter, node);
      nBox.insertBefore(this.HUDBox, this.splitter);
    }

    this.positionMenuitems[aPosition].setAttribute("checked", true);
    if (this.positionMenuitems.last) {
      this.positionMenuitems.last.setAttribute("checked", false);
    }
    this.positionMenuitems.last = this.positionMenuitems[aPosition];

    Services.prefs.setCharPref("devtools.webconsole.position", aPosition);

    if (lastIndex > -1 && lastIndex < this.outputNode.getRowCount()) {
      this.outputNode.ensureIndexIsVisible(lastIndex);
    }

    let closeButton = this.consoleFilterToolbar.
      getElementsByClassName("webconsole-close-button")[0];
    closeButton.removeAttribute("hidden");

    this.uiInOwnWindow = false;
    if (this.consolePanel) {
      this.HUDBox.removeAttribute("flex");
      this.HUDBox.removeAttribute("height");
      this.HUDBox.style.height = height + "px";

      
      this.consoleWindowUnregisterOnHide = false;
      this.consolePanel.hidePopup();
    }

    if (this.jsterm) {
      this.jsterm.inputNode.focus();
    }
    if (this.gcliterm) {
      this.gcliterm.inputNode.focus();
    }
  },

  





  getStr: function HUD_getStr(aName)
  {
    return stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function HUD_getFormatStr(aName, aArray)
  {
    return stringBundle.formatStringFromName(aName, aArray, aArray.length);
  },

  



  jsterm: null,

  


  gcliterm: null,

  





  createConsoleInput:
  function HUD_createConsoleInput(aWindow, aParentNode, aExistingConsole)
  {
    let usegcli = false;
    try {
      usegcli = Services.prefs.getBoolPref("devtools.gcli.enable");
    }
    catch (ex) {}

    if (appName() == "FIREFOX") {
      if (!usegcli) {
        let context = Cu.getWeakReference(aWindow);
        let mixin = new JSTermFirefoxMixin(context, aParentNode,
                                           aExistingConsole);
        this.jsterm = new JSTerm(context, aParentNode, mixin, this.console);
      }
      else {
        this.gcliterm = new GcliTerm(aWindow, this.hudId, this.chromeDocument,
                                     this.console, this.hintNode);
        aParentNode.appendChild(this.gcliterm.element);
      }
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

    if (this.consolePanel) {
      this.consolePanel.label = this.getPanelTitle();
    }

    if (this.jsterm) {
      this.jsterm.context = Cu.getWeakReference(this.contentWindow);
      this.jsterm.console = this.console;
      this.jsterm.createSandbox();
    }
    else if (this.gcliterm) {
      this.gcliterm.reattachConsole(this.contentWindow, this.console);
    }
    else {
      this.createConsoleInput(this.contentWindow, this.consoleWrap, this.outputNode);
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

    this.splitter = this.makeXULNode("splitter");
    this.splitter.setAttribute("class", "hud-splitter");

    this.HUDBox = this.makeXULNode("vbox");
    this.HUDBox.setAttribute("id", this.hudId);
    this.HUDBox.setAttribute("class", "hud-box animated");
    this.HUDBox.style.height = 0;

    let outerWrap = this.makeXULNode("vbox");
    outerWrap.setAttribute("class", "hud-outer-wrapper");
    outerWrap.setAttribute("flex", "1");

    let consoleCommandSet = this.makeXULNode("commandset");
    outerWrap.appendChild(consoleCommandSet);

    let consoleWrap = this.makeXULNode("vbox");
    this.consoleWrap = consoleWrap;
    consoleWrap.setAttribute("class", "hud-console-wrapper");
    consoleWrap.setAttribute("flex", "1");

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

    let hintSpacerNode = this.makeXULNode("box");
    hintSpacerNode.setAttribute("flex", 1);

    this.hintNode = this.makeXULNode("div");
    this.hintNode.setAttribute("class", "gcliterm-hint-node");

    let hintParentNode = this.makeXULNode("vbox");
    hintParentNode.setAttribute("flex", "0");
    hintParentNode.setAttribute("class", "gcliterm-hint-parent");
    hintParentNode.appendChild(hintSpacerNode);
    hintParentNode.appendChild(this.hintNode);
    hintParentNode.hidden = true;

    let hbox = this.makeXULNode("hbox");
    hbox.setAttribute("flex", "1");

    this.outputNode = this.makeXULNode("richlistbox");
    this.outputNode.setAttribute("class", "hud-output-node");
    this.outputNode.setAttribute("flex", "1");
    this.outputNode.setAttribute("orient", "vertical");
    this.outputNode.setAttribute("context", this.hudId + "-output-contextmenu");
    this.outputNode.setAttribute("style", "direction: ltr;");
    this.outputNode.setAttribute("seltype", "multiple");

    hbox.appendChild(hintParentNode);
    hbox.appendChild(this.outputNode);

    consoleWrap.appendChild(consoleFilterToolbar);
    consoleWrap.appendChild(hbox);

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
    const BUTTONS = [
      {
        name: "PageNet",
        category: "net",
        severities: [
          { name: "ConsoleErrors", prefKey: "network" },
          { name: "ConsoleLog", prefKey: "networkinfo" }
        ]
      },
      {
        name: "PageCSS",
        category: "css",
        severities: [
          { name: "ConsoleErrors", prefKey: "csserror" },
          { name: "ConsoleWarnings", prefKey: "cssparser" }
        ]
      },
      {
        name: "PageJS",
        category: "js",
        severities: [
          { name: "ConsoleErrors", prefKey: "exception" },
          { name: "ConsoleWarnings", prefKey: "jswarn" }
        ]
      },
      {
        name: "PageWebDeveloper",
        category: "webdev",
        severities: [
          { name: "ConsoleErrors", prefKey: "error" },
          { name: "ConsoleWarnings", prefKey: "warn" },
          { name: "ConsoleInfo", prefKey: "info" },
          { name: "ConsoleLog", prefKey: "log" }
        ]
      }
    ];

    let toolbar = this.makeXULNode("toolbar");
    toolbar.setAttribute("class", "hud-console-filter-toolbar");
    toolbar.setAttribute("mode", "full");

#ifdef XP_MACOSX
    this.makeCloseButton(toolbar);
#endif

    for (let i = 0; i < BUTTONS.length; i++) {
      this.makeFilterButton(toolbar, BUTTONS[i]);
    }

    toolbar.appendChild(this.filterSpacer);

    let positionUI = this.createPositionUI();
    toolbar.appendChild(positionUI);

    toolbar.appendChild(this.filterBox);
    this.makeClearConsoleButton(toolbar);

#ifndef XP_MACOSX
    this.makeCloseButton(toolbar);
#endif

    return toolbar;
  },

  






  createPositionUI: function HUD_createPositionUI()
  {
    this._positionConsoleAbove = (function HUD_positionAbove() {
      this.positionConsole("above");
    }).bind(this);

    this._positionConsoleBelow = (function HUD_positionBelow() {
      this.positionConsole("below");
    }).bind(this);
    this._positionConsoleWindow = (function HUD_positionWindow() {
      this.positionConsole("window");
    }).bind(this);

    let button = this.makeXULNode("toolbarbutton");
    button.setAttribute("type", "menu");
    button.setAttribute("label", this.getStr("webConsolePosition"));
    button.setAttribute("tooltip", this.getStr("webConsolePositionTooltip"));

    let menuPopup = this.makeXULNode("menupopup");
    button.appendChild(menuPopup);

    let itemAbove = this.makeXULNode("menuitem");
    itemAbove.setAttribute("label", this.getStr("webConsolePositionAbove"));
    itemAbove.setAttribute("type", "checkbox");
    itemAbove.setAttribute("autocheck", "false");
    itemAbove.addEventListener("command", this._positionConsoleAbove, false);
    menuPopup.appendChild(itemAbove);

    let itemBelow = this.makeXULNode("menuitem");
    itemBelow.setAttribute("label", this.getStr("webConsolePositionBelow"));
    itemBelow.setAttribute("type", "checkbox");
    itemBelow.setAttribute("autocheck", "false");
    itemBelow.addEventListener("command", this._positionConsoleBelow, false);
    menuPopup.appendChild(itemBelow);

    let itemWindow = this.makeXULNode("menuitem");
    itemWindow.setAttribute("label", this.getStr("webConsolePositionWindow"));
    itemWindow.setAttribute("type", "checkbox");
    itemWindow.setAttribute("autocheck", "false");
    itemWindow.addEventListener("command", this._positionConsoleWindow, false);
    menuPopup.appendChild(itemWindow);

    this.positionMenuitems = {
      last: null,
      above: itemAbove,
      below: itemBelow,
      window: itemWindow,
    };

    return button;
  },

  






  createConsoleMenu: function HUD_createConsoleMenu(aConsoleWrapper) {
    let menuPopup = this.makeXULNode("menupopup");
    let id = this.hudId + "-output-contextmenu";
    menuPopup.setAttribute("id", id);
    menuPopup.addEventListener("popupshowing", function() {
      saveBodiesItem.setAttribute("checked",
        HUDService.saveRequestAndResponseBodies);
    }, true);

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
    copyItem.setAttribute("buttonType", "copy");
    menuPopup.appendChild(copyItem);

    let selectAllItem = this.makeXULNode("menuitem");
    selectAllItem.setAttribute("label", this.getStr("selectAllCmd.label"));
    selectAllItem.setAttribute("accesskey",
                               this.getStr("selectAllCmd.accesskey"));
    selectAllItem.setAttribute("hudId", this.hudId);
    selectAllItem.setAttribute("buttonType", "selectAll");
    selectAllItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    menuPopup.appendChild(selectAllItem);

    aConsoleWrapper.appendChild(menuPopup);
    aConsoleWrapper.setAttribute("context", id);
  },

  










  makeFilterButton: function HUD_makeFilterButton(aParent, aDescriptor)
  {
    let toolbarButton = this.makeXULNode("toolbarbutton");
    aParent.appendChild(toolbarButton);

    let toggleFilter = HeadsUpDisplayUICommands.toggleFilter;
    toolbarButton.addEventListener("click", toggleFilter, false);

    let name = aDescriptor.name;
    toolbarButton.setAttribute("type", "menu-button");
    toolbarButton.setAttribute("label", this.getStr("btn" + name));
    toolbarButton.setAttribute("tooltip", this.getStr("tip" + name));
    toolbarButton.setAttribute("category", aDescriptor.category);
    toolbarButton.setAttribute("hudId", this.hudId);
    toolbarButton.classList.add("webconsole-filter-button");

    let menuPopup = this.makeXULNode("menupopup");
    toolbarButton.appendChild(menuPopup);

    let allChecked = true;
    for (let i = 0; i < aDescriptor.severities.length; i++) {
      let severity = aDescriptor.severities[i];
      let menuItem = this.makeXULNode("menuitem");
      menuItem.setAttribute("label", this.getStr("btn" + severity.name));
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("autocheck", "false");
      menuItem.setAttribute("hudId", this.hudId);

      let prefKey = severity.prefKey;
      menuItem.setAttribute("prefKey", prefKey);

      let checked = this.filterPrefs[prefKey];
      menuItem.setAttribute("checked", checked);
      if (!checked) {
        allChecked = false;
      }

      menuItem.addEventListener("command", toggleFilter, false);

      menuPopup.appendChild(menuItem);
    }

    toolbarButton.setAttribute("checked", allChecked);
  },

  






  makeCloseButton: function HUD_makeCloseButton(aToolbar)
  {
    this.closeButtonOnCommand = (function HUD_closeButton_onCommand() {
      HUDService.animate(this.hudId, ANIMATE_OUT, (function() {
        HUDService.deactivateHUDForContext(this.tab, true);
      }).bind(this));
    }).bind(this);

    this.closeButton = this.makeXULNode("toolbarbutton");
    this.closeButton.classList.add("webconsole-close-button");
    this.closeButton.addEventListener("command",
      this.closeButtonOnCommand, false);
    aToolbar.appendChild(this.closeButton);
  },

  








  makeClearConsoleButton: function HUD_makeClearConsoleButton(aToolbar)
  {
    let hudId = this.hudId;
    function HUD_clearButton_onCommand() {
      let hud = HUDService.getHudReferenceById(hudId);
      if (hud.jsterm) {
        hud.jsterm.clearOutput();
      }
      if (hud.gcliterm) {
        hud.gcliterm.clearOutput();
      }
    }

    let clearButton = this.makeXULNode("toolbarbutton");
    clearButton.setAttribute("label", this.getStr("btnClear"));
    clearButton.classList.add("webconsole-clear-console-button");
    clearButton.addEventListener("command", HUD_clearButton_onCommand, false);

    aToolbar.appendChild(clearButton);
  },

  







  pruneConsoleDirNode: function HUD_pruneConsoleDirNode(aMessageNode)
  {
    aMessageNode.parentNode.removeChild(aMessageNode);
    let tree = aMessageNode.querySelector("tree");
    tree.parentNode.removeChild(tree);
    aMessageNode.propertyTreeView = null;
    tree.view = null;
    tree = null;
  },

  





  createHUD: function HUD_createHUD()
  {
    if (!this.HUDBox) {
      this.makeHUDNodes();
      let positionPref = Services.prefs.getCharPref("devtools.webconsole.position");
      this.positionConsole(positionPref);
    }
    return this.HUDBox;
  },

  uiInOwnWindow: false,

  get console() { return this.contentWindow.wrappedJSObject.console; },

  getLogCount: function HUD_getLogCount()
  {
    return this.outputNode.childNodes.length;
  },

  getLogNodes: function HUD_getLogNodes()
  {
    return this.outputNode.childNodes;
  },

  ERRORS: {
    HUD_BOX_DOES_NOT_EXIST: "Heads Up Display does not exist",
    TAB_ID_REQUIRED: "Tab DOM ID is required",
    PARENTNODE_NOT_FOUND: "parentNode element not found"
  },

  



  destroy: function HUD_destroy()
  {
    if (this.jsterm) {
      this.jsterm.destroy();
    }
    if (this.gcliterm) {
      this.gcliterm.destroy();
    }

    this.positionMenuitems.above.removeEventListener("command",
      this._positionConsoleAbove, false);
    this.positionMenuitems.below.removeEventListener("command",
      this._positionConsoleBelow, false);
    this.positionMenuitems.window.removeEventListener("command",
      this._positionConsoleWindow, false);

    this.closeButton.removeEventListener("command",
      this.closeButtonOnCommand, false);
  },
};






let ConsoleAPIObserver = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  init: function CAO_init()
  {
    Services.obs.addObserver(this, "quit-application-granted", false);
    Services.obs.addObserver(this, "console-api-log-event", false);
  },

  observe: function CAO_observe(aMessage, aTopic, aData)
  {
    if (aTopic == "console-api-log-event") {
      aMessage = aMessage.wrappedJSObject;
      let windowId = parseInt(aData);
      let win = HUDService.getWindowByWindowId(windowId);
      if (!win)
        return;

      
      let hudId = HUDService.getHudIdByWindow(win.top);
      if (!hudId)
        return;

      HUDService.logConsoleAPIMessage(hudId, aMessage);
    }
    else if (aTopic == "quit-application-granted") {
      HUDService.shutdown();
    }
  },

  shutdown: function CAO_shutdown()
  {
    Services.obs.removeObserver(this, "quit-application-granted");
    Services.obs.removeObserver(this, "console-api-log-event");
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
          if (!last || OPEN_CLOSE_BODY[last.token] != c) {
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
  let obj = unwrap(aScope);

  
  
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
    matchProp = properties.pop().trimLeft();
    for (let i = 0; i < properties.length; i++) {
      let prop = properties[i].trim();

      
      
      if (typeof obj === "undefined" || obj === null) {
        return null;
      }

      
      
      if (isNonNativeGetter(obj, prop)) {
        return null;
      }
      try {
        obj = obj[prop];
      }
      catch (ex) {
        return null;
      }
    }
  }
  else {
    matchProp = properties[0].trimLeft();
  }

  
  
  if (typeof obj === "undefined" || obj === null) {
    return null;
  }

  
  if (isIteratorOrGenerator(obj)) {
    return null;
  }

  let matches = [];
  for (let prop in obj) {
    if (prop.indexOf(matchProp) == 0) {
      matches.push(prop);
    }
  }

  return {
    matchProp: matchProp,
    matches: matches.sort(),
  };
}

function isIteratorOrGenerator(aObject)
{
  if (aObject === null) {
    return false;
  }

  if (typeof aObject == "object") {
    if (typeof aObject.__iterator__ == "function" ||
        aObject.constructor && aObject.constructor.name == "Iterator") {
      return true;
    }

    try {
      let str = aObject.toString();
      if (typeof aObject.next == "function" &&
          str.indexOf("[object Generator") == 0) {
        return true;
      }
    }
    catch (ex) {
      
      return false;
    }
  }

  return false;
}













function JSTermHelper(aJSTerm)
{
  






  aJSTerm.sandbox.$ = function JSTH_$(aId)
  {
    try {
      return aJSTerm._window.document.getElementById(aId);
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }
  };

  






  aJSTerm.sandbox.$$ = function JSTH_$$(aSelector)
  {
    try {
      return aJSTerm._window.document.querySelectorAll(aSelector);
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }
  };

  








  aJSTerm.sandbox.$x = function JSTH_$x(aXPath, aContext)
  {
    let nodes = [];
    let doc = aJSTerm._window.document;
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
  };

  




  Object.defineProperty(aJSTerm.sandbox, "$0", {
    get: function() {
      let mw = HUDService.currentContext();
      try {
        return mw.InspectorUI.selection;
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
    },
    enumerable: true,
    configurable: false
  });

  


  aJSTerm.sandbox.clear = function JSTH_clear()
  {
    aJSTerm.helperEvaluated = true;
    aJSTerm.clearOutput();
  };

  






  aJSTerm.sandbox.keys = function JSTH_keys(aObject)
  {
    try {
      return Object.keys(unwrap(aObject));
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }
  };

  






  aJSTerm.sandbox.values = function JSTH_values(aObject)
  {
    let arrValues = [];
    let obj = unwrap(aObject);

    try {
      for (let prop in obj) {
        arrValues.push(obj[prop]);
      }
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }
    return arrValues;
  };

  


  aJSTerm.sandbox.help = function JSTH_help()
  {
    aJSTerm.helperEvaluated = true;
    aJSTerm._window.open(
        "https://developer.mozilla.org/AppLinks/WebConsoleHelp?locale=" +
        aJSTerm._window.navigator.language, "help", "");
  };

  






  aJSTerm.sandbox.inspect = function JSTH_inspect(aObject)
  {
    aJSTerm.helperEvaluated = true;
    let propPanel = aJSTerm.openPropertyPanel(null, unwrap(aObject));
    propPanel.panel.setAttribute("hudId", aJSTerm.hudId);
  };

  






  aJSTerm.sandbox.inspectstyle = function JSTH_inspectstyle(aNode)
  {
    let errstr = null;
    aJSTerm.helperEvaluated = true;

    if (!Services.prefs.getBoolPref("devtools.styleinspector.enabled")) {
      errstr = HUDService.getStr("inspectStyle.styleInspectorNotEnabled");
    } else if (!aNode) {
      errstr = HUDService.getStr("inspectStyle.nullObjectPassed");
    } else if (!(aNode instanceof Ci.nsIDOMNode)) {
      errstr = HUDService.getStr("inspectStyle.mustBeDomNode");
    } else if (!(aNode.style instanceof Ci.nsIDOMCSSStyleDeclaration)) {
      errstr = HUDService.getStr("inspectStyle.nodeHasNoStyleProps");
    }

    if (!errstr) {
      let stylePanel = StyleInspector.createPanel();
      stylePanel.setAttribute("hudToolId", aJSTerm.hudId);
      stylePanel.showTool(aNode);
    } else {
      aJSTerm.writeOutput(errstr + "\n", CATEGORY_OUTPUT, SEVERITY_ERROR);
    }
  };

  






  aJSTerm.sandbox.pprint = function JSTH_pprint(aObject)
  {
    aJSTerm.helperEvaluated = true;
    if (aObject === null || aObject === undefined || aObject === true || aObject === false) {
      aJSTerm.console.error(HUDService.getStr("helperFuncUnsupportedTypeError"));
      return;
    }
    else if (typeof aObject === TYPEOF_FUNCTION) {
      aJSTerm.writeOutput(aObject + "\n", CATEGORY_OUTPUT, SEVERITY_LOG);
      return;
    }

    let output = [];
    let pairs = namesAndValuesOf(unwrap(aObject));

    pairs.forEach(function(pair) {
      output.push("  " + pair.display);
    });

    aJSTerm.writeOutput(output.join("\n"), CATEGORY_OUTPUT, SEVERITY_LOG);
  };

  






  aJSTerm.sandbox.print = function JSTH_print(aString)
  {
    aJSTerm.helperEvaluated = true;
    aJSTerm.writeOutput("" + aString, CATEGORY_OUTPUT, SEVERITY_LOG);
  };
}




















function JSTerm(aContext, aParentNode, aMixin, aConsole)
{
  

  this.application = appName();
  this.context = aContext;
  this.parentNode = aParentNode;
  this.mixins = aMixin;
  this.console = aConsole;

  this.setTimeout = aParentNode.ownerDocument.defaultView.setTimeout;

  let node = aParentNode;
  while (!node.hasAttribute("id")) {
    node = node.parentNode;
  }
  this.hudId = node.getAttribute("id");

  this.historyIndex = 0;
  this.historyPlaceHolder = 0;  
  this.log = LogFactory("*** JSTerm:");
  this.autocompletePopup = new AutocompletePopup(aParentNode.ownerDocument);
  this.autocompletePopup.onSelect = this.onAutocompleteSelect.bind(this);
  this.autocompletePopup.onClick = this.acceptProposedCompletion.bind(this);
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
    this.outputNode = this.mixins.outputNode;
    this.completeNode = this.mixins.completeNode;

    this._keyPress = this.keyPress.bind(this);
    this._inputEventHandler = this.inputEventHandler.bind(this);

    this.inputNode.addEventListener("keypress",
      this._keyPress, false);
    this.inputNode.addEventListener("input",
      this._inputEventHandler, false);
    this.inputNode.addEventListener("keyup",
      this._inputEventHandler, false);
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
    
    this.sandbox = new Cu.Sandbox(this._window,
      { sandboxPrototype: this._window, wantXrays: false });
    this.sandbox.console = this.console;
    JSTermHelper(this);
  },

  get _window()
  {
    return this.context.get().QueryInterface(Ci.nsIDOMWindow);
  },

  







  evalInSandbox: function JST_evalInSandbox(aString)
  {
    
    if (aString.trim() === "help" || aString.trim() === "?") {
      aString = "help()";
    }

    let window = unwrap(this.sandbox.window);
    let $ = null, $$ = null;

    
    
    if (typeof window.$ == "function") {
      $ = this.sandbox.$;
      delete this.sandbox.$;
    }
    if (typeof window.$$ == "function") {
      $$ = this.sandbox.$$;
      delete this.sandbox.$$;
    }

    let result = Cu.evalInSandbox(aString, this.sandbox, "1.8", "Web Console", 1);

    if ($) {
      this.sandbox.$ = $;
    }
    if ($$) {
      this.sandbox.$$ = $$;
    }

    return result;
  },


  execute: function JST_execute(aExecuteString)
  {
    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      this.writeOutput("no value to execute", CATEGORY_OUTPUT, SEVERITY_LOG);
      return;
    }

    this.writeOutput(aExecuteString, CATEGORY_INPUT, SEVERITY_LOG);

    try {
      this.helperEvaluated = false;
      let result = this.evalInSandbox(aExecuteString);

      
      let shouldShow = !(result === undefined && this.helperEvaluated);
      if (shouldShow) {
        let inspectable = this.isResultInspectable(result);
        let resultString = this.formatResult(result);

        if (inspectable) {
          this.writeOutputJS(aExecuteString, result, resultString);
        }
        else {
          this.writeOutput(resultString, CATEGORY_OUTPUT, SEVERITY_LOG);
        }
      }
    }
    catch (ex) {
      this.writeOutput("" + ex, CATEGORY_OUTPUT, SEVERITY_ERROR);
    }

    this.history.push(aExecuteString);
    this.historyIndex++;
    this.historyPlaceHolder = this.history.length;
    this.setInputValue("");
    this.clearCompletion();
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

    let doc = self.parentNode.ownerDocument;
    let parent = doc.getElementById("mainPopupSet");
    let title = (aEvalString
        ? HUDService.getFormatStr("jsPropertyInspectTitle", [aEvalString])
        : HUDService.getStr("jsPropertyTitle"));

    propPanel = new PropertyPanel(parent, doc, title, aOutputObject, buttons);
    propPanel.linkNode = aAnchor;

    let panel = propPanel.panel;
    panel.openPopup(aAnchor, "after_pointer", 0, 0, false, false);
    panel.sizeTo(350, 450);
    return propPanel;
  },

  











  writeOutputJS: function JST_writeOutputJS(aEvalString, aOutputObject, aOutputString)
  {
    let node = ConsoleUtils.createMessageNode(this.parentNode.ownerDocument,
                                              CATEGORY_OUTPUT,
                                              SEVERITY_LOG,
                                              aOutputString,
                                              this.hudId);

    let linkNode = node.querySelector(".webconsole-msg-body");

    linkNode.classList.add("hud-clickable");
    linkNode.setAttribute("aria-haspopup", "true");

    
    node.addEventListener("mousedown", function(aEvent) {
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    let self = this;
    node.addEventListener("click", function(aEvent) {
      if (aEvent.detail != 1 || aEvent.button != 0 ||
          (this._startX != aEvent.clientX &&
           this._startY != aEvent.clientY)) {
        return;
      }

      if (!this._panelOpen) {
        let propPanel = self.openPropertyPanel(aEvalString, aOutputObject, this);
        propPanel.panel.setAttribute("hudId", self.hudId);
        this._panelOpen = true;
      }
    }, false);

    ConsoleUtils.outputMessageNode(node, this.hudId);
  },

  











  writeOutput: function JST_writeOutput(aOutputMessage, aCategory, aSeverity)
  {
    let node = ConsoleUtils.createMessageNode(this.parentNode.ownerDocument,
                                              aCategory, aSeverity,
                                              aOutputMessage, this.hudId);

    ConsoleUtils.outputMessageNode(node, this.hudId);
  },

  







  formatResult: function JST_formatResult(aResult)
  {
    let output = "";
    let type = this.getResultType(aResult);

    switch (type) {
      case "string":
        output = this.formatString(aResult);
        break;
      case "boolean":
      case "date":
      case "error":
      case "number":
      case "regexp":
        output = aResult.toString();
        break;
      case "null":
      case "undefined":
        output = type;
        break;
      default:
        if (aResult.toSource) {
          try {
            output = aResult.toSource();
          } catch (ex) { }
        }
        if (!output || output == "({})") {
          output = aResult.toString();
        }
        break;
    }

    return output;
  },

  







  formatString: function JST_formatString(aString)
  {
    function isControlCode(c) {
      
      
      
      
      return (c <= 0x1F) || (0x7F <= c && c <= 0xA0);
    }

    function replaceFn(aMatch, aType, aHex) {
      
      let c = parseInt(aHex, 16);
      return isControlCode(c) ? aMatch : String.fromCharCode(c);
    }

    let output = uneval(aString).replace(/\\(x)([0-9a-fA-F]{2})/g, replaceFn)
                 .replace(/\\(u)([0-9a-fA-F]{4})/g, replaceFn);

    return output;
  },

  







  isResultInspectable: function JST_isResultInspectable(aResult)
  {
    let isEnumerable = false;

    
    if (isIteratorOrGenerator(aResult)) {
      return false;
    }

    for (let p in aResult) {
      isEnumerable = true;
      break;
    }

    return isEnumerable && typeof(aResult) != "string";
  },

  








  getResultType: function JST_getResultType(aResult)
  {
    let type = aResult === null ? "null" : typeof aResult;
    if (type == "object" && aResult.constructor && aResult.constructor.name) {
      type = aResult.constructor.name;
    }

    return type.toLowerCase();
  },

  clearOutput: function JST_clearOutput()
  {
    let hud = HUDService.getHudReferenceById(this.hudId);
    hud.cssNodes = {};

    let node = hud.outputNode;
    while (node.firstChild) {
      if (node.firstChild.classList &&
          node.firstChild.classList.contains("webconsole-msg-inspector")) {
        hud.pruneConsoleDirNode(node.firstChild);
      }
      else {
        hud.outputNode.removeChild(node.firstChild);
      }
    }

    hud.HUDBox.lastTimestamp = 0;
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
    this.lastInputValue = aNewValue;
    this.completeNode.value = "";
    this.resizeInput();
  },

  




  inputEventHandler: function JSTF_inputEventHandler(aEvent)
  {
    if (this.lastInputValue != this.inputNode.value) {
      this.resizeInput();
      this.complete(this.COMPLETE_HINT_ONLY);
      this.lastInputValue = this.inputNode.value;
    }
  },

  




  keyPress: function JSTF_keyPress(aEvent)
  {
    if (aEvent.ctrlKey) {
      switch (aEvent.charCode) {
        case 97:
          
          this.inputNode.setSelectionRange(0, 0);
          aEvent.preventDefault();
          break;
        case 101:
          
          this.inputNode.setSelectionRange(this.inputNode.value.length,
                                           this.inputNode.value.length);
          aEvent.preventDefault();
          break;
        default:
          break;
      }
      return;
    }
    else if (aEvent.shiftKey &&
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN) {
      
      
      return;
    }

    let inputUpdated = false;

    switch(aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE:
        if (this.autocompletePopup.isOpen) {
          this.clearCompletion();
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_RETURN:
        if (this.autocompletePopup.isOpen) {
          this.acceptProposedCompletion();
        }
        else {
          this.execute();
        }
        aEvent.preventDefault();
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_UP:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_BACKWARD);
        }
        else if (this.canCaretGoPrevious()) {
          inputUpdated = this.historyPeruse(HISTORY_BACK);
        }
        if (inputUpdated) {
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_DOWN:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_FORWARD);
        }
        else if (this.canCaretGoNext()) {
          inputUpdated = this.historyPeruse(HISTORY_FORWARD);
        }
        if (inputUpdated) {
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_TAB:
        
        if (this.complete(this.COMPLETE_HINT_ONLY) &&
            this.lastCompletion &&
            this.acceptProposedCompletion()) {
          aEvent.preventDefault();
        }
        break;

      default:
        break;
    }
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

  








  canCaretGoPrevious: function JST_canCaretGoPrevious()
  {
    let node = this.inputNode;
    if (node.selectionStart != node.selectionEnd) {
      return false;
    }

    let multiline = /[\r\n]/.test(node.value);
    return node.selectionStart == 0 ? true :
           node.selectionStart == node.value.length && !multiline;
  },

  








  canCaretGoNext: function JST_canCaretGoNext()
  {
    let node = this.inputNode;
    if (node.selectionStart != node.selectionEnd) {
      return false;
    }

    let multiline = /[\r\n]/.test(node.value);
    return node.selectionStart == node.value.length ? true :
           node.selectionStart == 0 && !multiline;
  },

  history: [],

  
  lastCompletion: null,

  


























  complete: function JSTF_complete(type)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    
    if (!inputValue) {
      this.clearCompletion();
      return false;
    }

    
    if (inputNode.selectionStart == inputNode.selectionEnd &&
        inputNode.selectionEnd != inputValue.length) {
      this.clearCompletion();
      return false;
    }

    let popup = this.autocompletePopup;

    if (!this.lastCompletion || this.lastCompletion.value != inputValue) {
      let properties = this.propertyProvider(this.sandbox.window, inputValue);
      if (!properties || !properties.matches.length) {
        this.clearCompletion();
        return false;
      }

      let items = properties.matches.map(function(aMatch) {
        return {label: aMatch};
      });
      popup.setItems(items);
      this.lastCompletion = {value: inputValue,
                             matchProp: properties.matchProp};

      if (items.length > 1 && !popup.isOpen) {
        popup.openPopup(this.inputNode);
      }
      else if (items.length < 2 && popup.isOpen) {
        popup.hidePopup();
      }

      if (items.length > 0) {
        popup.selectedIndex = 0;
        if (items.length == 1) {
          
          this.onAutocompleteSelect();
        }
      }
    }

    let accepted = false;

    if (type != this.COMPLETE_HINT_ONLY && popup.itemCount == 1) {
      this.acceptProposedCompletion();
      accepted = true;
    }
    else if (type == this.COMPLETE_BACKWARD) {
      this.autocompletePopup.selectPreviousItem();
    }
    else if (type == this.COMPLETE_FORWARD) {
      this.autocompletePopup.selectNextItem();
    }

    return accepted || popup.itemCount > 0;
  },

  onAutocompleteSelect: function JSTF_onAutocompleteSelect()
  {
    let currentItem = this.autocompletePopup.selectedItem;
    if (currentItem && this.lastCompletion) {
      let suffix = currentItem.label.substring(this.lastCompletion.
                                               matchProp.length);
      this.updateCompleteNode(suffix);
    }
    else {
      this.updateCompleteNode("");
    }
  },

  



  clearCompletion: function JSTF_clearCompletion()
  {
    this.autocompletePopup.clearItems();
    this.lastCompletion = null;
    this.updateCompleteNode("");
    if (this.autocompletePopup.isOpen) {
      this.autocompletePopup.hidePopup();
    }
  },

  






  acceptProposedCompletion: function JSTF_acceptProposedCompletion()
  {
    let updated = false;

    let currentItem = this.autocompletePopup.selectedItem;
    if (currentItem && this.lastCompletion) {
      let suffix = currentItem.label.substring(this.lastCompletion.
                                               matchProp.length);
      this.setInputValue(this.inputNode.value + suffix);
      updated = true;
    }

    this.clearCompletion();

    return updated;
  },

  





  updateCompleteNode: function JSTF_updateCompleteNode(aSuffix)
  {
    
    let prefix = aSuffix ? this.inputNode.value.replace(/[\S]/g, " ") : "";
    this.completeNode.value = prefix + aSuffix;
  },

  


  destroy: function JST_destroy()
  {
    this.inputNode.removeEventListener("keypress", this._keyPress, false);
    this.inputNode.removeEventListener("input", this._inputEventHandler, false);
    this.inputNode.removeEventListener("keyup", this._inputEventHandler, false);
  },
};













function
JSTermFirefoxMixin(aContext,
                   aParentNode,
                   aExistingConsole)
{
  
  
  
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
    this.completeNode = this.xulElementFactory("textbox");
    this.completeNode.setAttribute("class", "jsterm-complete-node");
    this.completeNode.setAttribute("multiline", "true");
    this.completeNode.setAttribute("rows", "1");
    this.completeNode.setAttribute("tabindex", "-1");

    this.inputNode = this.xulElementFactory("textbox");
    this.inputNode.setAttribute("class", "jsterm-input-node");
    this.inputNode.setAttribute("multiline", "true");
    this.inputNode.setAttribute("rows", "1");

    let inputStack = this.xulElementFactory("stack");
    inputStack.setAttribute("class", "jsterm-stack-node");
    inputStack.setAttribute("flex", "1");
    inputStack.appendChild(this.completeNode);
    inputStack.appendChild(this.inputNode);

    if (this.existingConsoleNode == undefined) {
      throw new Error("This can't happen");
    }

    this.outputNode = this.existingConsoleNode;

    this.term = this.xulElementFactory("hbox");
    this.term.setAttribute("class", "jsterm-input-container");
    this.term.setAttribute("style", "direction: ltr;");
    this.term.appendChild(inputStack);
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






function FirefoxApplicationHooks()
{ }

FirefoxApplicationHooks.prototype = {
  




  getCurrentContext: function FAH_getCurrentContext()
  {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },
};










ConsoleUtils = {
  supString: function ConsoleUtils_supString(aString)
  {
    let str = Cc["@mozilla.org/supports-string;1"].
      createInstance(Ci.nsISupportsString);
    str.data = aString;
    return str;
  },

  




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
    
    let richListBoxNode = aNode.parentNode;
    while (richListBoxNode.tagName != "richlistbox") {
      richListBoxNode = richListBoxNode.parentNode;
    }

    
    let boxObject = richListBoxNode.scrollBoxObject;
    let nsIScrollBoxObject = boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    nsIScrollBoxObject.ensureElementIsVisible(aNode);
  },

  




























  createMessageNode:
  function ConsoleUtils_createMessageNode(aDocument, aCategory, aSeverity,
                                          aBody, aHUDId, aSourceURL,
                                          aSourceLine, aClipboardText, aLevel) {
    if (typeof aBody != "string" && aClipboardText == null && aBody.innerText) {
      aClipboardText = aBody.innerText;
    }

    
    
    
    let iconContainer = aDocument.createElementNS(XUL_NS, "vbox");
    iconContainer.classList.add("webconsole-msg-icon-container");
    
    let hud = HUDService.getHudReferenceById(aHUDId);
    iconContainer.style.marginLeft = hud.groupDepth * GROUP_INDENT + "px";

    
    
    let iconNode = aDocument.createElementNS(XUL_NS, "image");
    iconNode.classList.add("webconsole-msg-icon");
    iconContainer.appendChild(iconNode);

    
    let spacer = aDocument.createElementNS(XUL_NS, "spacer");
    spacer.setAttribute("flex", "1");
    iconContainer.appendChild(spacer);

    
    let bodyNode = aDocument.createElementNS(XUL_NS, "description");
    bodyNode.setAttribute("flex", "1");
    bodyNode.classList.add("webconsole-msg-body");

    
    
    let body = aBody;
    
    
    aClipboardText = aClipboardText ||
                     (aBody + (aSourceURL ? " @ " + aSourceURL : "") +
                              (aSourceLine ? ":" + aSourceLine : ""));
    aBody = aBody instanceof Ci.nsIDOMNode && !(aLevel == "dir") ?
            aBody : aDocument.createTextNode(aBody);

    if (!aBody.nodeType) {
      aBody = aDocument.createTextNode(aBody.toString());
    }
    if (typeof aBody == "string") {
      aBody = aDocument.createTextNode(aBody);
    }

    bodyNode.appendChild(aBody);

    let repeatContainer = aDocument.createElementNS(XUL_NS, "hbox");
    repeatContainer.setAttribute("align", "start");
    let repeatNode = aDocument.createElementNS(XUL_NS, "label");
    repeatNode.setAttribute("value", "1");
    repeatNode.classList.add("webconsole-msg-repeat");
    repeatContainer.appendChild(repeatNode);

    
    let timestampNode = aDocument.createElementNS(XUL_NS, "label");
    timestampNode.classList.add("webconsole-timestamp");
    let timestamp = ConsoleUtils.timestamp();
    let timestampString = ConsoleUtils.timestampString(timestamp);
    timestampNode.setAttribute("value", timestampString);

    
    
    let locationNode;
    if (aSourceURL) {
      locationNode = this.createLocationNode(aDocument, aSourceURL,
                                             aSourceLine);
    }

    
    let node = aDocument.createElementNS(XUL_NS, "richlistitem");
    node.clipboardText = aClipboardText;
    node.classList.add("hud-msg-node");

    node.timestamp = timestamp;
    ConsoleUtils.setMessageType(node, aCategory, aSeverity);

    node.appendChild(timestampNode);
    node.appendChild(iconContainer);
    
    if (aLevel == "dir") {
      
      
      let bodyContainer = aDocument.createElement("vbox");
      bodyContainer.setAttribute("flex", "1");
      bodyContainer.appendChild(bodyNode);
      
      let tree = createElement(aDocument, "tree", {
        flex: 1,
        hidecolumnpicker: "true"
      });

      let treecols = aDocument.createElement("treecols");
      let treecol = createElement(aDocument, "treecol", {
        primary: "true",
        flex: 1,
        hideheader: "true",
        ignoreincolumnpicker: "true"
      });
      treecols.appendChild(treecol);
      tree.appendChild(treecols);

      tree.appendChild(aDocument.createElement("treechildren"));

      bodyContainer.appendChild(tree);
      node.appendChild(bodyContainer);
      node.classList.add("webconsole-msg-inspector");
      
      let treeView = node.propertyTreeView = new PropertyTreeView();
      treeView.data = body;
      tree.setAttribute("rows", treeView.rowCount);
    }
    else {
      node.appendChild(bodyNode);
    }
    node.appendChild(repeatContainer);
    if (locationNode) {
      node.appendChild(locationNode);
    }

    node.setAttribute("id", "console-msg-" + HUDService.sequenceId());

    return node;
  },

  











  setMessageType:
  function ConsoleUtils_setMessageType(aMessageNode, aNewCategory,
                                       aNewSeverity) {
    
    if ("category" in aMessageNode) {
      let oldCategory = aMessageNode.category;
      let oldSeverity = aMessageNode.severity;
      aMessageNode.classList.remove("webconsole-msg-" +
                                    CATEGORY_CLASS_FRAGMENTS[oldCategory]);
      aMessageNode.classList.remove("webconsole-msg-" +
                                    SEVERITY_CLASS_FRAGMENTS[oldSeverity]);
      let key = "hud-" + MESSAGE_PREFERENCE_KEYS[oldCategory][oldSeverity];
      aMessageNode.classList.remove(key);
    }

    
    aMessageNode.category = aNewCategory;
    aMessageNode.severity = aNewSeverity;
    aMessageNode.classList.add("webconsole-msg-" +
                               CATEGORY_CLASS_FRAGMENTS[aNewCategory]);
    aMessageNode.classList.add("webconsole-msg-" +
                               SEVERITY_CLASS_FRAGMENTS[aNewSeverity]);
    let key = "hud-" + MESSAGE_PREFERENCE_KEYS[aNewCategory][aNewSeverity];
    aMessageNode.classList.add(key);
  },

  













  createLocationNode:
  function ConsoleUtils_createLocationNode(aDocument, aSourceURL,
                                           aSourceLine) {
    let locationNode = aDocument.createElementNS(XUL_NS, "label");

    
    
    let text = ConsoleUtils.abbreviateSourceURL(aSourceURL);
    if (aSourceLine) {
      text += ":" + aSourceLine;
    }
    locationNode.setAttribute("value", text);

    
    locationNode.setAttribute("crop", "center");
    locationNode.setAttribute("title", aSourceURL);
    locationNode.classList.add("webconsole-location");
    locationNode.classList.add("text-link");

    
    locationNode.addEventListener("click", function() {
      if (aSourceURL == "Scratchpad") {
        let win = Services.wm.getMostRecentWindow("devtools:scratchpad");
        if (win) {
          win.focus();
        }
        return;
      }
      let viewSourceUtils = aDocument.defaultView.gViewSourceUtils;
      viewSourceUtils.viewSource(aSourceURL, null, aDocument, aSourceLine);
    }, true);

    return locationNode;
  },

  








  filterMessageNode: function ConsoleUtils_filterMessageNode(aNode, aHUDId) {
    
    let prefKey = MESSAGE_PREFERENCE_KEYS[aNode.category][aNode.severity];
    if (prefKey && !HUDService.getFilterState(aHUDId, prefKey)) {
      
      aNode.classList.add("hud-filtered-by-type");
    }

    
    let search = HUDService.getFilterStringByHUDId(aHUDId);
    let text = aNode.clipboardText;

    
    if (!HUDService.stringMatchesFilters(text, search)) {
      aNode.classList.add("hud-filtered-by-string");
    }
  },

  








  mergeFilteredMessageNode:
  function ConsoleUtils_mergeFilteredMessageNode(aOriginal, aFiltered) {
    
    
    let repeatNode = aOriginal.childNodes[3].firstChild;
    if (!repeatNode) {
      return aOriginal; 
    }

    let occurrences = parseInt(repeatNode.getAttribute("value")) + 1;
    repeatNode.setAttribute("value", occurrences);
  },

  










  filterRepeatedCSS:
  function ConsoleUtils_filterRepeatedCSS(aNode, aOutput, aHUDId) {
    let hud = HUDService.getHudReferenceById(aHUDId);

    
    let description = aNode.childNodes[2].textContent;
    let location;

    
    
    if (aNode.childNodes[4]) {
      
      location = aNode.childNodes[4].getAttribute("title");
    }
    else {
      location = "";
    }

    let dupe = hud.cssNodes[description + location];
    if (!dupe) {
      
      hud.cssNodes[description + location] = aNode;
      return false;
    }

    this.mergeFilteredMessageNode(dupe, aNode);

    return true;
  },

  












  filterRepeatedConsole:
  function ConsoleUtils_filterRepeatedConsole(aNode, aOutput) {
    let lastMessage = aOutput.lastChild;

    
    if (lastMessage && !aNode.classList.contains("webconsole-msg-inspector") &&
        aNode.childNodes[2].textContent ==
        lastMessage.childNodes[2].textContent) {
      this.mergeFilteredMessageNode(lastMessage, aNode);
      return true;
    }

    return false;
  },

  








  outputMessageNode: function ConsoleUtils_outputMessageNode(aNode, aHUDId) {
    ConsoleUtils.filterMessageNode(aNode, aHUDId);
    let outputNode = HUDService.hudReferences[aHUDId].outputNode;

    let scrolledToBottom = ConsoleUtils.isOutputScrolledToBottom(outputNode);

    let isRepeated = false;
    if (aNode.classList.contains("webconsole-msg-cssparser")) {
      isRepeated = this.filterRepeatedCSS(aNode, outputNode, aHUDId);
    }

    if (!isRepeated &&
        (aNode.classList.contains("webconsole-msg-console") ||
         aNode.classList.contains("webconsole-msg-exception") ||
         aNode.classList.contains("webconsole-msg-error"))) {
      isRepeated = this.filterRepeatedConsole(aNode, outputNode);
    }

    if (!isRepeated) {
      outputNode.appendChild(aNode);
    }

    HUDService.regroupOutput(outputNode);

    if (pruneConsoleOutputIfNecessary(aHUDId, aNode.category) == 0) {
      
      
      return;
    }

    let isInputOutput = aNode.classList.contains("webconsole-msg-input") ||
                        aNode.classList.contains("webconsole-msg-output");
    let isFiltered = aNode.classList.contains("hud-filtered-by-string") ||
                     aNode.classList.contains("hud-filtered-by-type");

    
    
    
    if (!isFiltered && !isRepeated && (scrolledToBottom || isInputOutput)) {
      ConsoleUtils.scrollToVisible(aNode);
    }

    let id = ConsoleUtils.supString(aHUDId);
    let nodeID = aNode.getAttribute("id");
    Services.obs.notifyObservers(id, "web-console-message-created", nodeID);
  },

  







  isOutputScrolledToBottom:
  function ConsoleUtils_isOutputScrolledToBottom(aOutputNode)
  {
    let lastNodeHeight = aOutputNode.lastChild ?
                         aOutputNode.lastChild.clientHeight : 0;
    let scrollBox = aOutputNode.scrollBoxObject.element;

    return scrollBox.scrollTop + scrollBox.clientHeight >=
           scrollBox.scrollHeight - lastNodeHeight / 2;
  },

  








  abbreviateSourceURL: function ConsoleUtils_abbreviateSourceURL(aSourceURL) {
    
    let hookIndex = aSourceURL.indexOf("?");
    if (hookIndex > -1) {
      aSourceURL = aSourceURL.substring(0, hookIndex);
    }

    
    if (aSourceURL[aSourceURL.length - 1] == "/") {
      aSourceURL = aSourceURL.substring(0, aSourceURL.length - 1);
    }

    
    let slashIndex = aSourceURL.lastIndexOf("/");
    if (slashIndex > -1) {
      aSourceURL = aSourceURL.substring(slashIndex + 1);
    }

    return aSourceURL;
  }
};





HeadsUpDisplayUICommands = {
  toggleHUD: function UIC_toggleHUD() {
    var window = HUDService.currentContext();
    var gBrowser = window.gBrowser;
    var linkedBrowser = gBrowser.selectedTab.linkedBrowser;
    var tabId = gBrowser.getNotificationBox(linkedBrowser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var ownerDocument = gBrowser.selectedTab.ownerDocument;
    var hud = ownerDocument.getElementById(hudId);
    var hudRef = HUDService.hudReferences[hudId];

    if (hudRef && hud) {
      if (hudRef.consolePanel) {
        HUDService.deactivateHUDForContext(gBrowser.selectedTab, false);
      }
      else {
        HUDService.storeHeight(hudId);

        HUDService.animate(hudId, ANIMATE_OUT, function() {
          
          
          
          
          if (ownerDocument.getElementById(hudId)) {
            HUDService.deactivateHUDForContext(gBrowser.selectedTab, true);
          }
        });
      }
    }
    else {
      HUDService.activateHUDForContext(gBrowser.selectedTab, true);
      HUDService.animate(hudId, ANIMATE_IN);
    }
  },

  





  getOpenHUD: function UIC_getOpenHUD() {
    let chromeWindow = HUDService.currentContext();
    let contentWindow = chromeWindow.gBrowser.selectedBrowser.contentWindow;
    return HUDService.getHudIdByWindow(contentWindow);
  },

  







  toggleFilter: function UIC_toggleFilter(aEvent) {
    let hudId = this.getAttribute("hudId");
    switch (this.tagName) {
      case "toolbarbutton": {
        let originalTarget = aEvent.originalTarget;
        let classes = originalTarget.classList;

        if (originalTarget.localName !== "toolbarbutton") {
          
          
          
          break;
        }

        if (!classes.contains("toolbarbutton-menubutton-button") &&
            originalTarget.getAttribute("type") === "menu-button") {
          
          
          
          break;
        }

        let state = this.getAttribute("checked") !== "true";
        this.setAttribute("checked", state);

        
        
        
        let menuItems = this.querySelectorAll("menuitem");
        for (let i = 0; i < menuItems.length; i++) {
          menuItems[i].setAttribute("checked", state);
          let prefKey = menuItems[i].getAttribute("prefKey");
          HUDService.setFilterState(hudId, prefKey, state);
        }
        break;
      }

      case "menuitem": {
        let state = this.getAttribute("checked") !== "true";
        this.setAttribute("checked", state);

        let prefKey = this.getAttribute("prefKey");
        HUDService.setFilterState(hudId, prefKey, state);

        
        let menuPopup = this.parentNode;

        let allChecked = true;
        let menuItem = menuPopup.firstChild;
        while (menuItem) {
          if (menuItem.getAttribute("checked") !== "true") {
            allChecked = false;
            break;
          }
          menuItem = menuItem.nextSibling;
        }

        let toolbarButton = menuPopup.parentNode;
        toolbarButton.setAttribute("checked", allChecked);
        break;
      }
    }

    return true;
  },

  command: function UIC_command(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    switch (filter) {
      case "copy": {
        let outputNode = HUDService.hudReferences[hudId].outputNode;
        HUDService.copySelectedItems(outputNode);
        break;
      }
      case "selectAll": {
        HUDService.hudReferences[hudId].outputNode.selectAll();
        break;
      }
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
                networkinfo: PREFS_PREFIX + "networkinfo",
                csserror: PREFS_PREFIX + "csserror",
                cssparser: PREFS_PREFIX + "cssparser",
                exception: PREFS_PREFIX + "exception",
                jswarn: PREFS_PREFIX + "jswarn",
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
      networkinfo: (prefs.getBoolPref(PREFS.networkinfo) ? true: false),
      csserror: (prefs.getBoolPref(PREFS.csserror) ? true: false),
      cssparser: (prefs.getBoolPref(PREFS.cssparser) ? true: false),
      exception: (prefs.getBoolPref(PREFS.exception) ? true: false),
      jswarn: (prefs.getBoolPref(PREFS.jswarn) ? true: false),
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
    prefs.setBoolPref(PREFS.networkinfo, true);
    prefs.setBoolPref(PREFS.csserror, true);
    prefs.setBoolPref(PREFS.cssparser, true);
    prefs.setBoolPref(PREFS.exception, true);
    prefs.setBoolPref(PREFS.jswarn, true);
    prefs.setBoolPref(PREFS.error, true);
    prefs.setBoolPref(PREFS.info, true);
    prefs.setBoolPref(PREFS.warn, true);
    prefs.setBoolPref(PREFS.log, true);
    prefs.setBoolPref(PREFS.global, false);

    defaultDisplayPrefs = {
      network: prefs.getBoolPref(PREFS.network),
      networkinfo: prefs.getBoolPref(PREFS.networkinfo),
      csserror: prefs.getBoolPref(PREFS.csserror),
      cssparser: prefs.getBoolPref(PREFS.cssparser),
      exception: prefs.getBoolPref(PREFS.exception),
      jswarn: prefs.getBoolPref(PREFS.jswarn),
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
    prefs.setBoolPref(PREFS.networkinfo,
                      (aPrefsObject.networkinfo ? true : false));
    prefs.setBoolPref(PREFS.csserror, (aPrefsObject.csserror ? true : false));
    prefs.setBoolPref(PREFS.cssparser, (aPrefsObject.cssparser ? true : false));
    prefs.setBoolPref(PREFS.exception, (aPrefsObject.exception ? true : false));
    prefs.setBoolPref(PREFS.jswarn, (aPrefsObject.jswarn ? true : false));
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
    Services.obs.removeObserver(this, "xpcom-shutdown");
    this.initialConsoleCreated = false;
  },

};









function CommandController(aWindow) {
  this.window = aWindow;
}

CommandController.prototype = {
  






  _getFocusedOutputNode: function CommandController_getFocusedOutputNode()
  {
    let element = this.window.document.commandDispatcher.focusedElement;
    if (element && element.classList.contains("hud-output-node")) {
      return element;
    }
    return null;
  },

  







  copy: function CommandController_copy(aOutputNode)
  {
    HUDService.copySelectedItems(aOutputNode);
  },

  






  selectAll: function CommandController_selectAll(aOutputNode)
  {
    aOutputNode.selectAll();
  },

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    return this.isCommandEnabled(aCommand);
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    let outputNode = this._getFocusedOutputNode();
    if (!outputNode) {
      return false;
    }

    switch (aCommand) {
      case "cmd_copy":
        
        return outputNode.selectedCount > 0;
      case "cmd_selectAll":
        
        return true;
    }
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    let outputNode = this._getFocusedOutputNode();
    switch (aCommand) {
      case "cmd_copy":
        this.copy(outputNode);
        break;
      case "cmd_selectAll":
        this.selectAll(outputNode);
        break;
    }
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

  uninit: function HCO_uninit()
  {
    Services.console.unregisterListener(this);
    Services.obs.removeObserver(this, "xpcom-shutdown");
  },

  observe: function HCO_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      this.uninit();
      return;
    }

    if (!(aSubject instanceof Ci.nsIScriptError) ||
        !(aSubject instanceof Ci.nsIScriptError2) ||
        !aSubject.outerWindowID) {
      return;
    }

    switch (aSubject.category) {
      
      
      case "XPConnect JavaScript":
      case "component javascript":
      case "chrome javascript":
      case "chrome registration":
      case "XBL":
      case "XBL Prototype Handler":
      case "XBL Content Sink":
      case "xbl javascript":
        return;

      case "CSS Parser":
      case "CSS Loader":
        HUDService.reportPageError(CATEGORY_CSS, aSubject);
        return;

      default:
        HUDService.reportPageError(CATEGORY_JS, aSubject);
        return;
    }
  }
};









function ConsoleProgressListener(aHudId)
{
  this.hudId = aHudId;
}

ConsoleProgressListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),

  onStateChange: function CPL_onStateChange(aProgress, aRequest, aState,
                                            aStatus)
  {
    if (!(aState & Ci.nsIWebProgressListener.STATE_START)) {
      return;
    }

    let uri = null;
    if (aRequest instanceof Ci.imgIRequest) {
      let imgIRequest = aRequest.QueryInterface(Ci.imgIRequest);
      uri = imgIRequest.URI;
    }
    else if (aRequest instanceof Ci.nsIChannel) {
      let nsIChannel = aRequest.QueryInterface(Ci.nsIChannel);
      uri = nsIChannel.URI;
    }

    if (!uri || !uri.schemeIs("file") && !uri.schemeIs("ftp")) {
      return;
    }

    let outputNode = HUDService.hudReferences[this.hudId].outputNode;

    let chromeDocument = outputNode.ownerDocument;
    let msgNode = chromeDocument.createElementNS(HTML_NS, "html:span");

    
    let linkNode = chromeDocument.createElementNS(HTML_NS, "html:span");
    linkNode.appendChild(chromeDocument.createTextNode(uri.spec));
    linkNode.classList.add("hud-clickable");
    linkNode.classList.add("webconsole-msg-url");

    linkNode.addEventListener("mousedown", function(aEvent) {
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    linkNode.addEventListener("click", function(aEvent) {
      if (aEvent.detail == 1 && aEvent.button == 0 &&
          this._startX == aEvent.clientX && this._startY == aEvent.clientY) {
        let viewSourceUtils = chromeDocument.defaultView.gViewSourceUtils;
        viewSourceUtils.viewSource(uri.spec, null, chromeDocument);
      }
    }, false);

    msgNode.appendChild(linkNode);

    let messageNode = ConsoleUtils.createMessageNode(chromeDocument,
                                                     CATEGORY_NETWORK,
                                                     SEVERITY_LOG,
                                                     msgNode,
                                                     this.hudId,
                                                     null,
                                                     null,
                                                     uri.spec);

    ConsoleUtils.outputMessageNode(messageNode, this.hudId);
  },

  onLocationChange: function() {},
  onStatusChange: function() {},
  onProgressChange: function() {},
  onSecurityChange: function() {},
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
}
catch (ex) {
  Cu.reportError("HUDService failed initialization.\n" + ex);
  
  
}























function GcliTerm(aContentWindow, aHudId, aDocument, aConsole, aHintNode)
{
  this.context = Cu.getWeakReference(aContentWindow);
  this.hudId = aHudId;
  this.document = aDocument;
  this.console = aConsole;
  this.hintNode = aHintNode;

  this.createUI();
  this.createSandbox();

  this.show = this.show.bind(this);
  this.hide = this.hide.bind(this);

  this.opts = {
    environment: { hudId: this.hudId },
    chromeDocument: this.document,
    contentDocument: aContentWindow.document,
    jsEnvironment: {
      globalObject: aContentWindow,
      evalFunction: this.evalInSandbox.bind(this)
    },
    inputElement: this.inputNode,
    completeElement: this.completeNode,
    inputBackgroundElement: this.inputStack,
    hintElement: this.hintNode,
    completionPrompt: "",
    gcliTerm: this
  };

  gcli._internal.commandOutputManager.addListener(this.onCommandOutput, this);
  gcli._internal.createView(this.opts);
  GcliCommands.setDocument(aContentWindow.document);
}

GcliTerm.prototype = {
  


  hide: function GcliTerm_hide()
  {
    this.hintNode.parentNode.hidden = true;
  },

  


  show: function GcliTerm_show()
  {
    this.hintNode.parentNode.hidden = false;
  },

  


  destroy: function Gcli_destroy()
  {
    GcliCommands.unsetDocument();
    gcli._internal.removeView(this.opts);
    gcli._internal.commandOutputManager.removeListener(this.onCommandOutput, this);

    delete this.opts.chromeDocument;
    delete this.opts.inputElement;
    delete this.opts.completeElement;
    delete this.opts.inputBackgroundElement;
    delete this.opts.hintElement;
    delete this.opts.contentDocument;
    delete this.opts.jsEnvironment;
    delete this.opts.gcliTerm;

    delete this.context;
    delete this.document;
    delete this.console;
    delete this.hintNode;

    delete this.sandbox;
    delete this.element
    delete this.inputStack
    delete this.completeNode
    delete this.inputNode
  },

  







  reattachConsole: function Gcli_reattachConsole(aContentWindow, aConsole)
  {
    this.context = Cu.getWeakReference(aContentWindow);
    this.console = aConsole;
    this.createSandbox();
  },

  



  createUI: function Gcli_createUI()
  {
    this.element = this.document.createElement("vbox");
    this.element.setAttribute("class", "gcliterm-input-container");
    this.element.setAttribute("flex", "0");

    this.inputStack = this.document.createElement("stack");
    this.inputStack.setAttribute("class", "gcliterm-stack-node");
    this.element.appendChild(this.inputStack);

    this.completeNode = this.document.createElement("div");
    this.completeNode.setAttribute("class", "gcliterm-complete-node");
    this.completeNode.setAttribute("aria-live", "polite");
    this.inputStack.appendChild(this.completeNode);

    this.inputNode = this.document.createElement("textbox");
    this.inputNode.setAttribute("class", "gcliterm-input-node");
    this.inputNode.setAttribute("rows", "1");
    this.inputStack.appendChild(this.inputNode);
  },

  


  onCommandOutput: function Gcli_onCommandOutput(aEvent)
  {
    
    
    if (!aEvent.output.completed) {
      return;
    }

    this.writeOutput(aEvent.output.typed, { category: CATEGORY_INPUT });

    if (aEvent.output.output == null) {
      return;
    }

    let output = aEvent.output.output;
    if (aEvent.output.command.returnType == "html" && typeof output == "string") {
      let frag = this.document.createRange().createContextualFragment(
          '<div xmlns="' + HTML_NS + '" xmlns:xul="' + XUL_NS + '">' +
          output + '</div>');

      output = this.document.createElementNS(HTML_NS, "div");
      output.appendChild(frag);
    }
    this.writeOutput(output);
  },

  


  createSandbox: function Gcli_createSandbox()
  {
    let win = this.context.get().QueryInterface(Ci.nsIDOMWindow);

    
    this.sandbox = new Cu.Sandbox(win, {
      sandboxPrototype: win,
      wantXrays: false
    });
    this.sandbox.console = this.console;
  },

  






  evalInSandbox: function Gcli_evalInSandbox(aString)
  {
    return Cu.evalInSandbox(aString, this.sandbox, "1.8", "Web Console", 1);
  },

  










  writeOutput: function Gcli_writeOutput(aOutputMessage, aOptions)
  {
    aOptions = aOptions || {};

    let node = ConsoleUtils.createMessageNode(
                    this.document,
                    aOptions.category || CATEGORY_OUTPUT,
                    aOptions.severity || SEVERITY_LOG,
                    aOutputMessage,
                    this.hudId,
                    aOptions.sourceUrl || undefined,
                    aOptions.sourceLine || undefined,
                    aOptions.clipboardText || undefined);

    ConsoleUtils.outputMessageNode(node, this.hudId);
  },

  clearOutput: JSTerm.prototype.clearOutput,
};

