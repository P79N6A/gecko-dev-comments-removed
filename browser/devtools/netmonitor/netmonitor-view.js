




"use strict";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const EPSILON = 0.001;
const RESIZE_REFRESH_RATE = 50; 
const REQUESTS_REFRESH_RATE = 50; 
const REQUESTS_HEADERS_SAFE_BOUNDS = 30; 
const REQUESTS_WATERFALL_SAFE_BOUNDS = 90; 
const REQUESTS_WATERFALL_HEADER_TICKS_MULTIPLE = 5; 
const REQUESTS_WATERFALL_HEADER_TICKS_SPACING_MIN = 60; 
const REQUESTS_WATERFALL_BACKGROUND_TICKS_MULTIPLE = 5; 
const REQUESTS_WATERFALL_BACKGROUND_TICKS_SCALES = 3;
const REQUESTS_WATERFALL_BACKGROUND_TICKS_SPACING_MIN = 10; 
const REQUESTS_WATERFALL_BACKGROUND_TICKS_COLOR_RGB = [128, 136, 144];
const REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_MIN = 32; 
const REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_ADD = 32; 
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




let NetMonitorView = {
  





  initialize: function(aCallback) {
    dumpn("Initializing the NetMonitorView");

    this._initializePanes();

    this.Toolbar.initialize();
    this.RequestsMenu.initialize();
    this.NetworkDetails.initialize();

    aCallback();
  },

  





  destroy: function(aCallback) {
    dumpn("Destroying the NetMonitorView");

    this.Toolbar.destroy();
    this.RequestsMenu.destroy();
    this.NetworkDetails.destroy();

    this._destroyPanes();

    aCallback();
  },

  


  _initializePanes: function() {
    dumpn("Initializing the NetMonitorView panes");

    this._detailsPane = $("#details-pane");
    this._detailsPaneToggleButton = $("#details-pane-toggle");

    this._collapsePaneString = L10N.getStr("collapseDetailsPane");
    this._expandPaneString = L10N.getStr("expandDetailsPane");

    this._detailsPane.setAttribute("width", Prefs.networkDetailsWidth);
    this._detailsPane.setAttribute("height", Prefs.networkDetailsHeight);
    this.toggleDetailsPane({ visible: false });
  },

  


  _destroyPanes: function() {
    dumpn("Destroying the NetMonitorView panes");

    Prefs.networkDetailsWidth = this._detailsPane.getAttribute("width");
    Prefs.networkDetailsHeight = this._detailsPane.getAttribute("height");

    this._detailsPane = null;
    this._detailsPaneToggleButton = null;
  },

  



  get detailsPaneHidden()
    this._detailsPane.hasAttribute("pane-collapsed"),

  











  toggleDetailsPane: function(aFlags, aTabIndex) {
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

  







  editor: function(aId) {
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
  


  initialize: function() {
    dumpn("Initializing the ToolbarView");

    this._detailsPaneToggleButton = $("#details-pane-toggle");
    this._detailsPaneToggleButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
  },

  


  destroy: function() {
    dumpn("Destroying the ToolbarView");

    this._detailsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
  },

  


  _onTogglePanesPressed: function() {
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
  


  initialize: function() {
    dumpn("Initializing the RequestsMenuView");

    this.node = new SideMenuWidget($("#requests-menu-contents"), false);
    this.node.maintainSelectionVisible = false;
    this.node.autoscrollWithAppendedItems = true;

    this.node.addEventListener("mousedown", this._onMouseDown, false);
    this.node.addEventListener("select", this._onSelect, false);
    window.addEventListener("resize", this._onResize, false);
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");

    this.node.removeEventListener("mousedown", this._onMouseDown, false);
    this.node.removeEventListener("select", this._onSelect, false);
    window.removeEventListener("resize", this._onResize, false);
  },

  


  reset: function() {
    this.empty();
    this._firstRequestStartedMillis = -1;
    this._lastRequestEndedMillis = -1;
  },

  


  lazyUpdate: true,

  












  addRequest: function(aId, aStartedDateTime, aMethod, aUrl) {
    
    let unixTime = Date.parse(aStartedDateTime);

    
    let menuView = this._createMenuView(aMethod, aUrl);

    
    this._registerFirstRequestStart(unixTime);
    this._registerLastRequestEnd(unixTime);

    
    let requestItem = this.push(menuView, {
      attachment: {
        id: aId,
        startedDeltaMillis: unixTime - this._firstRequestStartedMillis,
        startedMillis: unixTime,
        method: aMethod,
        url: aUrl
      },
      finalize: this._onRequestItemRemoved
    });

    $("#details-pane-toggle").disabled = false;
    $(".requests-menu-empty-notice").hidden = true;

    this._cache.set(aId, requestItem);
  },

  





  sortBy: function(aType) {
    let target = $("#requests-menu-" + aType + "-button");
    let headers = document.querySelectorAll(".requests-menu-header-button");

    for (let header of headers) {
      if (header != target) {
        header.removeAttribute("sorted");
        header.removeAttribute("tooltiptext");
      }
    }

    let direction = "";
    if (target) {
      if (!target.hasAttribute("sorted")) {
        target.setAttribute("sorted", direction = "ascending");
        target.setAttribute("tooltiptext", L10N.getStr("networkMenu.sortedAsc"));
      } else if (target.getAttribute("sorted") == "ascending") {
        target.setAttribute("sorted", direction = "descending");
        target.setAttribute("tooltiptext", L10N.getStr("networkMenu.sortedDesc"));
      } else {
        target.removeAttribute("sorted");
        target.removeAttribute("tooltiptext");
      }
    }

    
    if (!target || !direction) {
      this.sortContents(this._byTiming);
    }
    
    else switch (aType) {
      case "status":
        if (direction == "ascending") {
          this.sortContents(this._byStatus);
        } else {
          this.sortContents((a, b) => !this._byStatus(a, b));
        }
        break;
      case "method":
        if (direction == "ascending") {
          this.sortContents(this._byMethod);
        } else {
          this.sortContents((a, b) => !this._byMethod(a, b));
        }
        break;
      case "file":
        if (direction == "ascending") {
          this.sortContents(this._byFile);
        } else {
          this.sortContents((a, b) => !this._byFile(a, b));
        }
        break;
      case "domain":
        if (direction == "ascending") {
          this.sortContents(this._byDomain);
        } else {
          this.sortContents((a, b) => !this._byDomain(a, b));
        }
        break;
      case "type":
        if (direction == "ascending") {
          this.sortContents(this._byType);
        } else {
          this.sortContents((a, b) => !this._byType(a, b));
        }
        break;
      case "size":
        if (direction == "ascending") {
          this.sortContents(this._bySize);
        } else {
          this.sortContents((a, b) => !this._bySize(a, b));
        }
        break;
    }
  },

  











  _byTiming: (aFirst, aSecond) =>
    aFirst.attachment.startedMillis > aSecond.attachment.startedMillis,

  _byStatus: (aFirst, aSecond) =>
    aFirst.attachment.status > aSecond.attachment.status,

  _byMethod: (aFirst, aSecond) =>
    aFirst.attachment.method > aSecond.attachment.method,

  _byFile: (aFirst, aSecond) =>
    !aFirst.target || !aSecond.target ? -1 :
      $(".requests-menu-file", aFirst.target).getAttribute("value").toLowerCase() >
      $(".requests-menu-file", aSecond.target).getAttribute("value").toLowerCase(),

  _byDomain: (aFirst, aSecond) =>
    !aFirst.target || !aSecond.target ? -1 :
      $(".requests-menu-domain", aFirst.target).getAttribute("value").toLowerCase() >
      $(".requests-menu-domain", aSecond.target).getAttribute("value").toLowerCase(),

  _byType: (aFirst, aSecond) =>
    !aFirst.target || !aSecond.target ? -1 :
      $(".requests-menu-type", aFirst.target).getAttribute("value").toLowerCase() >
      $(".requests-menu-type", aSecond.target).getAttribute("value").toLowerCase(),

  _bySize: (aFirst, aSecond) =>
    aFirst.attachment.contentSize > aSecond.attachment.contentSize,

  








  updateRequest: function(aId, aData) {
    
    if (NetMonitorView._isDestroyed) {
      return;
    }
    this._updateQueue.push([aId, aData]);

    
    if (!this.lazyUpdate) {
      return void this._flushRequests();
    }
    
    drain("update-requests", REQUESTS_REFRESH_RATE, () => this._flushRequests());
  },

  


  _flushRequests: function() {
    
    
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
            this._updateMenuView(requestItem, key,
              requestItem.attachment.status + " " +
              requestItem.attachment.statusText);
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

    
    this.sortContents();
  },

  









  _createMenuView: function(aMethod, aUrl) {
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let name = NetworkHelper.convertToUnicode(unescape(uri.fileName)) || "/";
    let query = NetworkHelper.convertToUnicode(unescape(uri.query));
    let hostPort = NetworkHelper.convertToUnicode(unescape(uri.hostPort));

    let template = $("#requests-menu-item-template");
    let fragment = document.createDocumentFragment();

    $(".requests-menu-method", template).setAttribute("value", aMethod);

    let file = $(".requests-menu-file", template);
    file.setAttribute("value", name + (query ? "?" + query : ""));
    file.setAttribute("tooltiptext", name + (query ? "?" + query : ""));

    let domain = $(".requests-menu-domain", template);
    domain.setAttribute("value", hostPort);
    domain.setAttribute("tooltiptext", hostPort);

    
    for (let node of template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    return fragment;
  },

  









  _updateMenuView: function(aItem, aKey, aValue) {
    switch (aKey) {
      case "status": {
        let node = $(".requests-menu-status", aItem.target);
        node.setAttribute("code", aValue);
        break;
      }
      case "statusText": {
        let node = $(".requests-menu-status-and-method", aItem.target);
        node.setAttribute("tooltiptext", aValue);
        break;
      }
      case "contentSize": {
        let kb = aValue / 1024;
        let size = L10N.numberWithDecimals(kb, CONTENT_SIZE_DECIMALS);
        let node = $(".requests-menu-size", aItem.target);
        let text = L10N.getFormatStr("networkMenu.sizeKB", size);
        node.setAttribute("value", text);
        node.setAttribute("tooltiptext", text);
        break;
      }
      case "mimeType": {
        let type = aValue.split(";")[0].split("/")[1] || "?";
        let node = $(".requests-menu-type", aItem.target);
        let text = CONTENT_MIME_TYPE_ABBREVIATIONS[type] || type;
        node.setAttribute("value", text);
        node.setAttribute("tooltiptext", aValue);
        break;
      }
      case "totalTime": {
        let node = $(".requests-menu-timings-total", aItem.target);
        let text = L10N.getFormatStr("networkMenu.totalMS", aValue); 
        node.setAttribute("value", text);
        node.setAttribute("tooltiptext", text);
        break;
      }
    }
  },

  







  _createWaterfallView: function(aItem, aTimings) {
    let { target, attachment } = aItem;
    let sections = ["dns", "connect", "send", "wait", "receive"];
    

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

  





  _flushWaterfallViews: function(aReset) {
    
    
    
    
    if (aReset) {
      this._cachedWaterfallWidth = 0;
      this._hideOverflowingColumns();
    }

    
    
    let availableWidth = this._waterfallWidth - REQUESTS_WATERFALL_SAFE_BOUNDS;
    let longestWidth = this._lastRequestEndedMillis - this._firstRequestStartedMillis;
    let scale = Math.min(Math.max(availableWidth / longestWidth, EPSILON), 1);

    
    this._showWaterfallDivisionLabels(scale);
    this._drawWaterfallBackground(scale);
    this._flushWaterfallBackgrounds();

    
    
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

  





  _showWaterfallDivisionLabels: function(aScale) {
    let container = $("#requests-menu-waterfall-header-box");
    let availableWidth = this._waterfallWidth - REQUESTS_WATERFALL_SAFE_BOUNDS;

    
    while (container.hasChildNodes()) {
      container.firstChild.remove();
    }

    
    let timingStep = REQUESTS_WATERFALL_HEADER_TICKS_MULTIPLE;
    let optimalTickIntervalFound = false;

    while (!optimalTickIntervalFound) {
      
      let scaledStep = aScale * timingStep;
      if (scaledStep < REQUESTS_WATERFALL_HEADER_TICKS_SPACING_MIN) {
        timingStep <<= 1;
        continue;
      }
      optimalTickIntervalFound = true;

      
      let fragment = document.createDocumentFragment();

      for (let x = 0; x < availableWidth; x += scaledStep) {
        let divisionMS = (x / aScale).toFixed(0);
        let translateX = "translateX(" + (x | 0) + "px)";

        let node = document.createElement("label");
        let text = L10N.getFormatStr("networkMenu.divisionMS", divisionMS);
        node.className = "plain requests-menu-timings-division";
        node.style.transform = translateX;

        node.setAttribute("value", text);
        fragment.appendChild(node);
      }
      container.appendChild(fragment);
    }
  },

  





  _drawWaterfallBackground: function(aScale) {
    if (!this._canvas || !this._ctx) {
      this._canvas = document.createElementNS(HTML_NS, "canvas");
      this._ctx = this._canvas.getContext("2d");
    }
    let canvas = this._canvas;
    let ctx = this._ctx;

    
    let canvasWidth = canvas.width = this._waterfallWidth;
    let canvasHeight = canvas.height = 1; 

    
    let imageData = ctx.createImageData(canvasWidth, canvasHeight);
    let pixelArray = imageData.data;

    let buf = new ArrayBuffer(pixelArray.length);
    let buf8 = new Uint8ClampedArray(buf);
    let data32 = new Uint32Array(buf);

    
    let timingStep = REQUESTS_WATERFALL_BACKGROUND_TICKS_MULTIPLE;
    let [r, g, b] = REQUESTS_WATERFALL_BACKGROUND_TICKS_COLOR_RGB;
    let alphaComponent = REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_MIN;
    let optimalTickIntervalFound = false;

    while (!optimalTickIntervalFound) {
      
      let scaledStep = aScale * timingStep;
      if (scaledStep < REQUESTS_WATERFALL_BACKGROUND_TICKS_SPACING_MIN) {
        timingStep <<= 1;
        continue;
      }
      optimalTickIntervalFound = true;

      
      for (let i = 1; i <= REQUESTS_WATERFALL_BACKGROUND_TICKS_SCALES; i++) {
        let increment = scaledStep * Math.pow(2, i);
        for (let x = 0; x < canvasWidth; x += increment) {
          data32[x | 0] = (alphaComponent << 24) | (b << 16) | (g << 8) | r;
        }
        alphaComponent += REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_ADD;
      }
    }

    
    pixelArray.set(buf8);
    ctx.putImageData(imageData, 0, 0);
    this._cachedWaterfallBackground = "url(" + canvas.toDataURL() + ")";
  },

  


  _flushWaterfallBackgrounds: function() {
    for (let [, { target }] of this._cache) {
      let waterfallNode = $(".requests-menu-waterfall", target);
      waterfallNode.style.backgroundImage = this._cachedWaterfallBackground;
    }
  },

  


  _hideOverflowingColumns: function() {
    let table = $("#network-table");
    let toolbar = $("#requests-menu-toolbar");
    let columns = [
      ["#requests-menu-waterfall-header-box", "waterfall-overflows"],
      ["#requests-menu-size-header-box", "size-overflows"],
      ["#requests-menu-type-header-box", "type-overflows"],
      ["#requests-menu-domain-header-box", "domain-overflows"]
    ];

    
    columns.forEach(([, attribute]) => table.removeAttribute(attribute));
    let availableWidth = toolbar.getBoundingClientRect().width;

    
    columns.forEach(([id, attribute]) => {
      let bounds = $(id).getBoundingClientRect();
      if (bounds.right > availableWidth - REQUESTS_HEADERS_SAFE_BOUNDS) {
        table.setAttribute(attribute, "");
      }
    });
  },

  





  _onRequestItemRemoved: function(aItem) {
    dumpn("Finalizing network request item: " + aItem);
    this._cache.delete(aItem.attachment.id);
  },

  


  _onMouseDown: function(e) {
    let item = this.getItemForElement(e.target);
    if (item) {
      
      this.selectedItem = item;
    }
  },

  


  _onSelect: function(e) {
    NetMonitorView.NetworkDetails.populate(this.selectedItem.attachment);
    NetMonitorView.NetworkDetails.toggle(true);
  },

  


  _onResize: function(e) {
    
    drain("resize-events", RESIZE_REFRESH_RATE, () => this._flushWaterfallViews(true));
  },

  






  _registerFirstRequestStart: function(aUnixTime) {
    if (this._firstRequestStartedMillis == -1) {
      this._firstRequestStartedMillis = aUnixTime;
    }
  },

  






  _registerLastRequestEnd: function(aUnixTime) {
    if (this._lastRequestEndedMillis < aUnixTime) {
      this._lastRequestEndedMillis = aUnixTime;
    }
  },

  



  get _waterfallWidth() {
    if (this._cachedWaterfallWidth == 0) {
      let container = $("#requests-menu-toolbar");
      let waterfall = $("#requests-menu-waterfall-header-box");
      let containerBounds = container.getBoundingClientRect();
      let waterfallBounds = waterfall.getBoundingClientRect();
      this._cachedWaterfallWidth = containerBounds.width - waterfallBounds.left;
    }
    return this._cachedWaterfallWidth;
  },

  _cache: null,
  _canvas: null,
  _ctx: null,
  _cachedWaterfallWidth: 0,
  _cachedWaterfallBackground: null,
  _firstRequestStartedMillis: -1,
  _lastRequestEndedMillis: -1,
  _updateQueue: [],
  _updateTimeout: null,
  _resizeTimeout: null
});




function NetworkDetailsView() {
  dumpn("NetworkDetailsView was instantiated");

  this._onTabSelect = this._onTabSelect.bind(this);
};

create({ constructor: NetworkDetailsView, proto: MenuContainer.prototype }, {
  


  initialize: function() {
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

    $("tabpanels", this.node).addEventListener("select", this._onTabSelect);
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");
  },

  





  toggle: function(aVisibleFlag) {
    NetMonitorView.toggleDetailsPane({ visible: aVisibleFlag });
    NetMonitorView.RequestsMenu._flushWaterfallViews(true);
  },

  


  reset: function() {
    this.toggle(false);
    this._dataSrc = null;
  },

  





  populate: function(aData) {
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

    this._dataSrc = { src: aData, populated: [] };
    this._onTabSelect();
  },

  


  _onTabSelect: function() {
    let { src, populated } = this._dataSrc || {};
    let tab = this.node.selectedIndex;

    
    if (!src || populated[tab]) {
      return;
    }

    switch (tab) {
      case 0: 
        this._setSummary(src);
        this._setResponseHeaders(src.responseHeaders);
        this._setRequestHeaders(src.requestHeaders);
        break;
      case 1: 
        this._setResponseCookies(src.responseCookies);
        this._setRequestCookies(src.requestCookies);
        break;
      case 2: 
        this._setRequestGetParams(src.url);
        this._setRequestPostParams(src.requestHeaders, src.requestPostData);
        break;
      case 3: 
        this._setResponseBody(src.url, src.responseContent);
        break;
      case 4: 
        this._setTimingsInformation(src.eventTimings);
        break;
    }

    populated[tab] = true;
  },

  





  _setSummary: function(aData) {
    if (aData.url) {
      let unicodeUrl = NetworkHelper.convertToUnicode(unescape(aData.url));
      $("#headers-summary-url-value").setAttribute("value", unicodeUrl);
      $("#headers-summary-url-value").setAttribute("tooltiptext", unicodeUrl);
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

  





  _setRequestHeaders: function(aResponse) {
    if (aResponse && aResponse.headers.length) {
      this._addHeaders(this._requestHeaders, aResponse);
    }
  },

  





  _setResponseHeaders: function(aResponse) {
    if (aResponse && aResponse.headers.length) {
      aResponse.headers.sort((a, b) => a.name > b.name);
      this._addHeaders(this._responseHeaders, aResponse);
    }
  },

  







  _addHeaders: function(aName, aResponse) {
    let kb = aResponse.headersSize / 1024;
    let size = L10N.numberWithDecimals(kb, HEADERS_SIZE_DECIMALS);
    let text = L10N.getFormatStr("networkMenu.sizeKB", size);
    let headersScope = this._headers.addScope(aName + " (" + text + ")");
    headersScope.expanded = true;

    for (let header of aResponse.headers) {
      let headerVar = headersScope.addVar(header.name, { null: true }, true);
      gNetwork.getString(header.value).then((aString) => headerVar.setGrip(aString));
    }
  },

  





  _setRequestCookies: function(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      aResponse.cookies.sort((a, b) => a.name > b.name);
      this._addCookies(this._requestCookies, aResponse);
    }
  },

  





  _setResponseCookies: function(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      this._addCookies(this._responseCookies, aResponse);
    }
  },

  







  _addCookies: function(aName, aResponse) {
    let cookiesScope = this._cookies.addScope(aName);
    cookiesScope.expanded = true;

    for (let cookie of aResponse.cookies) {
      let cookieVar = cookiesScope.addVar(cookie.name, { null: true }, true);
      gNetwork.getString(cookie.value).then((aString) => cookieVar.setGrip(aString));

      
      
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

  





  _setRequestGetParams: function(aUrl) {
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let query = uri.query;
    if (query) {
      this._addParams(this._paramsQueryString, query);
    }
  },

  







  _setRequestPostParams: function(aHeadersResponse, aPostResponse) {
    if (!aHeadersResponse || !aPostResponse) {
      return;
    }
    let contentType = aHeadersResponse.headers.filter(({ name }) => name == "Content-Type")[0];
    let text = aPostResponse.postData.text;

    gNetwork.getString(text).then((aString) => {
      
      if (contentType.value.contains("x-www-form-urlencoded")) {
        this._addParams(this._paramsFormData, aString);
      }
      
      else {
        
        
        
        $("#request-params-box").removeAttribute("flex");
        let paramsScope = this._params.addScope(this._paramsPostPayload);
        paramsScope.expanded = true;
        paramsScope.locked = true;

        $("#request-post-data-textarea-box").hidden = false;
        NetMonitorView.editor("#request-post-data-textarea").then((aEditor) => {
          aEditor.setText(aString);
        });
      }
      window.emit("NetMonitor:ResponsePostParamsAvailable");
    });
  },

  







  _addParams: function(aName, aParams) {
    
    let paramsArray = aParams.replace(/^[?&]/, "").split("&").map((e) =>
      let (param = e.split("=")) {
        name: NetworkHelper.convertToUnicode(unescape(param[0])),
        value: NetworkHelper.convertToUnicode(unescape(param[1]))
      });

    let paramsScope = this._params.addScope(aName);
    paramsScope.expanded = true;

    for (let param of paramsArray) {
      let headerVar = paramsScope.addVar(param.name, { null: true }, true);
      headerVar.setGrip(param.value);
    }
  },

  







  _setResponseBody: function(aUrl, aResponse) {
    if (!aResponse) {
      return;
    }
    let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    let { mimeType, text, encoding } = aResponse.content;

    gNetwork.getString(text).then((aString) => {
      
      if (mimeType.contains("/json")) {
        $("#response-content-json-box").hidden = false;
        let jsonpRegex = /^[a-zA-Z0-9_$]+\(|\)$/g; 
        let sanitizedJSON = aString.replace(jsonpRegex, "");
        let callbackPadding = aString.match(jsonpRegex);

        let jsonScopeName = callbackPadding
          ? L10N.getFormatStr("jsonpScopeName", callbackPadding[0].slice(0, -1))
          : L10N.getStr("jsonScopeName");

        let jsonScope = this._json.addScope(jsonScopeName);
        jsonScope.addVar().populate(JSON.parse(sanitizedJSON), { expanded: true });
        jsonScope.expanded = true;
      }
      
      else if (mimeType.contains("image/")) {
        $("#response-content-image-box").setAttribute("align", "center");
        $("#response-content-image-box").setAttribute("pack", "center");
        $("#response-content-image-box").hidden = false;
        $("#response-content-image").src =
          "data:" + mimeType + ";" + encoding + "," + aString;

        
        
        $("#response-content-image-name-value").setAttribute("value", uri.fileName);
        $("#response-content-image-mime-value").setAttribute("value", mimeType);
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
          aEditor.setText(NetworkHelper.convertToUnicode(aString, "UTF-8"));

          
          for (let key in CONTENT_MIME_TYPE_MAPPINGS) {
            if (mimeType.contains(key)) {
              aEditor.setMode(CONTENT_MIME_TYPE_MAPPINGS[key]);
              break;
            }
          }
        });
      }
      window.emit("NetMonitor:ResponseBodyAvailable");
    });
  },

  





  _setTimingsInformation: function(aResponse) {
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

  _dataSrc: null,
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




function $(aSelector, aTarget = document) aTarget.querySelector(aSelector);





function drain(aId, aWait, aCallback) {
  window.clearTimeout(drain.store.get(aId));
  drain.store.set(aId, window.setTimeout(aCallback, aWait));
}

drain.store = new Map();




NetMonitorView.Toolbar = new ToolbarView();
NetMonitorView.RequestsMenu = new RequestsMenuView();
NetMonitorView.NetworkDetails = new NetworkDetailsView();
