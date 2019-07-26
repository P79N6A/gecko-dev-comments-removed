




"use strict";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const EPSILON = 0.001;
const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 102400; 
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
  lazyEmpty: true,
  lazyEmptyDelay: 10, 
  searchEnabled: true,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChage: true,
  preventDescriptorModifiers: true,
  eval: () => {},
  switch: () => {}
};




let NetMonitorView = {
  


  initialize: function() {
    this._initializePanes();

    this.Toolbar.initialize();
    this.RequestsMenu.initialize();
    this.NetworkDetails.initialize();
  },

  


  destroy: function() {
    this.Toolbar.destroy();
    this.RequestsMenu.destroy();
    this.NetworkDetails.destroy();

    this._destroyPanes();
  },

  


  _initializePanes: function() {
    dumpn("Initializing the NetMonitorView panes");

    this._body = $("#body");
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
      this._body.removeAttribute("pane-collapsed");
      button.removeAttribute("pane-collapsed");
      button.setAttribute("tooltiptext", this._collapsePaneString);
    } else {
      this._body.setAttribute("pane-collapsed", "");
      button.setAttribute("pane-collapsed", "");
      button.setAttribute("tooltiptext", this._expandPaneString);
    }

    if (aTabIndex !== undefined) {
      $("#event-details-pane").selectedIndex = aTabIndex;
    }
  },

  







  editor: function(aId) {
    dumpn("Getting a NetMonitorView editor: " + aId);

    if (this._editorPromises.has(aId)) {
      return this._editorPromises.get(aId);
    }

    let deferred = promise.defer();
    this._editorPromises.set(aId, deferred.promise);

    
    
    new SourceEditor().init($(aId), DEFAULT_EDITOR_CONFIG, deferred.resolve);

    return deferred.promise;
  },

  _body: null,
  _detailsPane: null,
  _detailsPaneToggleButton: null,
  _collapsePaneString: "",
  _expandPaneString: "",
  _editorPromises: new Map()
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
    let selectedIndex = requestsMenu.selectedIndex;

    
    
    if (selectedIndex == -1 && requestsMenu.itemCount) {
      requestsMenu.selectedIndex = 0;
    } else {
      requestsMenu.selectedIndex = -1;
    }
  },

  _detailsPaneToggleButton: null
};






function RequestsMenuView() {
  dumpn("RequestsMenuView was instantiated");

  this._flushRequests = this._flushRequests.bind(this);
  this._onSelect = this._onSelect.bind(this);
  this._onResize = this._onResize.bind(this);
  this._byFile = this._byFile.bind(this);
  this._byDomain = this._byDomain.bind(this);
  this._byType = this._byType.bind(this);
}

