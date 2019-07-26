




"use strict";

const EPSILON = 0.001;
const REQUESTS_REFRESH_RATE = 50; 
const REQUESTS_WATERFALL_SAFE_BOUNDS = 100; 
const REQUESTS_WATERFALL_BACKGROUND_PATTERN = [5, 250, 1000, 2000]; 
const DEFAULT_HTTP_VERSION = "HTTP/1.1";
const HEADERS_SIZE_DECIMALS = 3;
const CONTENT_SIZE_DECIMALS = 2;
const CONTENT_MIME_TYPE_ABBREVIATIONS = {
  "ecmascript": "js",
  "javascript": "js",
  "x-javascript": "js"
};
const CONTENT_MIME_TYPE_MAPPINGS = {
  "/ecmascript": SourceEditor.MODES.JAVASCRIPT,
  "/javascript": SourceEditor.MODES.JAVASCRIPT,
  "/x-javascript": SourceEditor.MODES.JAVASCRIPT,
  "/html": SourceEditor.MODES.HTML,
  "/xhtml": SourceEditor.MODES.HTML,
  "/xml": SourceEditor.MODES.HTML,
  "/atom": SourceEditor.MODES.HTML,
  "/soap": SourceEditor.MODES.HTML,
  "/rdf": SourceEditor.MODES.HTML,
  "/rss": SourceEditor.MODES.HTML,
  "/css": SourceEditor.MODES.CSS
};
const DEFAULT_EDITOR_CONFIG = {
  mode: SourceEditor.MODES.TEXT,
  readOnly: true,
  showLineNumbers: true
};
const GENERIC_VARIABLES_VIEW_SETTINGS = {
  lazyEmpty: false,
  lazyEmptyDelay: 10, 
  searchEnabled: true,
  descriptorTooltip: false,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChage: true,
  eval: () => {},
  switch: () => {}
};

function $(aSelector, aTarget = document) aTarget.querySelector(aSelector);




let NetMonitorView = {
  





  initialize: function NV_initialize(aCallback) {
    dumpn("Initializing the NetMonitorView");

    this._initializePanes();

    this.Toolbar.initialize();
    this.RequestsMenu.initialize();
    this.NetworkDetails.initialize();

    aCallback();
  },

  





  destroy: function NV_destroy(aCallback) {
    dumpn("Destroying the NetMonitorView");

    this.Toolbar.destroy();
    this.RequestsMenu.destroy();
    this.NetworkDetails.destroy();

    this._destroyPanes();

    aCallback();
  },

  


  _initializePanes: function DV__initializePanes() {
    dumpn("Initializing the NetMonitorView panes");

    this._detailsPane = $("#details-pane");
    this._detailsPaneToggleButton = $("#details-pane-toggle");

    this._collapsePaneString = L10N.getStr("collapseDetailsPane");
    this._expandPaneString = L10N.getStr("expandDetailsPane");

    this._detailsPane.setAttribute("width", Prefs.networkDetailsWidth);
    this.toggleDetailsPane({ visible: false });
  },

  


  _destroyPanes: function DV__destroyPanes() {
    dumpn("Destroying the NetMonitorView panes");

    Prefs.networkDetailsWidth = this._detailsPane.getAttribute("width");

    this._detailsPane = null;
    this._detailsPaneToggleButton = null;
  },

  



  get detailsPaneHidden()
    this._detailsPane.hasAttribute("pane-collapsed"),

  











  toggleDetailsPane: function DV__toggleDetailsPane(aFlags, aTabIndex) {
    let pane = this._detailsPane;
    let button = this._detailsPaneToggleButton;

    ViewHelpers.togglePane(aFlags, pane);

    if (aFlags.visible) {
      button.removeAttribute("pane-collapsed");
      button.setAttribute("tooltiptext", this._collapsePaneString);
    } else {
      button.setAttribute("pane-collapsed", "");
      button.setAttribute("tooltiptext", this._expandPaneString);
    }

    if (aTabIndex !== undefined) {
      $("#details-pane").selectedIndex = aTabIndex;
    }
  },
  







  editor: function NV_editor(aId) {
    dumpn("Getting a NetMonitorView editor: " + aId);

    if (this._editorPromises.has(aId)) {
      return this._editorPromises.get(aId);
    }

    let deferred = Promise.defer();
    this._editorPromises.set(aId, deferred.promise);

    
    
    new SourceEditor().init($(aId), DEFAULT_EDITOR_CONFIG, deferred.resolve);

    return deferred.promise;
  },

  _editorPromises: new Map(),
  _collapsePaneString: "",
  _expandPaneString: "",
  _isInitialized: false,
  _isDestroyed: false
};




