





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "mimeService", "@mozilla.org/mime;1",
                                   "nsIMIMEService");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkHelper",
                                  "resource://gre/modules/devtools/NetworkHelper.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

this.EXPORTED_SYMBOLS = ["NetworkPanel"];









this.NetworkPanel = function NetworkPanel(aParent, aHttpActivity)
{
  let doc = aParent.ownerDocument;
  this.httpActivity = aHttpActivity;

  
  this.panel = createElement(doc, "panel", {
    label: l10n.getStr("NetworkPanel.label"),
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
    self.httpActivity = null;

    if (self.linkNode) {
      self.linkNode._panelOpen = false;
      self.linkNode = null;
    }
  }, false);

  
  this.iframe.addEventListener("load", function onLoad() {
    if (!self.iframe) {
      return;
    }

    self.iframe.removeEventListener("load", onLoad, true);
    self.update();
  }, true);

  this.panel.addEventListener("popupshown", function onPopupShown() {
    self.panel.removeEventListener("popupshown", onPopupShown, true);
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
  


  _state: 0,

  


  _INIT: 0,
  _DISPLAYED_REQUEST_HEADER: 1,
  _DISPLAYED_REQUEST_BODY: 2,
  _DISPLAYED_RESPONSE_HEADER: 3,
  _TRANSITION_CLOSED: 4,

  _fromDataRegExp: /Content-Type\:\s*application\/x-www-form-urlencoded/,

  _contentType: null,

  






  _onUpdate: null,

  get document() {
    return this.iframe && this.iframe.contentWindow ?
           this.iframe.contentWindow.document : null;
  },

  











  _format: function NP_format(aName, aArray)
  {
    return l10n.getFormatStr("NetworkPanel." + aName, aArray);
  },

  








  get contentType()
  {
    if (this._contentType) {
      return this._contentType;
    }

    let request = this.httpActivity.request;
    let response = this.httpActivity.response;

    let contentType = "";
    let types = response.content ?
                (response.content.mimeType || "").split(/,|;/) : [];
    for (let i = 0; i < types.length; i++) {
      if (types[i] in NetworkHelper.mimeCategoryMap) {
        contentType = types[i];
        break;
      }
    }

    if (contentType) {
      this._contentType = contentType;
      return contentType;
    }

    
    let uri = NetUtil.newURI(request.url);
    if ((uri instanceof Ci.nsIURL) && uri.fileExtension) {
      try {
         contentType = mimeService.getTypeFromExtension(uri.fileExtension);
      }
      catch(ex) {
        
        Cu.reportError(ex);
      }
    }

    this._contentType = contentType;
    return contentType;
  },

  




  get _responseIsImage()
  {
    return this.contentType &&
           NetworkHelper.mimeCategoryMap[this.contentType] == "image";
  },

  




  get _isResponseBodyTextData()
  {
    let contentType = this.contentType;

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
    return this.httpActivity.response.status == 304;
  },

  





  get _isRequestBodyFormData()
  {
    let requestBody = this.httpActivity.request.postData.text;
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

    aList.sort(function(a, b) {
      return a.name.toLowerCase() < b.name.toLowerCase();
    });

    aList.forEach(function(aItem) {
      let name = aItem.name;
      let value = aItem.value;
      if (aIgnoreCookie && name == "Cookie") {
        return;
      }

      







      let row = doc.createElement("tr");
      let textNode = doc.createTextNode(name + ":");
      let th = doc.createElement("th");
      th.setAttribute("scope", "row");
      th.setAttribute("class", "property-name");
      th.appendChild(textNode);
      row.appendChild(th);

      textNode = doc.createTextNode(value);
      let td = doc.createElement("td");
      td.setAttribute("class", "property-value");
      td.appendChild(textNode);
      row.appendChild(td);

      parent.appendChild(row);
    });
  },

  





  _displayNode: function NP_displayNode(aId)
  {
    this.document.getElementById(aId).style.display = "block";
  },

  








  _displayRequestHeader: function NP__displayRequestHeader()
  {
    let request = this.httpActivity.request;
    let requestTime = new Date(this.httpActivity.startedDateTime);

    this._appendTextNode("headUrl", request.url);
    this._appendTextNode("headMethod", request.method);
    this._appendTextNode("requestHeadersInfo",
                         l10n.timestampString(requestTime));

    this._appendList("requestHeadersContent", request.headers, true);

    if (request.cookies.length > 0) {
      this._displayNode("requestCookie");
      this._appendList("requestCookieContent", request.cookies);
    }
  },

  





  _displayRequestBody: function NP__displayRequestBody()
  {
    let postData = this.httpActivity.request.postData;
    this._displayNode("requestBody");
    this._appendTextNode("requestBodyContent", postData.text);
  },

  





  _displayRequestForm: function NP__processRequestForm()
  {
    let postData = this.httpActivity.request.postData.text;
    let requestBodyLines = postData.split("\n");
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

    let formDataArray = [];
    for (let i = 0; i < formData.length; i++) {
      let data = formData[i];
      let idx = data.indexOf("=");
      let key = data.substring(0, idx);
      let value = data.substring(idx + 1);
      formDataArray.push({
        name: unescapeText(key),
        value: unescapeText(value)
      });
    }

    this._appendList("requestFormDataContent", formDataArray);
    this._displayNode("requestFormData");
  },

  






  _displayResponseHeader: function NP__displayResponseHeader()
  {
    let timing = this.httpActivity.timings;
    let response = this.httpActivity.response;

    this._appendTextNode("headStatus",
                         [response.httpVersion, response.status,
                          response.statusText].join(" "));

    
    
    let deltaDuration = 0;
    ["dns", "connect", "send", "wait"].forEach(function (aValue) {
      let ms = timing[aValue];
      if (ms > -1) {
        deltaDuration += ms;
      }
    });

    this._appendTextNode("responseHeadersInfo",
      this._format("durationMS", [deltaDuration]));

    this._displayNode("responseContainer");
    this._appendList("responseHeadersContent", response.headers);
  },

  







  _displayResponseImage: function NP__displayResponseImage()
  {
    let self = this;
    let timing = this.httpActivity.timings;
    let request = this.httpActivity.request;
    let cached = "";

    if (this._isResponseCached) {
      cached = "Cached";
    }

    let imageNode = this.document.getElementById("responseImage" +
                                                 cached + "Node");
    imageNode.setAttribute("src", request.url);

    
    function setImageInfo() {
      self._appendTextNode("responseImage" + cached + "Info",
        self._format("imageSizeDeltaDurationMS",
          [ imageNode.width, imageNode.height, timing.receive ]
        )
      );
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

  






  _displayResponseBody: function NP__displayResponseBody()
  {
    let timing = this.httpActivity.timings;
    let response = this.httpActivity.response;
    let cached =  this._isResponseCached ? "Cached" : "";

    this._appendTextNode("responseBody" + cached + "Info",
      this._format("durationMS", [timing.receive]));

    this._displayNode("responseBody" + cached);
    this._appendTextNode("responseBody" + cached + "Content",
                         response.content.text);
  },

  





  _displayResponseBodyUnknownType: function NP__displayResponseBodyUnknownType()
  {
    let timing = this.httpActivity.timings;

    this._displayNode("responseBodyUnknownType");
    this._appendTextNode("responseBodyUnknownTypeInfo",
      this._format("durationMS", [timing.receive]));

    this._appendTextNode("responseBodyUnknownTypeContent",
      this._format("responseBodyUnableToDisplay.content", [this.contentType]));
  },

  





  _displayNoResponseBody: function NP_displayNoResponseBody()
  {
    let timing = this.httpActivity.timings;

    this._displayNode("responseNoBody");
    this._appendTextNode("responseNoBodyInfo",
      this._format("durationMS", [timing.receive]));
  },

  




  update: function NP_update()
  {
    
    
    if (!this.document || !this.document.getElementById("headUrl")) {
      return;
    }

    let updates = this.httpActivity.updates;
    let timing = this.httpActivity.timings;
    let request = this.httpActivity.request;
    let response = this.httpActivity.response;

    switch (this._state) {
      case this._INIT:
        this._displayRequestHeader();
        this._state = this._DISPLAYED_REQUEST_HEADER;
        

      case this._DISPLAYED_REQUEST_HEADER:
        
        if (!this.httpActivity.discardRequestBody && request.postData.text) {
          
          if (this._isRequestBodyFormData) {
            this._displayRequestForm();
          }
          else {
            this._displayRequestBody();
          }
          this._state = this._DISPLAYED_REQUEST_BODY;
        }
        

      case this._DISPLAYED_REQUEST_BODY:
        if (!response.headers.length || !Object.keys(timing).length) {
          break;
        }
        this._displayResponseHeader();
        this._state = this._DISPLAYED_RESPONSE_HEADER;
        

      case this._DISPLAYED_RESPONSE_HEADER:
        if (updates.indexOf("responseContent") == -1 ||
            updates.indexOf("eventTimings") == -1) {
          break;
        }

        this._state = this._TRANSITION_CLOSED;
        if (this.httpActivity.discardResponseBody) {
          break;
        }

        if (!response.content || !response.content.text) {
          this._displayNoResponseBody();
        }
        else if (this._responseIsImage) {
          this._displayResponseImage();
        }
        else if (!this._isResponseBodyTextData) {
          this._displayResponseBodyUnknownType();
        }
        else if (response.content.text) {
          this._displayResponseBody();
        }
        break;
    }

    if (this._onUpdate) {
      this._onUpdate();
    }
  }
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