RequestsMenuView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the RequestsMenuView");

    this.widget = new SideMenuWidget($("#requests-menu-contents"), false);
    this._splitter = $('#splitter');
    this._summary = $("#request-menu-network-summary");

    this.allowFocusOnRightClick = true;
    this.widget.maintainSelectionVisible = false;
    this.widget.autoscrollWithAppendedItems = true;

    this.widget.addEventListener("select", this._onSelect, false);
    this._splitter.addEventListener("mousemove", this._onResize, false);
    window.addEventListener("resize", this._onResize, false);
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");

    this.widget.removeEventListener("select", this._onSelect, false);
    this._splitter.removeEventListener("mousemove", this._onResize, false);
    window.removeEventListener("resize", this._onResize, false);
  },

  


  reset: function() {
    this.empty();
    this._firstRequestStartedMillis = -1;
    this._lastRequestEndedMillis = -1;
  },

  


  lazyUpdate: true,

  














  addRequest: function(aId, aStartedDateTime, aMethod, aUrl, aIsXHR) {
    
    let unixTime = Date.parse(aStartedDateTime);

    
    let menuView = this._createMenuView(aMethod, aUrl);

    
    this._registerFirstRequestStart(unixTime);
    this._registerLastRequestEnd(unixTime);

    
    let requestItem = this.push([menuView, aId, ""], {
      attachment: {
        startedDeltaMillis: unixTime - this._firstRequestStartedMillis,
        startedMillis: unixTime,
        method: aMethod,
        url: aUrl,
        isXHR: aIsXHR
      }
    });

    $("#details-pane-toggle").disabled = false;
    $("#requests-menu-empty-notice").hidden = true;

    this.refreshSummary();
    this.refreshZebra();

    if (aId == this._preferredItemId) {
      this.selectedItem = requestItem;
    }
  },

  



  cloneSelectedRequest: function() {
    let selected = this.selectedItem.attachment;

    
    let menuView = this._createMenuView(selected.method, selected.url);

    let newItem = this.push([menuView,, ""], {
      attachment: Object.create(selected, {
        isCustom: { value: true }
      })
    });

    
    this.selectedItem = newItem;
  },

  


  openRequestInTab: function() {
    let win = Services.wm.getMostRecentWindow("navigator:browser");

    let selected = this.selectedItem.attachment;

    win.openUILinkIn(selected.url, "tab", { relatedToCurrent: true });
  },

  


  copyUrl: function() {
    let selected = this.selectedItem.attachment;

    clipboardHelper.copyString(selected.url, document);
  },

  


  sendCustomRequest: function() {
    let selected = this.selectedItem.attachment;

    let data = Object.create(selected, {
      headers: { value: selected.requestHeaders.headers }
    });

    if (selected.requestPostData) {
      data.body = selected.requestPostData.postData.text;
    }

    NetMonitorController.webConsoleClient.sendHTTPRequest(data, aResponse => {
      let id = aResponse.eventActor.actor;
      this._preferredItemId = id;
    });

    this.closeCustomRequest();
  },

  


  closeCustomRequest: function() {
    this.remove(this.selectedItem);

    NetMonitorView.Sidebar.toggle(false);
   },

  






  filterOn: function(aType = "all") {
    let target = $("#requests-menu-filter-" + aType + "-button");
    let buttons = document.querySelectorAll(".requests-menu-footer-button");

    for (let button of buttons) {
      if (button != target) {
        button.removeAttribute("checked");
      } else {
        button.setAttribute("checked", "true");
      }
    }

    
    switch (aType) {
      case "all":
        this.filterContents(() => true);
        break;
      case "html":
        this.filterContents(this._onHtml);
        break;
      case "css":
        this.filterContents(this._onCss);
        break;
      case "js":
        this.filterContents(this._onJs);
        break;
      case "xhr":
        this.filterContents(this._onXhr);
        break;
      case "fonts":
        this.filterContents(this._onFonts);
        break;
      case "images":
        this.filterContents(this._onImages);
        break;
      case "media":
        this.filterContents(this._onMedia);
        break;
      case "flash":
        this.filterContents(this._onFlash);
        break;
    }

    this.refreshSummary();
    this.refreshZebra();
  },

  






  sortBy: function(aType = "waterfall") {
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
      if (target.getAttribute("sorted") == "ascending") {
        target.setAttribute("sorted", direction = "descending");
        target.setAttribute("tooltiptext", L10N.getStr("networkMenu.sortedDesc"));
      } else {
        target.setAttribute("sorted", direction = "ascending");
        target.setAttribute("tooltiptext", L10N.getStr("networkMenu.sortedAsc"));
      }
    }

    
    switch (aType) {
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
      case "waterfall":
        if (direction == "ascending") {
          this.sortContents(this._byTiming);
        } else {
          this.sortContents((a, b) => !this._byTiming(a, b));
        }
        break;
    }

    this.refreshSummary();
    this.refreshZebra();
  },

  







  _onHtml: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("/html"),

  _onCss: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("/css"),

  _onJs: function({ attachment: { mimeType } })
    mimeType && (
      mimeType.contains("/ecmascript") ||
      mimeType.contains("/javascript") ||
      mimeType.contains("/x-javascript")),

  _onXhr: function({ attachment: { isXHR } })
    isXHR,

  _onFonts: function({ attachment: { url, mimeType } }) 
    (mimeType && (
      mimeType.contains("font/") ||
      mimeType.contains("/font"))) ||
    url.contains(".eot") ||
    url.contains(".ttf") ||
    url.contains(".otf") ||
    url.contains(".woff"),

  _onImages: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("image/"),

  _onMedia: function({ attachment: { mimeType } }) 
    mimeType && (
      mimeType.contains("audio/") ||
      mimeType.contains("video/") ||
      mimeType.contains("model/")),

  _onFlash: function({ attachment: { url, mimeType } }) 
    (mimeType && (
      mimeType.contains("/x-flv") ||
      mimeType.contains("/x-shockwave-flash"))) ||
    url.contains(".swf") ||
    url.contains(".flv"),

  











  _byTiming: function({ attachment: first }, { attachment: second })
    first.startedMillis > second.startedMillis,

  _byStatus: function({ attachment: first }, { attachment: second })
    first.status == second.status
      ? first.startedMillis > second.startedMillis
      : first.status > second.status,

  _byMethod: function({ attachment: first }, { attachment: second })
    first.method == second.method
      ? first.startedMillis > second.startedMillis
      : first.method > second.method,

  _byFile: function({ attachment: first }, { attachment: second }) {
    let firstUrl = this._getUriNameWithQuery(first.url).toLowerCase();
    let secondUrl = this._getUriNameWithQuery(second.url).toLowerCase();
    return firstUrl == secondUrl
      ? first.startedMillis > second.startedMillis
      : firstUrl > secondUrl;
  },

  _byDomain: function({ attachment: first }, { attachment: second }) {
    let firstDomain = this._getUriHostPort(first.url).toLowerCase();
    let secondDomain = this._getUriHostPort(second.url).toLowerCase();
    return firstDomain == secondDomain
      ? first.startedMillis > second.startedMillis
      : firstDomain > secondDomain;
  },

  _byType: function({ attachment: first }, { attachment: second }) {
    let firstType = this._getAbbreviatedMimeType(first.mimeType).toLowerCase();
    let secondType = this._getAbbreviatedMimeType(second.mimeType).toLowerCase();
    return firstType == secondType
      ? first.startedMillis > second.startedMillis
      : firstType > secondType;
  },

  _bySize: function({ attachment: first }, { attachment: second })
    first.contentSize > second.contentSize,

  



  refreshSummary: function() {
    let visibleItems = this.visibleItems;
    let visibleRequestsCount = visibleItems.length;
    if (!visibleRequestsCount) {
      this._summary.setAttribute("value", L10N.getStr("networkMenu.empty"));
      return;
    }

    let totalBytes = this._getTotalBytesOfRequests(visibleItems);
    let totalMillis =
      this._getNewestRequest(visibleItems).attachment.endedMillis -
      this._getOldestRequest(visibleItems).attachment.startedMillis;

    
    let str = PluralForm.get(visibleRequestsCount, L10N.getStr("networkMenu.summary"));
    this._summary.setAttribute("value", str
      .replace("#1", visibleRequestsCount)
      .replace("#2", L10N.numberWithDecimals((totalBytes || 0) / 1024, 2))
      .replace("#3", L10N.numberWithDecimals((totalMillis || 0) / 1000, 2))
    );
  },

  


  refreshZebra: function() {
    let visibleItems = this.visibleItems;

    for (let i = 0, len = visibleItems.length; i < len; i++) {
      let requestItem = visibleItems[i];
      let requestTarget = requestItem.target;

      if (i % 2 == 0) {
        requestTarget.setAttribute("even", "");
        requestTarget.removeAttribute("odd");
      } else {
        requestTarget.setAttribute("odd", "");
        requestTarget.removeAttribute("even");
      }
    }
  },

  








  updateRequest: function(aId, aData) {
    
    if (NetMonitorView._isDestroyed) {
      return;
    }
    this._updateQueue.push([aId, aData]);

    
    if (!this.lazyUpdate) {
      return void this._flushRequests();
    }
    
    setNamedTimeout(
      "update-requests", REQUESTS_REFRESH_RATE, () => this._flushRequests());
  },

  


  _flushRequests: function() {
    
    
    for (let [id, data] of this._updateQueue) {
      let requestItem = this.getItemByValue(id);
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
            this.updateMenuView(requestItem, key, value);
            break;
          case "statusText":
            requestItem.attachment.statusText = value;
            this.updateMenuView(requestItem, key,
              requestItem.attachment.status + " " +
              requestItem.attachment.statusText);
            break;
          case "headersSize":
            requestItem.attachment.headersSize = value;
            break;
          case "contentSize":
            requestItem.attachment.contentSize = value;
            this.updateMenuView(requestItem, key, value);
            break;
          case "mimeType":
            requestItem.attachment.mimeType = value;
            this.updateMenuView(requestItem, key, value);
            break;
          case "responseContent":
            requestItem.attachment.responseContent = value;
            break;
          case "totalTime":
            requestItem.attachment.totalTime = value;
            requestItem.attachment.endedMillis = requestItem.attachment.startedMillis + value;
            this.updateMenuView(requestItem, key, value);
            this._registerLastRequestEnd(requestItem.attachment.endedMillis);
            break;
          case "eventTimings":
            requestItem.attachment.eventTimings = value;
            this._createWaterfallView(requestItem, value.timings);
            break;
        }
      }
      
      
      let selectedItem = this.selectedItem;
      if (selectedItem && selectedItem.value == id) {
        NetMonitorView.NetworkDetails.populate(selectedItem.attachment);
      }
    }

    
    this._updateQueue = [];

    
    
    
    
    
    this.sortContents();
    this.filterContents();
    this.refreshSummary();
    this.refreshZebra();
  },

  









  _createMenuView: function(aMethod, aUrl) {
    let template = $("#requests-menu-item-template");
    let fragment = document.createDocumentFragment();

    this.updateMenuView(template, 'method', aMethod);
    this.updateMenuView(template, 'url', aUrl);

    let waterfall = $(".requests-menu-waterfall", template);
    waterfall.style.backgroundImage = this._cachedWaterfallBackground;

    
    for (let node of template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    return fragment;
  },

  









  updateMenuView: function(aItem, aKey, aValue) {
    let target = aItem.target || aItem;

    switch (aKey) {
      case "method": {
        let node = $(".requests-menu-method", target);
        node.setAttribute("value", aValue);
        break;
      }
      case "url": {
        let uri;
        try {
          uri = nsIURL(aValue);
        } catch(e) {
          break; 
        }
        let nameWithQuery = this._getUriNameWithQuery(uri);
        let hostPort = this._getUriHostPort(uri);

        let node = $(".requests-menu-file", target);
        node.setAttribute("value", nameWithQuery);
        node.setAttribute("tooltiptext", nameWithQuery);

        let domain = $(".requests-menu-domain", target);
        domain.setAttribute("value", hostPort);
        domain.setAttribute("tooltiptext", hostPort);
        break;
      }
      case "status": {
        let node = $(".requests-menu-status", target);
        node.setAttribute("code", aValue);
        break;
      }
      case "statusText": {
        let node = $(".requests-menu-status-and-method", target);
        node.setAttribute("tooltiptext", aValue);
        break;
      }
      case "contentSize": {
        let kb = aValue / 1024;
        let size = L10N.numberWithDecimals(kb, CONTENT_SIZE_DECIMALS);
        let node = $(".requests-menu-size", target);
        let text = L10N.getFormatStr("networkMenu.sizeKB", size);
        node.setAttribute("value", text);
        node.setAttribute("tooltiptext", text);
        break;
      }
      case "mimeType": {
        let type = this._getAbbreviatedMimeType(aValue);
        let node = $(".requests-menu-type", target);
        let text = CONTENT_MIME_TYPE_ABBREVIATIONS[type] || type;
        node.setAttribute("value", text);
        node.setAttribute("tooltiptext", aValue);
        break;
      }
      case "totalTime": {
        let node = $(".requests-menu-timings-total", target);
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

    
    
    for (let { target, attachment } in this) {
      let timingsNode = $(".requests-menu-timings", target);
      let startCapNode = $(".requests-menu-timings-cap.start", target);
      let endCapNode = $(".requests-menu-timings-cap.end", target);
      let totalNode = $(".requests-menu-timings-total", target);
      let direction = window.isRTL ? -1 : 1;

      
      
      let translateX = "translateX(" + (direction * attachment.startedDeltaMillis) + "px)";

      
      
      let scaleX = "scaleX(" + scale + ")";

      
      
      let revScaleX = "scaleX(" + (1 / scale) + ")";

      timingsNode.style.transform = scaleX + " " + translateX;
      startCapNode.style.transform = revScaleX + " translateX(" + (direction * 0.5) + "px)";
      endCapNode.style.transform = revScaleX + " translateX(" + (direction * -0.5) + "px)";
      totalNode.style.transform = revScaleX;
    }
  },

  





  _showWaterfallDivisionLabels: function(aScale) {
    let container = $("#requests-menu-waterfall-button");
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
      let direction = window.isRTL ? -1 : 1;

      for (let x = 0; x < availableWidth; x += scaledStep) {
        let divisionMS = (x / aScale).toFixed(0);
        let translateX = "translateX(" + ((direction * x) | 0) + "px)";

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
          let position = (window.isRTL ? canvasWidth - x : x) | 0;
          data32[position] = (alphaComponent << 24) | (b << 16) | (g << 8) | r;
        }
        alphaComponent += REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_ADD;
      }
    }

    
    pixelArray.set(buf8);
    ctx.putImageData(imageData, 0, 0);
    this._cachedWaterfallBackground = "url(" + canvas.toDataURL() + ")";
  },

  


  _flushWaterfallBackgrounds: function() {
    for (let { target } in this) {
      let waterfallNode = $(".requests-menu-waterfall", target);
      waterfallNode.style.backgroundImage = this._cachedWaterfallBackground;
    }
  },

  


  _hideOverflowingColumns: function() {
    if (window.isRTL) {
      return;
    }
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

  


  _onSelect: function({ detail: item }) {
    if (item) {
      NetMonitorView.Sidebar.populate(item.attachment);
      NetMonitorView.Sidebar.toggle(true);
    } else {
      NetMonitorView.Sidebar.toggle(false);
    }
  },

  


  _onResize: function(e) {
    
    setNamedTimeout(
      "resize-events", RESIZE_REFRESH_RATE, () => this._flushWaterfallViews(true));
  },

  


  _onContextShowing: function() {
    let resendElement = $("#request-menu-context-resend");
    resendElement.hidden = !this.selectedItem || this.selectedItem.attachment.isCustom;

    let copyUrlElement = $("#request-menu-context-copy-url");
    copyUrlElement.hidden = !this.selectedItem;
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

  





  _getUriNameWithQuery: function(aUrl) {
    if (!(aUrl instanceof Ci.nsIURL)) {
      aUrl = nsIURL(aUrl);
    }
    let name = NetworkHelper.convertToUnicode(unescape(aUrl.fileName)) || "/";
    let query = NetworkHelper.convertToUnicode(unescape(aUrl.query));
    return name + (query ? "?" + query : "");
  },
  _getUriHostPort: function(aUrl) {
    if (!(aUrl instanceof Ci.nsIURL)) {
      aUrl = nsIURL(aUrl);
    }
    return NetworkHelper.convertToUnicode(unescape(aUrl.hostPort));
  },

  





  _getAbbreviatedMimeType: function(aMimeType) {
    if (!aMimeType) {
      return "";
    }
    return (aMimeType.split(";")[0].split("/")[1] || "").split("+")[0];
  },

  






  _getTotalBytesOfRequests: function(aItemsArray) {
    if (!aItemsArray.length) {
      return 0;
    }
    return aItemsArray.reduce((prev, curr) => prev + curr.attachment.contentSize || 0, 0);
  },

  






  _getOldestRequest: function(aItemsArray) {
    if (!aItemsArray.length) {
      return null;
    }
    return aItemsArray.reduce((prev, curr) =>
      prev.attachment.startedMillis < curr.attachment.startedMillis ? prev : curr);
  },

  






  _getNewestRequest: function(aItemsArray) {
    if (!aItemsArray.length) {
      return null;
    }
    return aItemsArray.reduce((prev, curr) =>
      prev.attachment.startedMillis > curr.attachment.startedMillis ? prev : curr);
  },

  



  get _waterfallWidth() {
    if (this._cachedWaterfallWidth == 0) {
      let container = $("#requests-menu-toolbar");
      let waterfall = $("#requests-menu-waterfall-header-box");
      let containerBounds = container.getBoundingClientRect();
      let waterfallBounds = waterfall.getBoundingClientRect();
      if (!window.isRTL) {
        this._cachedWaterfallWidth = containerBounds.width - waterfallBounds.left;
      } else {
        this._cachedWaterfallWidth = waterfallBounds.right;
      }
    }
    return this._cachedWaterfallWidth;
  },

  _splitter: null,
  _summary: null,
  _canvas: null,
  _ctx: null,
  _cachedWaterfallWidth: 0,
  _cachedWaterfallBackground: "",
  _firstRequestStartedMillis: -1,
  _lastRequestEndedMillis: -1,
  _updateQueue: [],
  _updateTimeout: null,
  _resizeTimeout: null
});




function SidebarView() {
  dumpn("SidebarView was instantiated");
}

SidebarView.prototype = {
  





  toggle: function(aVisibleFlag) {
    NetMonitorView.toggleDetailsPane({ visible: aVisibleFlag });
    NetMonitorView.RequestsMenu._flushWaterfallViews(true);
  },

  





  populate: function(aData) {
    if (aData.isCustom) {
      NetMonitorView.CustomRequest.populate(aData);
      $("#details-pane").selectedIndex = 0;
    } else {
      NetMonitorView.NetworkDetails.populate(aData);
      $("#details-pane").selectedIndex = 1;
    }
  },

  


  reset: function() {
    this.toggle(false);
  }
}




function CustomRequestView() {
  dumpn("CustomRequestView was instantiated");
}

CustomRequestView.prototype = {
  





  populate: function(aData) {
    $("#custom-url-value").value = aData.url;
    $("#custom-method-value").value = aData.method;
    $("#custom-headers-value").value =
       writeHeaderText(aData.requestHeaders.headers);

    if (aData.requestPostData) {
      let body = aData.requestPostData.postData.text;

      gNetwork.getString(body).then(aString => {
        $("#custom-postdata-value").value =  aString;
      });
    }

    this.updateCustomQuery(aData.url);
  },

  





  onUpdate: function(aField) {
    let selectedItem = NetMonitorView.RequestsMenu.selectedItem;
    let field = aField;
    let value;

    switch(aField) {
      case 'method':
        value = $("#custom-method-value").value.trim();
        selectedItem.attachment.method = value;
        break;
      case 'url':
        value = $("#custom-url-value").value;
        this.updateCustomQuery(value);
        selectedItem.attachment.url = value;
        break;
      case 'query':
        let query = $("#custom-query-value").value;
        this.updateCustomUrl(query);
        field = 'url';
        value = $("#custom-url-value").value
        selectedItem.attachment.url = value;
        break;
      case 'body':
        value = $("#custom-postdata-value").value;
        selectedItem.attachment.requestPostData = {
          postData: {
            text: value
          }
        };
        break;
      case 'headers':
        let headersText = $("#custom-headers-value").value;
        value = parseHeaderText(headersText);
        selectedItem.attachment.requestHeaders = {
          headers: value
        };
        break;
    }

    NetMonitorView.RequestsMenu.updateMenuView(selectedItem, field, value);
  },

  





  updateCustomQuery: function(aUrl) {
    let paramsArray = parseQueryString(nsIURL(aUrl).query);
    if (!paramsArray) {
      $("#custom-query").hidden = true;
      return;
    }
    $("#custom-query").hidden = false;
    $("#custom-query-value").value = writeQueryText(paramsArray);
  },

  





  updateCustomUrl: function(aQueryText) {
    let params = parseQueryText(aQueryText);
    let queryString = writeQueryString(params);

    let url = $("#custom-url-value").value;
    let oldQuery = nsIURL(url).query;
    let path = url.replace(oldQuery, queryString);

    $("#custom-url-value").value = path;
  }
}




function NetworkDetailsView() {
  dumpn("NetworkDetailsView was instantiated");

  this._onTabSelect = this._onTabSelect.bind(this);
};

NetworkDetailsView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the NetworkDetailsView");

    this.widget = $("#event-details-pane");

    this._headers = new VariablesView($("#all-headers"),
      Heritage.extend(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: L10N.getStr("headersEmptyText"),
        searchPlaceholder: L10N.getStr("headersFilterText")
      }));
    this._cookies = new VariablesView($("#all-cookies"),
      Heritage.extend(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: L10N.getStr("cookiesEmptyText"),
        searchPlaceholder: L10N.getStr("cookiesFilterText")
      }));
    this._params = new VariablesView($("#request-params"),
      Heritage.extend(GENERIC_VARIABLES_VIEW_SETTINGS, {
        emptyText: L10N.getStr("paramsEmptyText"),
        searchPlaceholder: L10N.getStr("paramsFilterText")
      }));
    this._json = new VariablesView($("#response-content-json"),
      Heritage.extend(GENERIC_VARIABLES_VIEW_SETTINGS, {
        searchPlaceholder: L10N.getStr("jsonFilterText")
      }));

    this._paramsQueryString = L10N.getStr("paramsQueryString");
    this._paramsFormData = L10N.getStr("paramsFormData");
    this._paramsPostPayload = L10N.getStr("paramsPostPayload");
    this._requestHeaders = L10N.getStr("requestHeaders");
    this._responseHeaders = L10N.getStr("responseHeaders");
    this._requestCookies = L10N.getStr("requestCookies");
    this._responseCookies = L10N.getStr("responseCookies");

    $("tabpanels", this.widget).addEventListener("select", this._onTabSelect);
  },

  


  destroy: function() {
    dumpn("Destroying the NetworkDetailsView");
  },

  


  reset: function() {
    this._dataSrc = null;
  },

  





  populate: function(aData) {
    $("#request-params-box").setAttribute("flex", "1");
    $("#request-params-box").hidden = false;
    $("#request-post-data-textarea-box").hidden = true;
    $("#response-content-info-header").hidden = true;
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
    let tab = this.widget.selectedIndex;

    
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
      let headerVar = headersScope.addItem(header.name, {}, true);
      gNetwork.getString(header.value).then(aString => headerVar.setGrip(aString));
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
      let cookieVar = cookiesScope.addItem(cookie.name, {}, true);
      gNetwork.getString(cookie.value).then(aString => cookieVar.setGrip(aString));

      
      
      let cookieProps = Object.keys(cookie);
      if (cookieProps.length == 2) {
        continue;
      }

      
      
      let rawObject = Object.create(null);
      let otherProps = cookieProps.filter(e => e != "name" && e != "value");
      for (let prop of otherProps) {
        rawObject[prop] = cookie[prop];
      }
      cookieVar.populate(rawObject);
      cookieVar.twisty = true;
      cookieVar.expanded = true;
    }
  },

  





  _setRequestGetParams: function(aUrl) {
    let query = nsIURL(aUrl).query;
    if (query) {
      this._addParams(this._paramsQueryString, query);
    }
  },

  







  _setRequestPostParams: function(aHeadersResponse, aPostDataResponse) {
    if (!aHeadersResponse || !aPostDataResponse) {
      return;
    }
    gNetwork.getString(aPostDataResponse.postData.text).then(aString => {
      
      let cType = aHeadersResponse.headers.filter(({ name }) => name == "Content-Type")[0];
      let cString = cType ? cType.value : "";
      if (cString.contains("x-www-form-urlencoded") ||
          aString.contains("x-www-form-urlencoded")) {
        let formDataGroups = aString.split(/\r\n|\n|\r/);
        for (let group of formDataGroups) {
          this._addParams(this._paramsFormData, group);
        }
      }
      
      else {
        
        
        
        $("#request-params-box").removeAttribute("flex");
        let paramsScope = this._params.addScope(this._paramsPostPayload);
        paramsScope.expanded = true;
        paramsScope.locked = true;

        $("#request-post-data-textarea-box").hidden = false;
        NetMonitorView.editor("#request-post-data-textarea").then(aEditor => {
          aEditor.setText(aString);
        });
      }
      window.emit(EVENTS.REQUEST_POST_PARAMS_DISPLAYED);
    });
  },

  







  _addParams: function(aName, aQueryString) {
    let paramsArray = parseQueryString(aQueryString);
    if (!paramsArray) {
      return;
    }
    let paramsScope = this._params.addScope(aName);
    paramsScope.expanded = true;

    for (let param of paramsArray) {
      let headerVar = paramsScope.addItem(param.name, {}, true);
      headerVar.setGrip(param.value);
    }
  },

  







  _setResponseBody: function(aUrl, aResponse) {
    if (!aResponse) {
      return;
    }
    let { mimeType, text, encoding } = aResponse.content;

    gNetwork.getString(text).then(aString => {
      
      
      
      
      if (/\bjson/.test(mimeType)) {
        let jsonpRegex = /^[a-zA-Z0-9_$]+\(|\)$/g; 
        let sanitizedJSON = aString.replace(jsonpRegex, "");
        let callbackPadding = aString.match(jsonpRegex);

        
        
        
        try {
          var jsonObject = JSON.parse(sanitizedJSON);
        } catch (e) {
          var parsingError = e;
        }

        
        if (jsonObject) {
          $("#response-content-json-box").hidden = false;
          let jsonScopeName = callbackPadding
            ? L10N.getFormatStr("jsonpScopeName", callbackPadding[0].slice(0, -1))
            : L10N.getStr("jsonScopeName");

          this._json.controller.setSingleVariable({
            label: jsonScopeName,
            rawObject: jsonObject,
          });
        }
        
        else {
          $("#response-content-textarea-box").hidden = false;
          NetMonitorView.editor("#response-content-textarea").then(aEditor => {
            aEditor.setMode(SourceEditor.MODES.JAVASCRIPT);
            aEditor.setText(aString);
          });
          let infoHeader = $("#response-content-info-header");
          infoHeader.setAttribute("value", parsingError);
          infoHeader.setAttribute("tooltiptext", parsingError);
          infoHeader.hidden = false;
        }
      }
      
      else if (mimeType.contains("image/")) {
        $("#response-content-image-box").setAttribute("align", "center");
        $("#response-content-image-box").setAttribute("pack", "center");
        $("#response-content-image-box").hidden = false;
        $("#response-content-image").src =
          "data:" + mimeType + ";" + encoding + "," + aString;

        
        
        $("#response-content-image-name-value").setAttribute("value", nsIURL(aUrl).fileName);
        $("#response-content-image-mime-value").setAttribute("value", mimeType);
        $("#response-content-image-encoding-value").setAttribute("value", encoding);

        
        $("#response-content-image").onload = e => {
          
          
          
          let { width, height } = e.target.getBoundingClientRect();
          let dimensions = (width - 2) + " x " + (height - 2);
          $("#response-content-image-dimensions-value").setAttribute("value", dimensions);
        };
      }
      
      else {
        $("#response-content-textarea-box").hidden = false;
        NetMonitorView.editor("#response-content-textarea").then(aEditor => {
          aEditor.setMode(SourceEditor.MODES.TEXT);
          aEditor.setText(aString);

          
          
          if (aString.length < SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
            for (let key in CONTENT_MIME_TYPE_MAPPINGS) {
              if (mimeType.contains(key)) {
                aEditor.setMode(CONTENT_MIME_TYPE_MAPPINGS[key]);
                break;
              }
            }
          }
        });
      }
      window.emit(EVENTS.RESPONSE_BODY_DISPLAYED);
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
};




function $(aSelector, aTarget = document) aTarget.querySelector(aSelector);




function nsIURL(aUrl, aStore = nsIURL.store) {
  if (aStore.has(aUrl)) {
    return aStore.get(aUrl);
  }
  let uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
  aStore.set(aUrl, uri);
  return uri;
}
nsIURL.store = new Map();









function parseQueryString(aQueryString) {
  
  if (!aQueryString || !aQueryString.contains("=")) {
    return;
  }
  
  let paramsArray = aQueryString.replace(/^[?&]/, "").split("&").map(e =>
    let (param = e.split("=")) {
      name: NetworkHelper.convertToUnicode(unescape(param[0])),
      value: NetworkHelper.convertToUnicode(unescape(param[1]))
    });
  return paramsArray;
}









function parseHeaderText(aText) {
  return parseRequestText(aText, ":");
}









function parseQueryText(aText) {
  return parseRequestText(aText, "=");
}










function parseRequestText(aText, aDivider) {
  let regex = new RegExp("(.+?)\\" + aDivider + "\\s*(.+)");
  let pairs = [];
  for (let line of aText.split("\n")) {
    let matches;
    if (matches = regex.exec(line)) {
      let [, name, value] = matches;
      pairs.push({name: name, value: value});
    }
  }
  return pairs;
}









function writeHeaderText(aHeaders) {
  return [(name + ": " + value) for ({name, value} of aHeaders)].join("\n");
}









function writeQueryText(aParams) {
  return [(name + "=" + value) for ({name, value} of aParams)].join("\n");
}









function writeQueryString(aParams) {
  return [(name + "=" + value) for ({name, value} of aParams)].join("&");
}




NetMonitorView.Toolbar = new ToolbarView();
NetMonitorView.RequestsMenu = new RequestsMenuView();
NetMonitorView.Sidebar = new SidebarView();
NetMonitorView.CustomRequest = new CustomRequestView();
NetMonitorView.NetworkDetails = new NetworkDetailsView();