function ToolbarView() {
  dumpn("ToolbarView was instantiated");

  this._onTogglePanesPressed = this._onTogglePanesPressed.bind(this);
}

ToolbarView.prototype = {
  


  initialize: function NVT_initialize() {
    dumpn("Initializing the ToolbarView");

    this._detailsPaneToggleButton = $("#details-pane-toggle");
    this._detailsPaneToggleButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
  },

  


  destroy: function NVT_destroy() {
    dumpn("Destroying the ToolbarView");

    this._detailsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
  },

  


  _onTogglePanesPressed: function NVT__onTogglePanesPressed() {
    let requestsMenu = NetMonitorView.RequestsMenu;
    let networkDetails = NetMonitorView.NetworkDetails;

    
    
    if (!requestsMenu.selectedItem && requestsMenu.itemCount) {
      requestsMenu.selectedIndex = 0;
    }
    
    else {
      networkDetails.toggle(NetMonitorView.detailsPaneHidden);
    }
  },

  _detailsPaneToggleButton: null
};






function RequestsMenuView() {
  dumpn("RequestsMenuView was instantiated");

  this._cache = new Map(); 
  this._flushRequests = this._flushRequests.bind(this);
  this._onRequestItemRemoved = this._onRequestItemRemoved.bind(this);
  this._onMouseDown = this._onMouseDown.bind(this);
  this._onSelect = this._onSelect.bind(this);
  this._onResize = this._onResize.bind(this);
}

