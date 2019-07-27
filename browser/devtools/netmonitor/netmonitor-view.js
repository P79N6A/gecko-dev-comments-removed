




"use strict";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const EPSILON = 0.001;
const SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 102400; 
const RESIZE_REFRESH_RATE = 50; 
const REQUESTS_REFRESH_RATE = 50; 
const REQUESTS_HEADERS_SAFE_BOUNDS = 30; 
const REQUESTS_TOOLTIP_POSITION = "topcenter bottomleft";
const REQUESTS_TOOLTIP_IMAGE_MAX_DIM = 400; 
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
const REQUEST_TIME_DECIMALS = 2;
const HEADERS_SIZE_DECIMALS = 3;
const CONTENT_SIZE_DECIMALS = 2;
const CONTENT_MIME_TYPE_ABBREVIATIONS = {
  "ecmascript": "js",
  "javascript": "js",
  "x-javascript": "js"
};
const CONTENT_MIME_TYPE_MAPPINGS = {
  "/ecmascript": Editor.modes.js,
  "/javascript": Editor.modes.js,
  "/x-javascript": Editor.modes.js,
  "/html": Editor.modes.html,
  "/xhtml": Editor.modes.html,
  "/xml": Editor.modes.html,
  "/atom": Editor.modes.html,
  "/soap": Editor.modes.html,
  "/rdf": Editor.modes.css,
  "/rss": Editor.modes.css,
  "/css": Editor.modes.css
};
const DEFAULT_EDITOR_CONFIG = {
  mode: Editor.modes.text,
  readOnly: true,
  lineNumbers: true
};
const GENERIC_VARIABLES_VIEW_SETTINGS = {
  lazyEmpty: true,
  lazyEmptyDelay: 10, 
  searchEnabled: true,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChange: true,
  preventDescriptorModifiers: true,
  eval: () => {}
};
const NETWORK_ANALYSIS_PIE_CHART_DIAMETER = 200; 
const FREETEXT_FILTER_SEARCH_DELAY = 200; 