create({ constructor: RequestsMenuView, proto: MenuContainer.prototype }, {
  


  initialize: function NVRM_initialize() {
    dumpn("Initializing the RequestsMenuView");

    this.node = new SideMenuWidget($("#requests-menu-contents"), false);

    this.node.addEventListener("mousedown", this._onMouseDown, false);
    this.node.addEventListener("select", this._onSelect, false);
    window.addEventListener("resize", this._onResize, false);
  },

  


  destroy: function NVRM_destroy() {
    dumpn("Destroying the SourcesView");

    this.node.removeEventListener("mousedown", this._onMouseDown, false);
    this.node.removeEventListener("select", this._onSelect, false);
    window.removeEventListener("resize", this._onResize, false);
  },

  


  reset: function NVRM_reset() {
    this.empty();
    this._firstRequestStartedMillis = -1;
    this._lastRequestEndedMillis = -1;
  },

  


  lazyUpdate: true,

  












  addRequest: function NVRM_addRequest(aId, aStartedDateTime, aMethod, aUrl) {
    
    let unixTime = Date.parse(aStartedDateTime);

    
    let menuView = this._createMenuView(aMethod, aUrl);

    
    this._registerFirstRequestStart(unixTime);
    this._registerLastRequestEnd(unixTime);

    
    let requestItem = this.push(menuView, {
      relaxed: true, 
      attachment: {
        id: aId,
        startedDeltaMillis: unixTime - this._firstRequestStartedMillis,
        startedMillis: unixTime,
        method: aMethod,
        url: aUrl
      },
      finalize: this._onRequestItemRemoved
    });

    $(".requests-menu-empty-notice").hidden = true;
    this._cache.set(aId, requestItem);
  },

  








  updateRequest: function NVRM_updateRequest(aId, aData) {
    
    if (NetMonitorView._isDestroyed) {
      return;
    }
    this._updateQueue.push([aId, aData]);

    
    if (!this.lazyUpdate) {
      return void this._flushRequests();
    }
    window.clearTimeout(this._updateTimeout);
    this._updateTimeout = window.setTimeout(this._flushRequests, REQUESTS_REFRESH_RATE);
  },

  


  _flushRequests: function NVRM__flushRequests() {
    
    
    for (let [id, data] of this._updateQueue) {
      let requestItem = this._cache.get(id);
      if (!requestItem) {
        
        continue;
      }

      
      
      for (let key in data) {
        let value = data[key];
        if (value === undefined) {
          
          continue;
        }

        switch (key) {
          case "requestHeaders":
            requestItem.attachment.requestHeaders = value;
            break;
          case "requestCookies":
            requestItem.attachment.requestCookies = value;
            break;
          case "requestPostData":
            requestItem.attachment.requestPostData = value;
            break;
          case "responseHeaders":
            requestItem.attachment.responseHeaders = value;
            break;
          case "responseCookies":
            requestItem.attachment.responseCookies = value;
            break;
          case "httpVersion":
            requestItem.attachment.httpVersion = value;
            break;
          case "status":
            requestItem.attachment.status = value;
            this._updateMenuView(requestItem, key, value);
            break;
          case "statusText":
            requestItem.attachment.statusText = value;
            break;
          case "headersSize":
            requestItem.attachment.headersSize = value;
            break;
          case "contentSize":
            requestItem.attachment.contentSize = value;
            this._updateMenuView(requestItem, key, value);
            break;
          case "mimeType":
            requestItem.attachment.mimeType = value;
            this._updateMenuView(requestItem, key, value);
            break;
          case "responseContent":
            requestItem.attachment.responseContent = value;
            break;
          case "totalTime":
            requestItem.attachment.totalTime = value;
            requestItem.attachment.endedMillis = requestItem.attachment.startedMillis + value;
            this._updateMenuView(requestItem, key, value);
            this._registerLastRequestEnd(requestItem.attachment.endedMillis);
            break;
          case "eventTimings":
            requestItem.attachment.eventTimings = value;
            this._createWaterfallView(requestItem, value.timings);
            break;
        }
      }
      
      
      let selectedItem = this.selectedItem;
      if (selectedItem && selectedItem.attachment.id == id) {
        NetMonitorView.NetworkDetails.populate(selectedItem.attachment);
      }
    }
    
    this._updateQueue = [];
  },

  









  _createMenuView: function NVRM__createMenuView(aMethod, aUrl) {
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let name = NetworkHelper.convertToUnicode(unescape(uri.fileName)) || "/";
    let query = NetworkHelper.convertToUnicode(unescape(uri.query));
    let hostPort = NetworkHelper.convertToUnicode(unescape(uri.hostPort));

    let template = $("#requests-menu-item-template");
    let fragment = document.createDocumentFragment();

    $(".requests-menu-method", template)
      .setAttribute("value", aMethod);

    $(".requests-menu-file", template)
      .setAttribute("value", name + (query ? "?" + query : ""));

    $(".requests-menu-domain", template)
      .setAttribute("value", hostPort);

    
    for (let node of template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    return fragment;
  },

  









  _updateMenuView: function NVRM__updateMenuView(aItem, aKey, aValue) {
    switch (aKey) {
      case "status": {
        let node = $(".requests-menu-status", aItem.target);
        node.setAttribute("code", aValue);
        break;
      }
      case "contentSize": {
        let size = (aValue / 1024).toFixed(CONTENT_SIZE_DECIMALS);
        let node = $(".requests-menu-size", aItem.target);
        node.setAttribute("value", L10N.getFormatStr("networkMenu.sizeKB", size));
        break;
      }
      case "mimeType": {
        let type = aValue.split(";")[0].split("/")[1] || "?";
        let node = $(".requests-menu-type", aItem.target);
        node.setAttribute("value", CONTENT_MIME_TYPE_ABBREVIATIONS[type] || type);
        break;
      }
      case "totalTime": {
        let node = $(".requests-menu-timings-total", aItem.target);
        node.setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", aValue));
        break;
      }
    }
  },

  







  _createWaterfallView: function NVRM__createWaterfallView(aItem, aTimings) {
    let { target, attachment } = aItem;
    let sections = ["blocked", "dns", "connect", "send", "wait", "receive"];

    let timingsNode = $(".requests-menu-timings", target);
    let startCapNode = $(".requests-menu-timings-cap.start", timingsNode);
    let endCapNode = $(".requests-menu-timings-cap.end", timingsNode);
    let firstBox;

    
    for (let key of sections) {
      let width = aTimings[key];

      
      
      if (width > 0) {
        let timingBox = document.createElement("hbox");
        timingBox.className = "requests-menu-timings-box " + key;
        timingBox.setAttribute("width", width);
        timingsNode.insertBefore(timingBox, endCapNode);

        
        if (!firstBox) {
          firstBox = timingBox;
          startCapNode.classList.add(key);
        }
        
        endCapNode.classList.add(key);
      }
    }

    
    
    startCapNode.hidden = false;
    endCapNode.hidden = false;

    
    this._flushWaterfallViews();
  },

  





  _flushWaterfallViews: function NVRM__flushWaterfallViews(aReset) {
    
    
    
    if (aReset) {
      this._cachedWaterfallWidth = 0;
    }

    
    
    let availableWidth = this._waterfallWidth - REQUESTS_WATERFALL_SAFE_BOUNDS;
    let longestWidth = this._lastRequestEndedMillis - this._firstRequestStartedMillis;
    let scale = Math.min(Math.max(availableWidth / longestWidth, EPSILON), 1);

    
    
    for (let [, { target, attachment }] of this._cache) {
      let timingsNode = $(".requests-menu-timings", target);
      let startCapNode = $(".requests-menu-timings-cap.start", target);
      let endCapNode = $(".requests-menu-timings-cap.end", target);
      let totalNode = $(".requests-menu-timings-total", target);

      
      
      let translateX = "translateX(" + attachment.startedDeltaMillis + "px)";

      
      
      let scaleX = "scaleX(" + scale + ")";

      
      
      let revScaleX = "scaleX(" + (1 / scale) + ")";

      timingsNode.style.transform = scaleX + " " + translateX;
      startCapNode.style.transform = revScaleX + " translateX(0.5px)";
      endCapNode.style.transform = revScaleX + " translateX(-0.5px)";
      totalNode.style.transform = revScaleX;
    }
  },

  





  _onRequestItemRemoved: function NVRM__onRequestItemRemoved(aItem) {
    dumpn("Finalizing network request item: " + aItem);
    this._cache.delete(aItem.attachment.id);
  },

  


  _onMouseDown: function NVRM__onMouseDown(e) {
    let item = this.getItemForElement(e.target);
    if (item) {
      
      this.selectedItem = item;
    }
  },

  


  _onSelect: function NVRM__onSelect(e) {
    NetMonitorView.NetworkDetails.populate(this.selectedItem.attachment);
    NetMonitorView.NetworkDetails.toggle(true);
  },

  


  _onResize: function NVRM__onResize(e) {
    this._flushWaterfallViews(true);
  },

  






  _registerFirstRequestStart: function NVRM__registerFirstRequestStart(aUnixTime) {
    if (this._firstRequestStartedMillis == -1) {
      this._firstRequestStartedMillis = aUnixTime;
    }
  },

  






  _registerLastRequestEnd: function NVRM__registerLastRequestEnd(aUnixTime) {
    if (this._lastRequestEndedMillis < aUnixTime) {
      this._lastRequestEndedMillis = aUnixTime;
    }
  },

  



  get _waterfallWidth() {
    if (this._cachedWaterfallWidth == 0) {
      let container = $("#requests-menu-toolbar");
      let waterfall = $("#requests-menu-waterfall-label");
      let containerBounds = container.getBoundingClientRect();
      let waterfallBounds = waterfall.getBoundingClientRect();
      this._cachedWaterfallWidth = containerBounds.width - waterfallBounds.left;
    }
    return this._cachedWaterfallWidth;
  },

  _cache: null,
  _cachedWaterfallWidth: 0,
  _firstRequestStartedMillis: -1,
  _lastRequestEndedMillis: -1,
  _updateQueue: [],
  _updateTimeout: null
});




function NetworkDetailsView() {
  dumpn("NetworkDetailsView was instantiated");
};

create({ constructor: NetworkDetailsView, proto: MenuContainer.prototype }, {
  


  initialize: function NVND_initialize() {
    dumpn("Initializing the RequestsMenuView");

    this.node = $("#details-pane");

    this._headers = new VariablesView($("#all-headers"),
      Object.create(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: { value: L10N.getStr("headersEmptyText"), enumerable: true },
        searchPlaceholder: { value: L10N.getStr("headersFilterText"), enumerable: true }
      }));
    this._cookies = new VariablesView($("#all-cookies"),
      Object.create(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: { value: L10N.getStr("cookiesEmptyText"), enumerable: true },
        searchPlaceholder: { value: L10N.getStr("cookiesFilterText"), enumerable: true }
      }));
    this._params = new VariablesView($("#request-params"),
      Object.create(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: { value: L10N.getStr("paramsEmptyText"), enumerable: true },
        searchPlaceholder: { value: L10N.getStr("paramsFilterText"), enumerable: true }
      }));
    this._json = new VariablesView($("#response-content-json"),
      Object.create(GENERIC_VARIABLES_VIEW_SETTINGS, {
        searchPlaceholder: { value: L10N.getStr("jsonFilterText"), enumerable: true }
      }));

    this._paramsQueryString = L10N.getStr("paramsQueryString");
    this._paramsFormData = L10N.getStr("paramsFormData");
    this._paramsPostPayload = L10N.getStr("paramsPostPayload");
    this._requestHeaders = L10N.getStr("requestHeaders");
    this._responseHeaders = L10N.getStr("responseHeaders");
    this._requestCookies = L10N.getStr("requestCookies");
    this._responseCookies = L10N.getStr("responseCookies");
  },

  


  destroy: function NVND_destroy() {
    dumpn("Destroying the SourcesView");
  },

  





  toggle: function NVND_toggle(aVisibleFlag) {
    NetMonitorView.toggleDetailsPane({ visible: aVisibleFlag });
    NetMonitorView.RequestsMenu._flushWaterfallViews(true);
  },

  





  populate: function NVND_populate(aData) {
    $("#request-params-box").setAttribute("flex", "1");
    $("#request-params-box").hidden = false;
    $("#request-post-data-textarea-box").hidden = true;
    $("#response-content-json-box").hidden = true;
    $("#response-content-textarea-box").hidden = true;
    $("#response-content-image-box").hidden = true;

    this._headers.empty();
    this._cookies.empty();
    this._params.empty();
    this._json.empty();

    if (aData) {
      this._setSummary(aData);
      this._setResponseHeaders(aData.responseHeaders);
      this._setRequestHeaders(aData.requestHeaders);
      this._setResponseCookies(aData.responseCookies);
      this._setRequestCookies(aData.requestCookies);
      this._setRequestGetParams(aData.url);
      this._setRequestPostParams(aData.requestHeaders, aData.requestPostData);
      this._setResponseBody(aData.url, aData.responseContent);
      this._setTimingsInformation(aData.eventTimings);
    }
  },

  





  _setSummary: function NVND__setSummary(aData) {
    if (aData.url) {
      let unicodeUrl = NetworkHelper.convertToUnicode(unescape(aData.url));
      $("#headers-summary-url-value").setAttribute("value", unicodeUrl);
      $("#headers-summary-url").removeAttribute("hidden");
    } else {
      $("#headers-summary-url").setAttribute("hidden", "true");
    }

    if (aData.method) {
      $("#headers-summary-method-value").setAttribute("value", aData.method);
      $("#headers-summary-method").removeAttribute("hidden");
    } else {
      $("#headers-summary-method").setAttribute("hidden", "true");
    }

    if (aData.status) {
      $("#headers-summary-status-circle").setAttribute("code", aData.status);
      $("#headers-summary-status-value").setAttribute("value", aData.status + " " + aData.statusText);
      $("#headers-summary-status").removeAttribute("hidden");
    } else {
      $("#headers-summary-status").setAttribute("hidden", "true");
    }

    if (aData.httpVersion && aData.httpVersion != DEFAULT_HTTP_VERSION) {
      $("#headers-summary-version-value").setAttribute("value", aData.httpVersion);
      $("#headers-summary-version").removeAttribute("hidden");
    } else {
      $("#headers-summary-version").setAttribute("hidden", "true");
    }
  },

  





  _setRequestHeaders: function NVND__setRequestHeaders(aResponse) {
    if (aResponse && aResponse.headers.length) {
      this._addHeaders(this._requestHeaders, aResponse);
    }
  },

  





  _setResponseHeaders: function NVND__setResponseHeaders(aResponse) {
    if (aResponse && aResponse.headers.length) {
      aResponse.headers.sort((a, b) => a.name > b.name);
      this._addHeaders(this._responseHeaders, aResponse);
    }
  },

  







  _addHeaders: function NVND__addHeaders(aName, aResponse) {
    let kb = (aResponse.headersSize / 1024).toFixed(HEADERS_SIZE_DECIMALS);
    let size = L10N.getFormatStr("networkMenu.sizeKB", kb);
    let headersScope = this._headers.addScope(aName + " (" + size + ")");
    headersScope.expanded = true;

    for (let header of aResponse.headers) {
      let headerVar = headersScope.addVar(header.name, { null: true }, true);
      headerVar.setGrip(header.value);
    }
  },

  





  _setRequestCookies: function NVND__setRequestCookies(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      aResponse.cookies.sort((a, b) => a.name > b.name);
      this._addCookies(this._requestCookies, aResponse);
    }
  },

  





  _setResponseCookies: function NVND__setResponseCookies(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      this._addCookies(this._responseCookies, aResponse);
    }
  },

  







  _addCookies: function NVND__addCookies(aName, aResponse) {
    let cookiesScope = this._cookies.addScope(aName);
    cookiesScope.expanded = true;

    for (let cookie of aResponse.cookies) {
      let cookieVar = cookiesScope.addVar(cookie.name, { null: true }, true);
      cookieVar.setGrip(cookie.value);

      
      
      let cookieProps = Object.keys(cookie);
      if (cookieProps.length == 2) {
        continue;
      }

      
      
      let rawObject = Object.create(null);
      let otherProps = cookieProps.filter((e) => e != "name" && e != "value");
      for (let prop of otherProps) {
        rawObject[prop] = cookie[prop];
      }
      cookieVar.populate(rawObject);
      cookieVar.twisty = true;
      cookieVar.expanded = true;
    }
  },

  





  _setRequestGetParams: function NVND__setRequestGetParams(aUrl) {
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let query = uri.query;
    if (query) {
      this._addParams(this._paramsQueryString, query.split("&").map((e) =>
        let (param = e.split("=")) {
          name: param[0],
          value: NetworkHelper.convertToUnicode(unescape(param[1]))
        }
      ));
    }
  },

  







  _setRequestPostParams: function NVND__setRequestPostParams(aHeadersResponse, aPostResponse) {
    if (!aHeadersResponse || !aPostResponse) {
      return;
    }
    let contentType = aHeadersResponse.headers.filter(({ name }) => name == "Content-Type")[0];
    let text = aPostResponse.postData.text;

    if (contentType.value.contains("x-www-form-urlencoded")) {
      this._addParams(this._paramsFormData, text.replace(/^[?&]/, "").split("&").map((e) =>
        let (param = e.split("=")) {
          name: param[0],
          value: NetworkHelper.convertToUnicode(unescape(param[1]))
        }
      ));
    } else {
      
      
      
      $("#request-params-box").removeAttribute("flex");
      let paramsScope = this._params.addScope(this._paramsPostPayload);
      paramsScope.expanded = true;
      paramsScope.locked = true;

      $("#request-post-data-textarea-box").hidden = false;
      NetMonitorView.editor("#request-post-data-textarea").then((aEditor) => {
        aEditor.setText(text);
      });
    }
  },

  







  _addParams: function NVND__addParams(aName, aParams) {
    let paramsScope = this._params.addScope(aName);
    paramsScope.expanded = true;

    for (let param of aParams) {
      let headerVar = paramsScope.addVar(param.name, { null: true }, true);
      headerVar.setGrip(param.value);
    }
  },

  







  _setResponseBody: function NVND__setresponseBody(aUrl, aResponse) {
    if (!aResponse) {
      return;
    }
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let { mimeType: mime, text, encoding } = aResponse.content;

    if (mime.contains("/json")) {
      $("#response-content-json-box").hidden = false;
      let jsonScope = this._json.addScope("JSON");
      jsonScope.addVar().populate(JSON.parse(text), { expanded: true });
      jsonScope.expanded = true;
    }
    else if (mime.contains("image/")) {
      $("#response-content-image-box").setAttribute("align", "center");
      $("#response-content-image-box").setAttribute("pack", "center");
      $("#response-content-image-box").hidden = false;
      $("#response-content-image").src = "data:" + mime + ";" + encoding + "," + text;

      
      
      $("#response-content-image-name-value").setAttribute("value", uri.fileName);
      $("#response-content-image-mime-value").setAttribute("value", mime);
      $("#response-content-image-encoding-value").setAttribute("value", encoding);

      
      $("#response-content-image").onload = (e) => {
        
        
        
        let { width, height } = e.target.getBoundingClientRect();
        let dimensions = (width - 2) + " x " + (height - 2);
        $("#response-content-image-dimensions-value").setAttribute("value", dimensions);
      };
    }
    else {
      $("#response-content-textarea-box").hidden = false;
      NetMonitorView.editor("#response-content-textarea").then((aEditor) => {
        aEditor.setMode(SourceEditor.MODES.TEXT);
        aEditor.setText(typeof text == "string" ? text : text.initial);

        
        for (let key in CONTENT_MIME_TYPE_MAPPINGS) {
          if (mime.contains(key)) {
            aEditor.setMode(CONTENT_MIME_TYPE_MAPPINGS[key]);
            break;
          }
        }
      });
    }
  },

  





  _setTimingsInformation: function NVND__setTimingsInformation(aResponse) {
    if (!aResponse) {
      return;
    }
    let { blocked, dns, connect, send, wait, receive } = aResponse.timings;

    let tabboxWidth = $("#details-pane").getAttribute("width");
    let availableWidth = tabboxWidth / 2; 
    let scale = Math.max(availableWidth / aResponse.totalTime, 0);

    $("#timings-summary-blocked .requests-menu-timings-box")
      .setAttribute("width", blocked * scale);
    $("#timings-summary-blocked .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", blocked));

    $("#timings-summary-dns .requests-menu-timings-box")
      .setAttribute("width", dns * scale);
    $("#timings-summary-dns .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", dns));

    $("#timings-summary-connect .requests-menu-timings-box")
      .setAttribute("width", connect * scale);
    $("#timings-summary-connect .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", connect));

    $("#timings-summary-send .requests-menu-timings-box")
      .setAttribute("width", send * scale);
    $("#timings-summary-send .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", send));

    $("#timings-summary-wait .requests-menu-timings-box")
      .setAttribute("width", wait * scale);
    $("#timings-summary-wait .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", wait));

    $("#timings-summary-receive .requests-menu-timings-box")
      .setAttribute("width", receive * scale);
    $("#timings-summary-receive .requests-menu-timings-total")
      .setAttribute("value", L10N.getFormatStr("networkMenu.totalMS", receive));

    $("#timings-summary-dns .requests-menu-timings-box")
      .style.transform = "translateX(" + (scale * blocked) + "px)";
    $("#timings-summary-connect .requests-menu-timings-box")
      .style.transform = "translateX(" + (scale * (blocked + dns)) + "px)";
    $("#timings-summary-send .requests-menu-timings-box")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect)) + "px)";
    $("#timings-summary-wait .requests-menu-timings-box")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect + send)) + "px)";
    $("#timings-summary-receive .requests-menu-timings-box")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect + send + wait)) + "px)";

    $("#timings-summary-dns .requests-menu-timings-total")
      .style.transform = "translateX(" + (scale * blocked) + "px)";
    $("#timings-summary-connect .requests-menu-timings-total")
      .style.transform = "translateX(" + (scale * (blocked + dns)) + "px)";
    $("#timings-summary-send .requests-menu-timings-total")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect)) + "px)";
    $("#timings-summary-wait .requests-menu-timings-total")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect + send)) + "px)";
    $("#timings-summary-receive .requests-menu-timings-total")
      .style.transform = "translateX(" + (scale * (blocked + dns + connect + send + wait)) + "px)";
  },

  _headers: null,
  _cookies: null,
  _params: null,
  _json: null,
  _paramsQueryString: "",
  _paramsFormData: "",
  _paramsPostPayload: "",
  _requestHeaders: "",
  _responseHeaders: "",
  _requestCookies: "",
  _responseCookies: ""
});




NetMonitorView.Toolbar = new ToolbarView();
NetMonitorView.RequestsMenu = new RequestsMenuView();
NetMonitorView.NetworkDetails = new NetworkDetailsView();