let NetMonitorView = {
  


  initialize: function() {
    this._initializePanes();

    this.Toolbar.initialize();
    this.RequestsMenu.initialize();
    this.NetworkDetails.initialize();
    this.CustomRequest.initialize();
  },

  


  destroy: function() {
    this.Toolbar.destroy();
    this.RequestsMenu.destroy();
    this.NetworkDetails.destroy();
    this.CustomRequest.destroy();

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

    
    if (!Prefs.statistics) {
      $("#request-menu-context-perf").hidden = true;
      $("#notice-perf-message").hidden = true;
      $("#requests-menu-network-summary-button").hidden = true;
      $("#requests-menu-network-summary-label").hidden = true;
    }
  },

  


  _destroyPanes: Task.async(function*() {
    dumpn("Destroying the NetMonitorView panes");

    Prefs.networkDetailsWidth = this._detailsPane.getAttribute("width");
    Prefs.networkDetailsHeight = this._detailsPane.getAttribute("height");

    this._detailsPane = null;
    this._detailsPaneToggleButton = null;

    for (let p of this._editorPromises.values()) {
      let editor = yield p;
      editor.destroy();
    }
  }),

  



  get detailsPaneHidden() {
    return this._detailsPane.hasAttribute("pane-collapsed");
  },

  











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

  



  get currentFrontendMode() {
    return this._body.selectedPanel.id;
  },

  


  toggleFrontendMode: function() {
    if (this.currentFrontendMode != "network-inspector-view") {
      this.showNetworkInspectorView();
    } else {
      this.showNetworkStatisticsView();
    }
  },

  


  showNetworkInspectorView: function() {
    this._body.selectedPanel = $("#network-inspector-view");
    this.RequestsMenu._flushWaterfallViews(true);
  },

  


  showNetworkStatisticsView: function() {
    this._body.selectedPanel = $("#network-statistics-view");

    let controller = NetMonitorController;
    let requestsView = this.RequestsMenu;
    let statisticsView = this.PerformanceStatistics;

    Task.spawn(function*() {
      statisticsView.displayPlaceholderCharts();
      yield controller.triggerActivity(ACTIVITY_TYPE.RELOAD.WITH_CACHE_ENABLED);

      try {
        
        
        
        
        
        yield whenDataAvailable(requestsView.attachments, [
          "responseHeaders", "status", "contentSize", "mimeType", "totalTime"
        ]);
      } catch (ex) {
        
        DevToolsUtils.reportException("showNetworkStatisticsView", ex);
      }

      statisticsView.createPrimedCacheChart(requestsView.items);
      statisticsView.createEmptyCacheChart(requestsView.items);
    });
  },

  reloadPage: function() {
    NetMonitorController.triggerActivity(ACTIVITY_TYPE.RELOAD.WITH_CACHE_DEFAULT);
  },

  







  editor: function(aId) {
    dumpn("Getting a NetMonitorView editor: " + aId);

    if (this._editorPromises.has(aId)) {
      return this._editorPromises.get(aId);
    }

    let deferred = promise.defer();
    this._editorPromises.set(aId, deferred.promise);

    
    
    let editor = new Editor(DEFAULT_EDITOR_CONFIG);
    editor.appendTo($(aId)).then(() => deferred.resolve(editor));

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
  this._onHover = this._onHover.bind(this);
  this._onSelect = this._onSelect.bind(this);
  this._onSwap = this._onSwap.bind(this);
  this._onResize = this._onResize.bind(this);
  this._byFile = this._byFile.bind(this);
  this._byDomain = this._byDomain.bind(this);
  this._byType = this._byType.bind(this);
  this._onSecurityIconClick = this._onSecurityIconClick.bind(this);
}

RequestsMenuView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the RequestsMenuView");

    this.widget = new SideMenuWidget($("#requests-menu-contents"));
    this._splitter = $("#network-inspector-view-splitter");
    this._summary = $("#requests-menu-network-summary-label");
    this._summary.setAttribute("value", L10N.getStr("networkMenu.empty"));
    this.userInputTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    Prefs.filters.forEach(type => this.filterOn(type));
    this.sortContents(this._byTiming);

    this.allowFocusOnRightClick = true;
    this.maintainSelectionVisible = true;
    this.widget.autoscrollWithAppendedItems = true;

    this.widget.addEventListener("select", this._onSelect, false);
    this.widget.addEventListener("swap", this._onSwap, false);
    this._splitter.addEventListener("mousemove", this._onResize, false);
    window.addEventListener("resize", this._onResize, false);

    this.requestsMenuSortEvent = getKeyWithEvent(this.sortBy.bind(this));
    this.requestsMenuFilterEvent = getKeyWithEvent(this.filterOn.bind(this));
    this.reqeustsMenuClearEvent = this.clear.bind(this);
    this._onContextShowing = this._onContextShowing.bind(this);
    this._onContextNewTabCommand = this.openRequestInTab.bind(this);
    this._onContextCopyUrlCommand = this.copyUrl.bind(this);
    this._onContextCopyImageAsDataUriCommand = this.copyImageAsDataUri.bind(this);
    this._onContextResendCommand = this.cloneSelectedRequest.bind(this);
    this._onContextToggleRawHeadersCommand = this.toggleRawHeaders.bind(this);
    this._onContextPerfCommand = () => NetMonitorView.toggleFrontendMode();
    this._onReloadCommand = () => NetMonitorView.reloadPage();

    this.sendCustomRequestEvent = this.sendCustomRequest.bind(this);
    this.closeCustomRequestEvent = this.closeCustomRequest.bind(this);
    this.cloneSelectedRequestEvent = this.cloneSelectedRequest.bind(this);
    this.toggleRawHeadersEvent = this.toggleRawHeaders.bind(this);

    this.requestsFreetextFilterEvent = this.requestsFreetextFilterEvent.bind(this);
    this.reFilterRequests = this.reFilterRequests.bind(this);

    this.freetextFilterBox = $("#requests-menu-filter-freetext-text");
    this.freetextFilterBox.addEventListener("input", this.requestsFreetextFilterEvent, false);
    this.freetextFilterBox.addEventListener("command", this.requestsFreetextFilterEvent, false);

    $("#toolbar-labels").addEventListener("click", this.requestsMenuSortEvent, false);
    $("#requests-menu-footer").addEventListener("click", this.requestsMenuFilterEvent, false);
    $("#requests-menu-clear-button").addEventListener("click", this.reqeustsMenuClearEvent, false);
    $("#network-request-popup").addEventListener("popupshowing", this._onContextShowing, false);
    $("#request-menu-context-newtab").addEventListener("command", this._onContextNewTabCommand, false);
    $("#request-menu-context-copy-url").addEventListener("command", this._onContextCopyUrlCommand, false);
    $("#request-menu-context-copy-image-as-data-uri").addEventListener("command", this._onContextCopyImageAsDataUriCommand, false);
    $("#toggle-raw-headers").addEventListener("click", this.toggleRawHeadersEvent, false);

    window.once("connected", this._onConnect.bind(this));
  },

  _onConnect: function() {
    $("#requests-menu-reload-notice-button").addEventListener("command", this._onReloadCommand, false);

    if (NetMonitorController.supportsCustomRequest) {
      $("#request-menu-context-resend").addEventListener("command", this._onContextResendCommand, false);
      $("#custom-request-send-button").addEventListener("click", this.sendCustomRequestEvent, false);
      $("#custom-request-close-button").addEventListener("click", this.closeCustomRequestEvent, false);
      $("#headers-summary-resend").addEventListener("click", this.cloneSelectedRequestEvent, false);
    } else {
      $("#request-menu-context-resend").hidden = true;
      $("#headers-summary-resend").hidden = true;
    }

    if (NetMonitorController.supportsPerfStats) {
      $("#request-menu-context-perf").addEventListener("command", this._onContextPerfCommand, false);
      $("#requests-menu-perf-notice-button").addEventListener("command", this._onContextPerfCommand, false);
      $("#requests-menu-network-summary-button").addEventListener("command", this._onContextPerfCommand, false);
      $("#requests-menu-network-summary-label").addEventListener("click", this._onContextPerfCommand, false);
      $("#network-statistics-back-button").addEventListener("command", this._onContextPerfCommand, false);
    } else {
      $("#notice-perf-message").hidden = true;
      $("#request-menu-context-perf").hidden = true;
      $("#requests-menu-network-summary-button").hidden = true;
      $("#requests-menu-network-summary-label").hidden = true;
    }

    if (!NetMonitorController.supportsTransferredResponseSize) {
      $("#requests-menu-transferred-header-box").hidden = true;
      $("#requests-menu-item-template .requests-menu-transferred").hidden = true;
    }
  },

  


  destroy: function() {
    dumpn("Destroying the SourcesView");

    Prefs.filters = this._activeFilters;

    this.widget.removeEventListener("select", this._onSelect, false);
    this.widget.removeEventListener("swap", this._onSwap, false);
    this._splitter.removeEventListener("mousemove", this._onResize, false);
    window.removeEventListener("resize", this._onResize, false);

    $("#toolbar-labels").removeEventListener("click", this.requestsMenuSortEvent, false);
    $("#requests-menu-footer").removeEventListener("click", this.requestsMenuFilterEvent, false);
    $("#requests-menu-clear-button").removeEventListener("click", this.reqeustsMenuClearEvent, false);
    this.freetextFilterBox.removeEventListener("input", this.requestsFreetextFilterEvent, false);
    this.freetextFilterBox.removeEventListener("command", this.requestsFreetextFilterEvent, false);
    this.userInputTimer.cancel();
    $("#network-request-popup").removeEventListener("popupshowing", this._onContextShowing, false);
    $("#request-menu-context-newtab").removeEventListener("command", this._onContextNewTabCommand, false);
    $("#request-menu-context-copy-url").removeEventListener("command", this._onContextCopyUrlCommand, false);
    $("#request-menu-context-copy-image-as-data-uri").removeEventListener("command", this._onContextCopyImageAsDataUriCommand, false);
    $("#request-menu-context-resend").removeEventListener("command", this._onContextResendCommand, false);
    $("#request-menu-context-perf").removeEventListener("command", this._onContextPerfCommand, false);

    $("#requests-menu-reload-notice-button").removeEventListener("command", this._onReloadCommand, false);
    $("#requests-menu-perf-notice-button").removeEventListener("command", this._onContextPerfCommand, false);
    $("#requests-menu-network-summary-button").removeEventListener("command", this._onContextPerfCommand, false);
    $("#requests-menu-network-summary-label").removeEventListener("click", this._onContextPerfCommand, false);
    $("#network-statistics-back-button").removeEventListener("command", this._onContextPerfCommand, false);

    $("#custom-request-send-button").removeEventListener("click", this.sendCustomRequestEvent, false);
    $("#custom-request-close-button").removeEventListener("click", this.closeCustomRequestEvent, false);
    $("#headers-summary-resend").removeEventListener("click", this.cloneSelectedRequestEvent, false);
    $("#toggle-raw-headers").removeEventListener("click", this.toggleRawHeadersEvent, false);
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

    
    let requestItem = this.push([menuView, aId], {
      attachment: {
        startedDeltaMillis: unixTime - this._firstRequestStartedMillis,
        startedMillis: unixTime,
        method: aMethod,
        url: aUrl,
        isXHR: aIsXHR
      }
    });

    
    let requestTooltip = requestItem.attachment.tooltip = new Tooltip(document, {
      closeOnEvents: [{
        emitter: $("#requests-menu-contents"),
        event: "scroll",
        useCapture: true
      }]
    });

    $("#details-pane-toggle").disabled = false;
    $("#requests-menu-empty-notice").hidden = true;

    this.refreshSummary();
    this.refreshZebra();
    this.refreshTooltip(requestItem);

    if (aId == this._preferredItemId) {
      this.selectedItem = requestItem;
    }
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

  


  copyAsCurl: function() {
    let selected = this.selectedItem.attachment;

    Task.spawn(function*() {
      
      let data = {
        url: selected.url,
        method: selected.method,
        headers: [],
        httpVersion: selected.httpVersion,
        postDataText: null
      };

      
      for (let { name, value } of selected.requestHeaders.headers) {
        let text = yield gNetwork.getString(value);
        data.headers.push({ name: name, value: text });
      }

      
      if (selected.requestPostData) {
        let postData = selected.requestPostData.postData.text;
        data.postDataText = yield gNetwork.getString(postData);
      }

      clipboardHelper.copyString(Curl.generateCommand(data), document);
    });
  },

  


  copyImageAsDataUri: function() {
    let selected = this.selectedItem.attachment;
    let { mimeType, text, encoding } = selected.responseContent.content;

    gNetwork.getString(text).then(aString => {
      let data = "data:" + mimeType + ";" + encoding + "," + aString;
      clipboardHelper.copyString(data, document);
    });
  },

  



  cloneSelectedRequest: function() {
    let selected = this.selectedItem.attachment;

    
    let menuView = this._createMenuView(selected.method, selected.url);

    
    let newItem = this.push([menuView], {
      attachment: Object.create(selected, {
        isCustom: { value: true }
      })
    });

    
    this.selectedItem = newItem;
  },

  


  sendCustomRequest: function() {
    let selected = this.selectedItem.attachment;

    let data = {
      url: selected.url,
      method: selected.method,
      httpVersion: selected.httpVersion,
    };
    if (selected.requestHeaders) {
      data.headers = selected.requestHeaders.headers;
    }
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

  


  toggleRawHeaders: function() {
    let requestTextarea = $("#raw-request-headers-textarea");
    let responseTextare = $("#raw-response-headers-textarea");
    let rawHeadersHidden = $("#raw-headers").getAttribute("hidden");

    if (rawHeadersHidden) {
      let selected = this.selectedItem.attachment;
      let selectedRequestHeaders = selected.requestHeaders.headers;
      let selectedResponseHeaders = selected.responseHeaders.headers;
      requestTextarea.value = writeHeaderText(selectedRequestHeaders);
      responseTextare.value = writeHeaderText(selectedResponseHeaders);
      $("#raw-headers").hidden = false;
    } else {
      requestTextarea.value = null;
      responseTextare.value = null;
      $("#raw-headers").hidden = true;
    }
  },

  


  requestsFreetextFilterEvent: function() {
    this.userInputTimer.cancel();
    this._currentFreetextFilter = this.freetextFilterBox.value || "";

    if (this._currentFreetextFilter.length === 0) {
      this.freetextFilterBox.removeAttribute("filled");
    } else {
      this.freetextFilterBox.setAttribute("filled", true);
    }

    this.userInputTimer.initWithCallback(this.reFilterRequests, FREETEXT_FILTER_SEARCH_DELAY, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  reFilterRequests: function() {
    this.filterContents(this._filterPredicate);
    this.refreshSummary();
    this.refreshZebra();
  },

  






  filterOn: function(aType = "all") {
    if (aType === "all") {
      
      
      
      
      if (this._activeFilters.indexOf("all") !== -1) {
        return;
      }

      
      
      
      
      this._activeFilters.slice().forEach(this._disableFilter, this);
    }
    else if (this._activeFilters.indexOf(aType) === -1) {
      this._enableFilter(aType);
    }
    else {
      this._disableFilter(aType);
    }

    this.reFilterRequests();
  },

  





  filterOnlyOn: function(aType = "all") {
    this._activeFilters.slice().forEach(this._disableFilter, this);
    this.filterOn(aType);
  },

  







  _disableFilter: function (aType) {
    
    this._activeFilters.splice(this._activeFilters.indexOf(aType), 1);

    
    let target = $("#requests-menu-filter-" + aType + "-button");
    target.removeAttribute("checked");

    
    if (this._activeFilters.length === 0) {
      this._enableFilter("all");
    }
  },

  







  _enableFilter: function (aType) {
    
    if (Object.keys(this._allFilterPredicates).indexOf(aType) == -1) {
      return;
    }

    
    this._activeFilters.push(aType);

    
    let target = $("#requests-menu-filter-" + aType + "-button");
    target.setAttribute("checked", true);

    
    if (aType !== "all" && this._activeFilters.indexOf("all") !== -1) {
      this._disableFilter("all");
    }
  },

  



  get _filterPredicate() {
    let filterPredicates = this._allFilterPredicates;
    let currentFreetextFilter = this._currentFreetextFilter;

    return requestItem => {
      return this._activeFilters.some(filterName => {
        return filterPredicates[filterName].call(this, requestItem) &&
                filterPredicates["freetext"].call(this, requestItem, currentFreetextFilter);
      });
    };
  },

  


  get _allFilterPredicates() ({
    all: () => true,
    html: this.isHtml,
    css: this.isCss,
    js: this.isJs,
    xhr: this.isXHR,
    fonts: this.isFont,
    images: this.isImage,
    media: this.isMedia,
    flash: this.isFlash,
    other: this.isOther,
    freetext: this.isFreetextMatch
  }),

  






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
      case "transferred":
        if (direction == "ascending") {
          this.sortContents(this._byTransferred);
        } else {
          this.sortContents((a, b) => !this._byTransferred(a, b));
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

  


  clear: function() {
    NetMonitorView.Sidebar.toggle(false);
    $("#details-pane-toggle").disabled = true;

    this.empty();
    this.refreshSummary();
  },

  







  isHtml: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("/html"),

  isCss: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("/css"),

  isJs: function({ attachment: { mimeType } })
    mimeType && (
      mimeType.contains("/ecmascript") ||
      mimeType.contains("/javascript") ||
      mimeType.contains("/x-javascript")),

  isXHR: function({ attachment: { isXHR } })
    isXHR,

  isFont: function({ attachment: { url, mimeType } }) 
    (mimeType && (
      mimeType.contains("font/") ||
      mimeType.contains("/font"))) ||
    url.contains(".eot") ||
    url.contains(".ttf") ||
    url.contains(".otf") ||
    url.contains(".woff"),

  isImage: function({ attachment: { mimeType } })
    mimeType && mimeType.contains("image/"),

  isMedia: function({ attachment: { mimeType } }) 
    mimeType && (
      mimeType.contains("audio/") ||
      mimeType.contains("video/") ||
      mimeType.contains("model/")),

  isFlash: function({ attachment: { url, mimeType } }) 
    (mimeType && (
      mimeType.contains("/x-flv") ||
      mimeType.contains("/x-shockwave-flash"))) ||
    url.contains(".swf") ||
    url.contains(".flv"),

  isOther: function(e)
    !this.isHtml(e) && !this.isCss(e) && !this.isJs(e) && !this.isXHR(e) &&
    !this.isFont(e) && !this.isImage(e) && !this.isMedia(e) && !this.isFlash(e),

  isFreetextMatch: function({ attachment: { url } }, text) 
    !text || url.contains(text),

  











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

  _byTransferred: function({ attachment: first }, { attachment: second }) {
    return first.transferredSize > second.transferredSize;
  },

  _bySize: function({ attachment: first }, { attachment: second }) {
    return first.contentSize > second.contentSize;
  },

  



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
      .replace("#2", L10N.numberWithDecimals((totalBytes || 0) / 1024, CONTENT_SIZE_DECIMALS))
      .replace("#3", L10N.numberWithDecimals((totalMillis || 0) / 1000, REQUEST_TIME_DECIMALS))
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

  





  refreshTooltip: function(aItem) {
    let tooltip = aItem.attachment.tooltip;
    tooltip.hide();
    tooltip.startTogglingOnHover(aItem.target, this._onHover);
    tooltip.defaultPosition = REQUESTS_TOOLTIP_POSITION;
  },

  





  attachSecurityIconClickListener: function ({ target }) {
    let icon = $(".requests-security-state-icon", target);
    icon.addEventListener("click", this._onSecurityIconClick);
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
            
            
            
            
            let currentItem = requestItem;
            let currentStore = { headers: [], headersSize: 0 };

            Task.spawn(function*() {
              let postData = yield gNetwork.getString(value.postData.text);
              let payloadHeaders = CurlUtils.getHeadersFromMultipartText(postData);

              currentStore.headers = payloadHeaders;
              currentStore.headersSize = payloadHeaders.reduce(
                (acc, { name, value }) => acc + name.length + value.length + 2, 0);

              
              
              refreshNetworkDetailsPaneIfNecessary(currentItem);
            });

            requestItem.attachment.requestPostData = value;
            requestItem.attachment.requestHeadersFromUploadStream = currentStore;
            break;
          case "securityState":
            requestItem.attachment.securityState = value;
            this.updateMenuView(requestItem, key, value);
            break;
          case "securityInfo":
            requestItem.attachment.securityInfo = value;
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
          case "remoteAddress":
            requestItem.attachment.remoteAddress = value;
            break;
          case "remotePort":
            requestItem.attachment.remotePort = value;
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
          case "transferredSize":
            requestItem.attachment.transferredSize = value;
            this.updateMenuView(requestItem, key, value);
            break;
          case "mimeType":
            requestItem.attachment.mimeType = value;
            this.updateMenuView(requestItem, key, value);
            break;
          case "responseContent":
            
            
            if (!requestItem.attachment.mimeType) {
              requestItem.attachment.mimeType = "text/plain";
              this.updateMenuView(requestItem, "mimeType", "text/plain");
            }
            requestItem.attachment.responseContent = value;
            this.updateMenuView(requestItem, key, value);
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
      refreshNetworkDetailsPaneIfNecessary(requestItem);
    }

    








    function refreshNetworkDetailsPaneIfNecessary(aRequestItem) {
      let selectedItem = NetMonitorView.RequestsMenu.selectedItem;
      if (selectedItem == aRequestItem) {
        NetMonitorView.NetworkDetails.populate(selectedItem.attachment);
      }
    }

    
    this._updateQueue = [];

    
    
    
    
    
    this.sortContents();
    this.filterContents();
    this.refreshSummary();
    this.refreshZebra();

    
    this._flushWaterfallViews();
  },

  









  _createMenuView: function(aMethod, aUrl) {
    let template = $("#requests-menu-item-template");
    let fragment = document.createDocumentFragment();

    this.updateMenuView(template, 'method', aMethod);
    this.updateMenuView(template, 'url', aUrl);

    
    for (let node of template.childNodes) {
      fragment.appendChild(node.cloneNode(true));
    }

    return fragment;
  },

  











  updateMenuView: Task.async(function*(aItem, aKey, aValue) {
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

        let file = $(".requests-menu-file", target);
        file.setAttribute("value", nameWithQuery);
        file.setAttribute("tooltiptext", nameWithQuery);

        let domain = $(".requests-menu-domain", target);
        domain.setAttribute("value", hostPort);
        domain.setAttribute("tooltiptext", hostPort);
        break;
      }
      case "securityState": {
        let tooltip = L10N.getStr("netmonitor.security.state." + aValue);
        let icon = $(".requests-security-state-icon", target);
        icon.classList.add("security-state-" + aValue);
        icon.setAttribute("tooltiptext", tooltip);

        this.attachSecurityIconClickListener(aItem);
        break;
      }
      case "status": {
        let node = $(".requests-menu-status", target);
        let codeNode = $(".requests-menu-status-code", target);
        codeNode.setAttribute("value", aValue);
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
      case "transferredSize": {
        let text;
        if (aValue === null) {
          text = L10N.getStr("networkMenu.sizeUnavailable");
        } else {
          let kb = aValue / 1024;
          let size = L10N.numberWithDecimals(kb, CONTENT_SIZE_DECIMALS);
          text = L10N.getFormatStr("networkMenu.sizeKB", size);
        }
        let node = $(".requests-menu-transferred", target);
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
      case "responseContent": {
        let { mimeType } = aItem.attachment;
        let { text, encoding } = aValue.content;

        if (mimeType.contains("image/")) {
          let responseBody = yield gNetwork.getString(text);
          let node = $(".requests-menu-icon", aItem.target);
          node.src = "data:" + mimeType + ";" + encoding + "," + responseBody;
          node.setAttribute("type", "thumbnail");
          node.removeAttribute("hidden");

          window.emit(EVENTS.RESPONSE_IMAGE_THUMBNAIL_DISPLAYED);
        }
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
  }),

  







  _createWaterfallView: function(aItem, aTimings) {
    let { target, attachment } = aItem;
    let sections = ["dns", "connect", "send", "wait", "receive"];
    

    let timingsNode = $(".requests-menu-timings", target);
    let timingsTotal = $(".requests-menu-timings-total", timingsNode);

    
    for (let key of sections) {
      let width = aTimings[key];

      
      
      if (width > 0) {
        let timingBox = document.createElement("hbox");
        timingBox.className = "requests-menu-timings-box " + key;
        timingBox.setAttribute("width", width);
        timingsNode.insertBefore(timingBox, timingsTotal);
      }
    }
  },

  





  _flushWaterfallViews: function(aReset) {
    
    
    if (NetMonitorView.currentFrontendMode != "network-inspector-view" || !this.itemCount) {
      return;
    }

    
    
    
    
    if (aReset) {
      this._cachedWaterfallWidth = 0;
    }

    
    
    let availableWidth = this._waterfallWidth - REQUESTS_WATERFALL_SAFE_BOUNDS;
    let longestWidth = this._lastRequestEndedMillis - this._firstRequestStartedMillis;
    let scale = Math.min(Math.max(availableWidth / longestWidth, EPSILON), 1);

    
    this._showWaterfallDivisionLabels(scale);
    this._drawWaterfallBackground(scale);

    
    
    for (let { target, attachment } of this) {
      let timingsNode = $(".requests-menu-timings", target);
      let totalNode = $(".requests-menu-timings-total", target);
      let direction = window.isRTL ? -1 : 1;

      
      
      let translateX = "translateX(" + (direction * attachment.startedDeltaMillis) + "px)";

      
      
      let scaleX = "scaleX(" + scale + ")";

      
      
      let revScaleX = "scaleX(" + (1 / scale) + ")";

      timingsNode.style.transform = scaleX + " " + translateX;
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
        let translateX = "translateX(" + ((direction * x) | 0) + "px)";
        let millisecondTime = x / aScale;

        let normalizedTime = millisecondTime;
        let divisionScale = "millisecond";

        
        if (normalizedTime > 60000) {
          normalizedTime /= 60000;
          divisionScale = "minute";
        }
        
        else if (normalizedTime > 1000) {
          normalizedTime /= 1000;
          divisionScale = "second";
        }

        
        if (divisionScale == "millisecond") {
          normalizedTime |= 0;
        } else {
          normalizedTime = L10N.numberWithDecimals(normalizedTime, REQUEST_TIME_DECIMALS);
        }

        let node = document.createElement("label");
        let text = L10N.getFormatStr("networkMenu." + divisionScale, normalizedTime);
        node.className = "plain requests-menu-timings-division";
        node.setAttribute("division-scale", divisionScale);
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
    let view8bit = new Uint8ClampedArray(buf);
    let view32bit = new Uint32Array(buf);

    
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
          view32bit[position] = (alphaComponent << 24) | (b << 16) | (g << 8) | r;
        }
        alphaComponent += REQUESTS_WATERFALL_BACKGROUND_TICKS_OPACITY_ADD;
      }
    }

    
    pixelArray.set(view8bit);
    ctx.putImageData(imageData, 0, 0);
    document.mozSetImageElement("waterfall-background", canvas);
  },

  


  _onSelect: function({ detail: item }) {
    if (item) {
      NetMonitorView.Sidebar.populate(item.attachment);
      NetMonitorView.Sidebar.toggle(true);
    } else {
      NetMonitorView.Sidebar.toggle(false);
    }
  },

  



  _onSwap: function({ detail: [firstItem, secondItem] }) {
    
    
    this.refreshTooltip(firstItem);
    this.refreshTooltip(secondItem);

    
    this.attachSecurityIconClickListener(firstItem);
    this.attachSecurityIconClickListener(secondItem);

  },

  








  _onHover: function(aTarget, aTooltip) {
    let requestItem = this.getItemForElement(aTarget);
    if (!requestItem || !requestItem.attachment.responseContent) {
      return;
    }

    let hovered = requestItem.attachment;
    let { url } = hovered;
    let { mimeType, text, encoding } = hovered.responseContent.content;

    if (mimeType && mimeType.contains("image/") && (
      aTarget.classList.contains("requests-menu-icon") ||
      aTarget.classList.contains("requests-menu-file")))
    {
      return gNetwork.getString(text).then(aString => {
        let anchor = $(".requests-menu-icon", requestItem.target);
        let src = "data:" + mimeType + ";" + encoding + "," + aString;
        aTooltip.setImageContent(src, { maxDim: REQUESTS_TOOLTIP_IMAGE_MAX_DIM });
        return anchor;
      });
    }
  },

  



  _onSecurityIconClick: function(e) {
    let state = this.selectedItem.attachment.securityState;
    if (state !== "insecure") {
      
      NetMonitorView.NetworkDetails.widget.selectedIndex = 5;
    }
  },

  


  _onResize: function(e) {
    
    setNamedTimeout(
      "resize-events", RESIZE_REFRESH_RATE, () => this._flushWaterfallViews(true));
  },

  


  _onContextShowing: function() {
    let selectedItem = this.selectedItem;

    let resendElement = $("#request-menu-context-resend");
    resendElement.hidden = !NetMonitorController.supportsCustomRequest ||
      !selectedItem || selectedItem.attachment.isCustom;

    let copyUrlElement = $("#request-menu-context-copy-url");
    copyUrlElement.hidden = !selectedItem;

    let copyAsCurlElement = $("#request-menu-context-copy-as-curl");
    copyAsCurlElement.hidden = !selectedItem || !selectedItem.attachment.responseContent;

    let copyImageAsDataUriElement = $("#request-menu-context-copy-image-as-data-uri");
    copyImageAsDataUriElement.hidden = !selectedItem ||
      !selectedItem.attachment.responseContent ||
      !selectedItem.attachment.responseContent.content.mimeType.contains("image/");

    let separator = $("#request-menu-context-separator");
    separator.hidden = !selectedItem;

    let newTabElement = $("#request-menu-context-newtab");
    newTabElement.hidden = !selectedItem;
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
  _firstRequestStartedMillis: -1,
  _lastRequestEndedMillis: -1,
  _updateQueue: [],
  _updateTimeout: null,
  _resizeTimeout: null,
  _activeFilters: ["all"],
  _currentFreetextFilter: ""
});




function SidebarView() {
  dumpn("SidebarView was instantiated");
}

SidebarView.prototype = {
  





  toggle: function(aVisibleFlag) {
    NetMonitorView.toggleDetailsPane({ visible: aVisibleFlag });
    NetMonitorView.RequestsMenu._flushWaterfallViews(true);
  },

  







  populate: Task.async(function*(aData) {
    let isCustom = aData.isCustom;
    let view = isCustom ?
      NetMonitorView.CustomRequest :
      NetMonitorView.NetworkDetails;

    yield view.populate(aData);
    $("#details-pane").selectedIndex = isCustom ? 0 : 1;

    window.emit(EVENTS.SIDEBAR_POPULATED);
  })
}




function CustomRequestView() {
  dumpn("CustomRequestView was instantiated");
}

CustomRequestView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the CustomRequestView");

    this.updateCustomRequestEvent = getKeyWithEvent(this.onUpdate.bind(this));
    $("#custom-pane").addEventListener("input", this.updateCustomRequestEvent, false);
  },

  


  destroy: function() {
    dumpn("Destroying the CustomRequestView");

    $("#custom-pane").removeEventListener("input", this.updateCustomRequestEvent, false);
  },

  







  populate: Task.async(function*(aData) {
    $("#custom-url-value").value = aData.url;
    $("#custom-method-value").value = aData.method;
    this.updateCustomQuery(aData.url);

    if (aData.requestHeaders) {
      let headers = aData.requestHeaders.headers;
      $("#custom-headers-value").value = writeHeaderText(headers);
    }
    if (aData.requestPostData) {
      let postData = aData.requestPostData.postData.text;
      $("#custom-postdata-value").value = yield gNetwork.getString(postData);
    }

    window.emit(EVENTS.CUSTOMREQUESTVIEW_POPULATED);
  }),

  





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
        selectedItem.attachment.requestPostData = { postData: { text: value } };
        break;
      case 'headers':
        let headersText = $("#custom-headers-value").value;
        value = parseHeadersText(headersText);
        selectedItem.attachment.requestHeaders = { headers: value };
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

  
  EventEmitter.decorate(this);

  this._onTabSelect = this._onTabSelect.bind(this);
};

NetworkDetailsView.prototype = {
  


  _viewState: {
    
    updating: [],
    
    
    dirty: [],
    
    latestData: null,
  },

  


  initialize: function() {
    dumpn("Initializing the NetworkDetailsView");

    this.widget = $("#event-details-pane");
    this.sidebar = new ToolSidebar(this.widget, this, "netmonitor", {
      disableTelemetry: true,
      showAllTabsMenu: true
    });

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
        onlyEnumVisible: true,
        searchPlaceholder: L10N.getStr("jsonFilterText")
      }));
    VariablesViewController.attach(this._json);

    this._paramsQueryString = L10N.getStr("paramsQueryString");
    this._paramsFormData = L10N.getStr("paramsFormData");
    this._paramsPostPayload = L10N.getStr("paramsPostPayload");
    this._requestHeaders = L10N.getStr("requestHeaders");
    this._requestHeadersFromUpload = L10N.getStr("requestHeadersFromUpload");
    this._responseHeaders = L10N.getStr("responseHeaders");
    this._requestCookies = L10N.getStr("requestCookies");
    this._responseCookies = L10N.getStr("responseCookies");

    $("tabpanels", this.widget).addEventListener("select", this._onTabSelect);
  },

  


  destroy: function() {
    dumpn("Destroying the NetworkDetailsView");
    this.sidebar.destroy();
    $("tabpanels", this.widget).removeEventListener("select", this._onTabSelect);
  },

  







  populate: function(aData) {
    $("#request-params-box").setAttribute("flex", "1");
    $("#request-params-box").hidden = false;
    $("#request-post-data-textarea-box").hidden = true;
    $("#response-content-info-header").hidden = true;
    $("#response-content-json-box").hidden = true;
    $("#response-content-textarea-box").hidden = true;
    $("#raw-headers").hidden = true;
    $("#response-content-image-box").hidden = true;

    let isHtml = RequestsMenuView.prototype.isHtml({ attachment: aData });

    
    this.sidebar.toggleTab(isHtml, "preview-tab", "preview-tabpanel");

    
    
    
    let hasSecurityInfo = aData.securityState &&
                          aData.securityState !== "insecure";
    this.sidebar.toggleTab(hasSecurityInfo, "security-tab", "security-tabpanel");

    
    
    

    if (!isHtml && this.widget.selectedPanel === $("#preview-tabpanel") ||
        !hasSecurityInfo && this.widget.selectedPanel === $("#security-tabpanel")) {
      this.widget.selectedIndex = 0;
    }

    this._headers.empty();
    this._cookies.empty();
    this._params.empty();
    this._json.empty();

    this._dataSrc = { src: aData, populated: [] };
    this._onTabSelect();
    window.emit(EVENTS.NETWORKDETAILSVIEW_POPULATED);

    return promise.resolve();
  },

  


  _onTabSelect: function() {
    let { src, populated } = this._dataSrc || {};
    let tab = this.widget.selectedIndex;
    let view = this;

    
    if (!src || populated[tab]) {
      return;
    }

    let viewState = this._viewState;
    if (viewState.updating[tab]) {
      
      
      
      
      viewState.dirty[tab] = true;
      viewState.latestData = src;
      return;
    }

    Task.spawn(function*() {
      viewState.updating[tab] = true;
      switch (tab) {
        case 0: 
          yield view._setSummary(src);
          yield view._setResponseHeaders(src.responseHeaders);
          yield view._setRequestHeaders(
            src.requestHeaders,
            src.requestHeadersFromUploadStream);
          break;
        case 1: 
          yield view._setResponseCookies(src.responseCookies);
          yield view._setRequestCookies(src.requestCookies);
          break;
        case 2: 
          yield view._setRequestGetParams(src.url);
          yield view._setRequestPostParams(
            src.requestHeaders,
            src.requestHeadersFromUploadStream,
            src.requestPostData);
          break;
        case 3: 
          yield view._setResponseBody(src.url, src.responseContent);
          break;
        case 4: 
          yield view._setTimingsInformation(src.eventTimings);
          break;
        case 5: 
          yield view._setSecurityInfo(src.securityInfo, src.url);
          break;
        case 6: 
          yield view._setHtmlPreview(src.responseContent);
          break;
      }
      viewState.updating[tab] = false;
    }).then(() => {
      if (tab == this.widget.selectedIndex) {
        if (viewState.dirty[tab]) {
          
          viewState.dirty[tab] = false;
          view.populate(viewState.latestData);
        }
        else {
          
          populated[tab] = true;
          window.emit(EVENTS.TAB_UPDATED);

          if (NetMonitorController.isConnected()) {
            NetMonitorView.RequestsMenu.ensureSelectedItemIsVisible();
          }
        }
      }
      else {
        if (viewState.dirty[tab]) {
          
          
          viewState.dirty[tab] = false;
        }
      }
    }, Cu.reportError);
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

    if (aData.remoteAddress) {
      let address = aData.remoteAddress;
      if (address.indexOf(":") != -1) {
        address = `[${address}]`;
      }
      if(aData.remotePort) {
        address += `:${aData.remotePort}`;
      }
      $("#headers-summary-address-value").setAttribute("value", address);
      $("#headers-summary-address-value").setAttribute("tooltiptext", address);
      $("#headers-summary-address").removeAttribute("hidden");
    } else {
      $("#headers-summary-address").setAttribute("hidden", "true");
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

  









  _setRequestHeaders: Task.async(function*(aHeadersResponse, aHeadersFromUploadStream) {
    if (aHeadersResponse && aHeadersResponse.headers.length) {
      yield this._addHeaders(this._requestHeaders, aHeadersResponse);
    }
    if (aHeadersFromUploadStream && aHeadersFromUploadStream.headers.length) {
      yield this._addHeaders(this._requestHeadersFromUpload, aHeadersFromUploadStream);
    }
  }),

  







  _setResponseHeaders: Task.async(function*(aResponse) {
    if (aResponse && aResponse.headers.length) {
      aResponse.headers.sort((a, b) => a.name > b.name);
      yield this._addHeaders(this._responseHeaders, aResponse);
    }
  }),

  









  _addHeaders: Task.async(function*(aName, aResponse) {
    let kb = aResponse.headersSize / 1024;
    let size = L10N.numberWithDecimals(kb, HEADERS_SIZE_DECIMALS);
    let text = L10N.getFormatStr("networkMenu.sizeKB", size);

    let headersScope = this._headers.addScope(aName + " (" + text + ")");
    headersScope.expanded = true;

    for (let header of aResponse.headers) {
      let headerVar = headersScope.addItem(header.name, {}, true);
      let headerValue = yield gNetwork.getString(header.value);
      headerVar.setGrip(headerValue);
    }
  }),

  







  _setRequestCookies: Task.async(function*(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      aResponse.cookies.sort((a, b) => a.name > b.name);
      yield this._addCookies(this._requestCookies, aResponse);
    }
  }),

  







  _setResponseCookies: Task.async(function*(aResponse) {
    if (aResponse && aResponse.cookies.length) {
      yield this._addCookies(this._responseCookies, aResponse);
    }
  }),

  









  _addCookies: Task.async(function*(aName, aResponse) {
    let cookiesScope = this._cookies.addScope(aName);
    cookiesScope.expanded = true;

    for (let cookie of aResponse.cookies) {
      let cookieVar = cookiesScope.addItem(cookie.name, {}, true);
      let cookieValue = yield gNetwork.getString(cookie.value);
      cookieVar.setGrip(cookieValue);

      
      
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
  }),

  





  _setRequestGetParams: function(aUrl) {
    let query = nsIURL(aUrl).query;
    if (query) {
      this._addParams(this._paramsQueryString, query);
    }
  },

  











  _setRequestPostParams: Task.async(function*(aHeadersResponse, aHeadersFromUploadStream, aPostDataResponse) {
    if (!aHeadersResponse || !aHeadersFromUploadStream || !aPostDataResponse) {
      return;
    }

    let { headers: requestHeaders } = aHeadersResponse;
    let { headers: payloadHeaders } = aHeadersFromUploadStream;
    let allHeaders = [...payloadHeaders, ...requestHeaders];

    let contentTypeHeader = allHeaders.find(e => e.name.toLowerCase() == "content-type");
    let contentTypeLongString = contentTypeHeader ? contentTypeHeader.value : "";
    let postDataLongString = aPostDataResponse.postData.text;

    let postData = yield gNetwork.getString(postDataLongString);
    let contentType = yield gNetwork.getString(contentTypeLongString);

    
    if (contentType.contains("x-www-form-urlencoded")) {
      for (let section of postData.split(/\r\n|\r|\n/)) {
        
        
        if (payloadHeaders.every(header => !section.startsWith(header.name))) {
          this._addParams(this._paramsFormData, section);
        }
      }
    }
    
    else {
      
      
      
      $("#request-params-box").removeAttribute("flex");
      let paramsScope = this._params.addScope(this._paramsPostPayload);
      paramsScope.expanded = true;
      paramsScope.locked = true;

      $("#request-post-data-textarea-box").hidden = false;
      let editor = yield NetMonitorView.editor("#request-post-data-textarea");
      
      
      try {
        JSON.parse(postData);
        editor.setMode(Editor.modes.js);
      } catch (e) {
        editor.setMode(Editor.modes.text);
      } finally {
        editor.setText(postData);
      }
    }

    window.emit(EVENTS.REQUEST_POST_PARAMS_DISPLAYED);
  }),

  







  _addParams: function(aName, aQueryString) {
    let paramsArray = parseQueryString(aQueryString);
    if (!paramsArray) {
      return;
    }
    let paramsScope = this._params.addScope(aName);
    paramsScope.expanded = true;

    for (let param of paramsArray) {
      let paramVar = paramsScope.addItem(param.name, {}, true);
      paramVar.setGrip(param.value);
    }
  },

  









  _setResponseBody: Task.async(function*(aUrl, aResponse) {
    if (!aResponse) {
      return;
    }
    let { mimeType, text, encoding } = aResponse.content;
    let responseBody = yield gNetwork.getString(text);

    
    
    
    
    
    
    let jsonMimeType, jsonObject, jsonObjectParseError;
    try {
      jsonMimeType = /\bjson/.test(mimeType);
      jsonObject = JSON.parse(responseBody);
    } catch (e) {
      jsonObjectParseError = e;
    }
    if (jsonMimeType || jsonObject) {
      
      
      
      let jsonpRegex = /^\s*([\w$]+)\s*\(\s*([^]*)\s*\)\s*;?\s*$/;
      let [_, callbackPadding, jsonpString] = responseBody.match(jsonpRegex) || [];

      
      
      
      if (callbackPadding && jsonpString) {
        try {
          jsonObject = JSON.parse(jsonpString);
        } catch (e) {
          jsonObjectParseError = e;
        }
      }

      
      if (jsonObject) {
        $("#response-content-json-box").hidden = false;
        let jsonScopeName = callbackPadding
          ? L10N.getFormatStr("jsonpScopeName", callbackPadding)
          : L10N.getStr("jsonScopeName");

        let jsonVar = { label: jsonScopeName, rawObject: jsonObject };
        yield this._json.controller.setSingleVariable(jsonVar).expanded;
      }
      
      else {
        $("#response-content-textarea-box").hidden = false;
        let infoHeader = $("#response-content-info-header");
        infoHeader.setAttribute("value", jsonObjectParseError);
        infoHeader.setAttribute("tooltiptext", jsonObjectParseError);
        infoHeader.hidden = false;

        let editor = yield NetMonitorView.editor("#response-content-textarea");
        editor.setMode(Editor.modes.js);
        editor.setText(responseBody);
      }
    }
    
    else if (mimeType.contains("image/")) {
      $("#response-content-image-box").setAttribute("align", "center");
      $("#response-content-image-box").setAttribute("pack", "center");
      $("#response-content-image-box").hidden = false;
      $("#response-content-image").src =
        "data:" + mimeType + ";" + encoding + "," + responseBody;

      
      
      $("#response-content-image-name-value").setAttribute("value", nsIURL(aUrl).fileName);
      $("#response-content-image-mime-value").setAttribute("value", mimeType);
      $("#response-content-image-encoding-value").setAttribute("value", encoding);

      
      $("#response-content-image").onload = e => {
        
        
        
        let { width, height } = e.target.getBoundingClientRect();
        let dimensions = (width - 2) + " \u00D7 " + (height - 2);
        $("#response-content-image-dimensions-value").setAttribute("value", dimensions);
      };
    }
    
    else {
      $("#response-content-textarea-box").hidden = false;
      let editor = yield NetMonitorView.editor("#response-content-textarea");
      editor.setMode(Editor.modes.text);
      editor.setText(responseBody);

      
      
      if (responseBody.length < SOURCE_SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
        let mapping = Object.keys(CONTENT_MIME_TYPE_MAPPINGS).find(key => mimeType.contains(key));
        if (mapping) {
          editor.setMode(CONTENT_MIME_TYPE_MAPPINGS[mapping]);
        }
      }
    }

    window.emit(EVENTS.RESPONSE_BODY_DISPLAYED);
  }),

  





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

  







  _setHtmlPreview: Task.async(function*(aResponse) {
    if (!aResponse) {
      return promise.resolve();
    }
    let { text } = aResponse.content;
    let responseBody = yield gNetwork.getString(text);

    
    let iframe = $("#response-preview");
    iframe.contentDocument.docShell.allowJavascript = false;
    iframe.contentDocument.documentElement.innerHTML = responseBody;

    window.emit(EVENTS.RESPONSE_HTML_PREVIEW_DISPLAYED);
  }),

  









  _setSecurityInfo: Task.async(function* (securityInfo, url) {
    if (!securityInfo) {
      
      
      
      
      
      
      return;
    }

    









    function setValue(selector, value) {
      let label = $(selector);
      if (!value) {
        label.setAttribute("value", L10N.getStr("netmonitor.security.notAvailable"));
        label.setAttribute("tooltiptext", label.getAttribute("value"));
      } else {
        label.setAttribute("value", value);
        label.setAttribute("tooltiptext", value);
      }
    }

    let errorbox = $("#security-error");
    let infobox = $("#security-information");

    if (securityInfo.state === "secure" || securityInfo.state === "weak") {
      infobox.hidden = false;
      errorbox.hidden = true;

      
      let cipher = $("#security-warning-cipher");

      if (securityInfo.state === "weak") {
        cipher.hidden = securityInfo.weaknessReasons.indexOf("cipher") === -1;
      } else {
        cipher.hidden = true;
      }

      let enabledLabel = L10N.getStr("netmonitor.security.enabled");
      let disabledLabel = L10N.getStr("netmonitor.security.disabled");

      
      setValue("#security-protocol-version-value", securityInfo.protocolVersion);
      setValue("#security-ciphersuite-value", securityInfo.cipherSuite);

      
      let domain = NetMonitorView.RequestsMenu._getUriHostPort(url);
      let hostHeader = L10N.getFormatStr("netmonitor.security.hostHeader", domain);
      setValue("#security-info-host-header", hostHeader);

      
      setValue("#security-http-strict-transport-security-value",
                securityInfo.hsts ? enabledLabel : disabledLabel);

      setValue("#security-public-key-pinning-value",
                securityInfo.hpkp ? enabledLabel : disabledLabel);

      
      let cert = securityInfo.cert;
      setValue("#security-cert-subject-cn", cert.subject.commonName);
      setValue("#security-cert-subject-o", cert.subject.organization);
      setValue("#security-cert-subject-ou", cert.subject.organizationalUnit);

      setValue("#security-cert-issuer-cn", cert.issuer.commonName);
      setValue("#security-cert-issuer-o", cert.issuer.organization);
      setValue("#security-cert-issuer-ou", cert.issuer.organizationalUnit);

      setValue("#security-cert-validity-begins", cert.validity.start);
      setValue("#security-cert-validity-expires", cert.validity.end);

      setValue("#security-cert-sha1-fingerprint", cert.fingerprint.sha1);
      setValue("#security-cert-sha256-fingerprint", cert.fingerprint.sha256);
    } else {
      infobox.hidden = true;
      errorbox.hidden = false;

      
      let plain = DOMParser.parseFromString(securityInfo.errorMessage, "text/html");
      setValue("#security-error-message", plain.body.textContent);
    }
  }),

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




function PerformanceStatisticsView() {
}

PerformanceStatisticsView.prototype = {
  


  displayPlaceholderCharts: function() {
    this._createChart({
      id: "#primed-cache-chart",
      title: "charts.cacheEnabled"
    });
    this._createChart({
      id: "#empty-cache-chart",
      title: "charts.cacheDisabled"
    });
    window.emit(EVENTS.PLACEHOLDER_CHARTS_DISPLAYED);
  },

  





  createPrimedCacheChart: function(aItems) {
    this._createChart({
      id: "#primed-cache-chart",
      title: "charts.cacheEnabled",
      data: this._sanitizeChartDataSource(aItems),
      strings: this._commonChartStrings,
      totals: this._commonChartTotals,
      sorted: true
    });
    window.emit(EVENTS.PRIMED_CACHE_CHART_DISPLAYED);
  },

  





  createEmptyCacheChart: function(aItems) {
    this._createChart({
      id: "#empty-cache-chart",
      title: "charts.cacheDisabled",
      data: this._sanitizeChartDataSource(aItems, true),
      strings: this._commonChartStrings,
      totals: this._commonChartTotals,
      sorted: true
    });
    window.emit(EVENTS.EMPTY_CACHE_CHART_DISPLAYED);
  },

  



  _commonChartStrings: {
    size: value => {
      let string = L10N.numberWithDecimals(value / 1024, CONTENT_SIZE_DECIMALS);
      return L10N.getFormatStr("charts.sizeKB", string);
    },
    time: value => {
      let string = L10N.numberWithDecimals(value / 1000, REQUEST_TIME_DECIMALS);
      return L10N.getFormatStr("charts.totalS", string);
    }
  },
  _commonChartTotals: {
    size: total => {
      let string = L10N.numberWithDecimals(total / 1024, CONTENT_SIZE_DECIMALS);
      return L10N.getFormatStr("charts.totalSize", string);
    },
    time: total => {
      let seconds = total / 1000;
      let string = L10N.numberWithDecimals(seconds, REQUEST_TIME_DECIMALS);
      return PluralForm.get(seconds, L10N.getStr("charts.totalSeconds")).replace("#1", string);
    },
    cached: total => {
      return L10N.getFormatStr("charts.totalCached", total);
    },
    count: total => {
      return L10N.getFormatStr("charts.totalCount", total);
    }
  },

  







  _createChart: function({ id, title, data, strings, totals, sorted }) {
    let container = $(id);

    
    while (container.hasChildNodes()) {
      container.firstChild.remove();
    }

    
    let chart = Chart.PieTable(document, {
      diameter: NETWORK_ANALYSIS_PIE_CHART_DIAMETER,
      title: L10N.getStr(title),
      data: data,
      strings: strings,
      totals: totals,
      sorted: sorted
    });

    chart.on("click", (_, item) => {
      NetMonitorView.RequestsMenu.filterOnlyOn(item.label);
      NetMonitorView.showNetworkInspectorView();
    });

    container.appendChild(chart.node);
  },

  








  _sanitizeChartDataSource: function(aItems, aEmptyCache) {
    let data = [
      "html", "css", "js", "xhr", "fonts", "images", "media", "flash", "other"
    ].map(e => ({
      cached: 0,
      count: 0,
      label: e,
      size: 0,
      time: 0
    }));

    for (let requestItem of aItems) {
      let details = requestItem.attachment;
      let type;

      if (RequestsMenuView.prototype.isHtml(requestItem)) {
        type = 0; 
      } else if (RequestsMenuView.prototype.isCss(requestItem)) {
        type = 1; 
      } else if (RequestsMenuView.prototype.isJs(requestItem)) {
        type = 2; 
      } else if (RequestsMenuView.prototype.isFont(requestItem)) {
        type = 4; 
      } else if (RequestsMenuView.prototype.isImage(requestItem)) {
        type = 5; 
      } else if (RequestsMenuView.prototype.isMedia(requestItem)) {
        type = 6; 
      } else if (RequestsMenuView.prototype.isFlash(requestItem)) {
        type = 7; 
      } else if (RequestsMenuView.prototype.isXHR(requestItem)) {
        
        type = 3; 
      } else {
        type = 8; 
      }

      if (aEmptyCache || !responseIsFresh(details)) {
        data[type].time += details.totalTime || 0;
        data[type].size += details.contentSize || 0;
      } else {
        data[type].cached++;
      }
      data[type].count++;
    }

    return data.filter(e => e.count > 0);
  },
};




function $(aSelector, aTarget = document) aTarget.querySelector(aSelector);
function $all(aSelector, aTarget = document) aTarget.querySelectorAll(aSelector);




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
  
  
  
  if (!aQueryString) {
    return;
  }
  
  let paramsArray = aQueryString.replace(/^[?&]/, "").split("&").map(e => {
    let param = e.split("=");
    return {
      name: param[0] ? NetworkHelper.convertToUnicode(unescape(param[0])) : "",
      value: param[1] ? NetworkHelper.convertToUnicode(unescape(param[1])) : ""
    }});
  return paramsArray;
}









function parseHeadersText(aText) {
  return parseRequestText(aText, "\\S+?", ":");
}









function parseQueryText(aText) {
  return parseRequestText(aText, ".+?", "=");
}










function parseRequestText(aText, aName, aDivider) {
  let regex = new RegExp("(" + aName + ")\\" + aDivider + "\\s*(.+)");
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










function responseIsFresh({ responseHeaders, status }) {
  
  if (status != 304 || !responseHeaders) {
    return false;
  }

  let list = responseHeaders.headers;
  let cacheControl = list.filter(e => e.name.toLowerCase() == "cache-control")[0];
  let expires = list.filter(e => e.name.toLowerCase() == "expires")[0];

  
  if (cacheControl) {
    let maxAgeMatch =
      cacheControl.value.match(/s-maxage\s*=\s*(\d+)/) ||
      cacheControl.value.match(/max-age\s*=\s*(\d+)/);

    if (maxAgeMatch && maxAgeMatch.pop() > 0) {
      return true;
    }
  }

  
  if (expires && Date.parse(expires.value)) {
    return true;
  }

  return false;
}









function getKeyWithEvent(callback) {
  return function(event) {
    var key = event.target.getAttribute("data-key");
    if (key) {
      callback.call(null, key);
    }
  };
}




NetMonitorView.Toolbar = new ToolbarView();
NetMonitorView.RequestsMenu = new RequestsMenuView();
NetMonitorView.Sidebar = new SidebarView();
NetMonitorView.CustomRequest = new CustomRequestView();
NetMonitorView.NetworkDetails = new NetworkDetailsView();
NetMonitorView.PerformanceStatistics = new PerformanceStatisticsView();
