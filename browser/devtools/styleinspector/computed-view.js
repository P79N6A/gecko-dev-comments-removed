





const {Cc, Ci, Cu} = require("chrome");

const ToolDefinitions = require("main").Tools;
const {CssLogic} = require("devtools/styleinspector/css-logic");
const {ELEMENT_STYLE} = require("devtools/server/actors/styles");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {EventEmitter} = require("devtools/toolkit/event-emitter");
const {OutputParser} = require("devtools/output-parser");
const {PrefObserver, PREF_ORIG_SOURCES} = require("devtools/styleeditor/utils");
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const overlays = require("devtools/styleinspector/style-inspector-overlays");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

const FILTER_CHANGED_TIMEOUT = 150;
const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";




















function UpdateProcess(aWin, aGenerator, aOptions)
{
  this.win = aWin;
  this.iter = _Iterator(aGenerator);
  this.onItem = aOptions.onItem || function() {};
  this.onBatch = aOptions.onBatch || function () {};
  this.onDone = aOptions.onDone || function() {};
  this.onCancel = aOptions.onCancel || function() {};
  this.threshold = aOptions.threshold || 45;

  this.canceled = false;
}

UpdateProcess.prototype = {
  


  schedule: function UP_schedule()
  {
    if (this.canceled) {
      return;
    }
    this._timeout = this.win.setTimeout(this._timeoutHandler.bind(this), 0);
  },

  



  cancel: function UP_cancel()
  {
    if (this._timeout) {
      this.win.clearTimeout(this._timeout);
      this._timeout = 0;
    }
    this.canceled = true;
    this.onCancel();
  },

  _timeoutHandler: function UP_timeoutHandler() {
    this._timeout = null;
    try {
      this._runBatch();
      this.schedule();
    } catch(e) {
      if (e instanceof StopIteration) {
        this.onBatch();
        this.onDone();
        return;
      }
      console.error(e);
      throw e;
    }
  },

  _runBatch: function Y_runBatch()
  {
    let time = Date.now();
    while(!this.canceled) {
      
      let next = this.iter.next();
      this.onItem(next[1]);
      if ((Date.now() - time) > this.threshold) {
        this.onBatch();
        return;
      }
    }
  }
};













function CssHtmlTree(aStyleInspector, aPageStyle)
{
  this.styleWindow = aStyleInspector.doc.defaultView;
  this.styleDocument = aStyleInspector.doc;
  this.styleInspector = aStyleInspector;
  this.inspector = this.styleInspector.inspector;
  this.pageStyle = aPageStyle;
  this.propertyViews = [];

  this._outputParser = new OutputParser();

  let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
    getService(Ci.nsIXULChromeRegistry);
  this.getRTLAttr = chromeReg.isLocaleRTL("global") ? "rtl" : "ltr";

  
  this.focusWindow = this.focusWindow.bind(this);
  this._onKeypress = this._onKeypress.bind(this);
  this._onContextMenu = this._onContextMenu.bind(this);
  this._contextMenuUpdate = this._contextMenuUpdate.bind(this);
  this._onSelectAll = this._onSelectAll.bind(this);
  this._onClick = this._onClick.bind(this);
  this._onCopy = this._onCopy.bind(this);
  this._onCopyColor = this._onCopyColor.bind(this);
  this._onCopyUrl = this._onCopyUrl.bind(this);
  this._onCopyImageDataUrl = this._onCopyImageDataUrl.bind(this);
  this._onFilterStyles = this._onFilterStyles.bind(this);
  this._onFilterKeyPress = this._onFilterKeyPress.bind(this);
  this._onClearSearch = this._onClearSearch.bind(this);
  this._onIncludeBrowserStyles = this._onIncludeBrowserStyles.bind(this);
  this._onFilterTextboxContextMenu = this._onFilterTextboxContextMenu.bind(this);

  let doc = this.styleDocument;
  this.root = doc.getElementById("root");
  this.element = doc.getElementById("propertyContainer");
  this.searchField = doc.getElementById("computedview-searchbox");
  this.searchClearButton = doc.getElementById("computedview-searchinput-clear");
  this.includeBrowserStylesCheckbox = doc.getElementById("browser-style-checkbox");

  this.styleDocument.addEventListener("keypress", this._onKeypress);
  this.styleDocument.addEventListener("mousedown", this.focusWindow);
  this.element.addEventListener("click", this._onClick);
  this.element.addEventListener("copy", this._onCopy);
  this.element.addEventListener("contextmenu", this._onContextMenu);
  this.searchField.addEventListener("input", this._onFilterStyles);
  this.searchField.addEventListener("keypress", this._onFilterKeyPress);
  this.searchField.addEventListener("contextmenu", this._onFilterTextboxContextMenu);
  this.searchClearButton.addEventListener("click", this._onClearSearch);
  this.includeBrowserStylesCheckbox.addEventListener("command",
    this._onIncludeBrowserStyles);

  this.searchClearButton.hidden = true;

  
  this.noResults = this.styleDocument.getElementById("noResults");

  
  this._handlePrefChange = this._handlePrefChange.bind(this);
  gDevTools.on("pref-changed", this._handlePrefChange);

  
  this._updateSourceLinks = this._updateSourceLinks.bind(this);
  this._prefObserver = new PrefObserver("devtools.");
  this._prefObserver.on(PREF_ORIG_SOURCES, this._updateSourceLinks);

  
  this.viewedElement = null;

  this._buildContextMenu();
  this.createStyleViews();

  
  this.tooltips = new overlays.TooltipsOverlay(this);
  this.tooltips.addToView();
  this.highlighters = new overlays.HighlightersOverlay(this);
  this.highlighters.addToView();
}






CssHtmlTree.l10n = function CssHtmlTree_l10n(aName)
{
  try {
    return CssHtmlTree._strings.GetStringFromName(aName);
  } catch (ex) {
    Services.console.logStringMessage("Error reading '" + aName + "'");
    throw new Error("l10n error with " + aName);
  }
};

XPCOMUtils.defineLazyGetter(CssHtmlTree, "_strings", function() {
  return Services.strings.createBundle(
    "chrome://global/locale/devtools/styleinspector.properties");
});

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"]
         .getService(Ci.nsIClipboardHelper);
});

CssHtmlTree.prototype = {
  
  _matchedProperties: null,

  
  _filterChangedTimeout: null,

  
  _panelRefreshTimeout: null,

  
  _darkStripe: true,

  
  numVisibleProperties: 0,

  setPageStyle: function(pageStyle) {
    this.pageStyle = pageStyle;
  },

  get includeBrowserStyles()
  {
    return this.includeBrowserStylesCheckbox.checked;
  },

  _handlePrefChange: function(event, data) {
    if (this._computed && (data.pref == "devtools.defaultColorUnit" ||
        data.pref == PREF_ORIG_SOURCES)) {
      this.refreshPanel();
    }
  },

  





  selectElement: function(aElement) {
    if (!aElement) {
      this.viewedElement = null;
      this.noResults.hidden = false;

      if (this._refreshProcess) {
        this._refreshProcess.cancel();
      }
      
      for (let propView of this.propertyViews) {
        propView.refresh();
      }
      return promise.resolve(undefined);
    }

    if (aElement === this.viewedElement) {
      return promise.resolve(undefined);
    }

    this.viewedElement = aElement;
    this.refreshSourceFilter();

    return this.refreshPanel();
  },

  








  getNodeInfo: function(node) {
    if (!node) {
      return null;
    }

    let classes = node.classList;

    
    
    if (classes.contains("matched") ||
        classes.contains("bestmatch") ||
        classes.contains("parentmatch")) {
      let selectorText = "";
      for (let child of node.childNodes) {
        if (child.nodeType === node.TEXT_NODE) {
          selectorText += child.textContent;
        }
      }
      return {
        type: overlays.VIEW_NODE_SELECTOR_TYPE,
        value: selectorText.trim()
      };
    }

    
    let propertyView;
    let propertyContent;
    let parent = node;
    while (parent.parentNode) {
      if (parent.classList.contains("property-view")) {
        propertyView = parent;
        break;
      }
      if (parent.classList.contains("property-content")) {
        propertyContent = parent;
        break;
      }
      parent = parent.parentNode;
    }
    if (!propertyView && !propertyContent) {
      return null;
    }

    let value, type;

    
    let isHref = classes.contains("theme-link") && !classes.contains("link");
    if (propertyView && (classes.contains("property-name") ||
                         classes.contains("property-value") ||
                         isHref)) {
      value = {
        property: parent.querySelector(".property-name").textContent,
        value: parent.querySelector(".property-value").textContent
      };
    }
    if (propertyContent && (classes.contains("other-property-value") ||
                            isHref)) {
      let view = propertyContent.previousSibling;
      value = {
        property: view.querySelector(".property-name").textContent,
        value: node.textContent
      };
    }

    
    if (classes.contains("property-name")) {
      type = overlays.VIEW_NODE_PROPERTY_TYPE;
    } else if (classes.contains("property-value") ||
               classes.contains("other-property-value")) {
      type = overlays.VIEW_NODE_VALUE_TYPE;
    } else if (isHref) {
      type = overlays.VIEW_NODE_IMAGE_URL_TYPE;
      value.url = node.href;
    } else {
      return null;
    }

    return {type, value};
  },

  _createPropertyViews: function()
  {
    if (this._createViewsPromise) {
      return this._createViewsPromise;
    }

    let deferred = promise.defer();
    this._createViewsPromise = deferred.promise;

    this.refreshSourceFilter();
    this.numVisibleProperties = 0;
    let fragment = this.styleDocument.createDocumentFragment();

    this._createViewsProcess = new UpdateProcess(this.styleWindow, CssHtmlTree.propertyNames, {
      onItem: (aPropertyName) => {
        
        let propView = new PropertyView(this, aPropertyName);
        fragment.appendChild(propView.buildMain());
        fragment.appendChild(propView.buildSelectorContainer());

        if (propView.visible) {
          this.numVisibleProperties++;
        }
        this.propertyViews.push(propView);
      },
      onCancel: () => {
        deferred.reject("_createPropertyViews cancelled");
      },
      onDone: () => {
        
        this.element.appendChild(fragment);
        this.noResults.hidden = this.numVisibleProperties > 0;
        deferred.resolve(undefined);
      }
    });

    this._createViewsProcess.schedule();
    return deferred.promise;
  },

  


  refreshPanel: function CssHtmlTree_refreshPanel()
  {
    if (!this.viewedElement) {
      return promise.resolve();
    }

    
    
    let viewedElement = this.viewedElement;

    return promise.all([
      this._createPropertyViews(),
      this.pageStyle.getComputed(this.viewedElement, {
        filter: this._sourceFilter,
        onlyMatched: !this.includeBrowserStyles,
        markMatched: true
      })
    ]).then(([createViews, computed]) => {
      if (viewedElement !== this.viewedElement) {
        return;
      }

      this._matchedProperties = new Set;
      for (let name in computed) {
        if (computed[name].matched) {
          this._matchedProperties.add(name);
        }
      }
      this._computed = computed;

      if (this._refreshProcess) {
        this._refreshProcess.cancel();
      }

      this.noResults.hidden = true;

      
      this.numVisibleProperties = 0;

      
      this._darkStripe = true;

      let deferred = promise.defer();
      this._refreshProcess = new UpdateProcess(this.styleWindow, this.propertyViews, {
        onItem: (aPropView) => {
          aPropView.refresh();
        },
        onDone: () => {
          this._refreshProcess = null;
          this.noResults.hidden = this.numVisibleProperties > 0;

          if (this.searchField.value.length > 0 && !this.numVisibleProperties) {
            this.searchField.classList.add("devtools-style-searchbox-no-match");
          } else {
            this.searchField.classList.remove("devtools-style-searchbox-no-match");
          }

          this.inspector.emit("computed-view-refreshed");
          deferred.resolve(undefined);
        }
      });
      this._refreshProcess.schedule();
      return deferred.promise;
    }).then(null, (err) => console.error(err));
  },

  


  _onKeypress: function(event) {
    let isOSX = Services.appinfo.OS == "Darwin";

    if (((isOSX && event.metaKey && !event.ctrlKey && !event.altKey) ||
        (!isOSX && event.ctrlKey && !event.metaKey && !event.altKey)) &&
        event.code === "KeyF") {
      this.searchField.focus();
      event.preventDefault();
    }
  },

  




  _onFilterStyles: function(aEvent)
  {
    let win = this.styleWindow;

    if (this._filterChangedTimeout) {
      win.clearTimeout(this._filterChangedTimeout);
    }

    let filterTimeout = (this.searchField.value.length > 0)
      ? FILTER_CHANGED_TIMEOUT : 0;
    this.searchClearButton.hidden = this.searchField.value.length === 0;

    this._filterChangedTimeout = win.setTimeout(() => {
      if (this.searchField.value.length > 0) {
        this.searchField.setAttribute("filled", true);
      } else {
        this.searchField.removeAttribute("filled");
      }

      this.refreshPanel();
      this._filterChangeTimeout = null;
    }, filterTimeout);
  },

  



  _onFilterKeyPress: function(aEvent) {
    if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE &&
        this._onClearSearch()) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  },

  


  _onFilterTextboxContextMenu: function(event) {
    try {
      this.styleDocument.defaultView.focus();
      let contextmenu = this.inspector.toolbox.textboxContextMenuPopup;
      contextmenu.openPopupAtScreen(event.screenX, event.screenY, true);
    } catch(e) {
      console.error(e);
    }
  },

  



  _onClearSearch: function() {
    if (this.searchField.value) {
      this.searchField.value = "";
      this.searchField.focus();
      this._onFilterStyles();
      return true;
    }

    return false;
  },

  




  _onIncludeBrowserStyles: function(aEvent)
  {
    this.refreshSourceFilter();
    this.refreshPanel();
  },

  





  refreshSourceFilter: function CssHtmlTree_setSourceFilter()
  {
    this._matchedProperties = null;
    this._sourceFilter = this.includeBrowserStyles ?
                                 CssLogic.FILTER.UA :
                                 CssLogic.FILTER.USER;
  },

  _updateSourceLinks: function CssHtmlTree__updateSourceLinks()
  {
    for (let propView of this.propertyViews) {
      propView.updateSourceLinks();
    }
    this.inspector.emit("computed-view-sourcelinks-updated");
  },

  


  createStyleViews: function CssHtmlTree_createStyleViews()
  {
    if (CssHtmlTree.propertyNames) {
      return;
    }

    CssHtmlTree.propertyNames = [];

    
    
    let styles = this.styleWindow.getComputedStyle(this.styleDocument.documentElement);
    let mozProps = [];
    for (let i = 0, numStyles = styles.length; i < numStyles; i++) {
      let prop = styles.item(i);
      if (prop.startsWith("--")) {
        
        continue;
      } else if (prop.startsWith("-")) {
        mozProps.push(prop);
      } else {
        CssHtmlTree.propertyNames.push(prop);
      }
    }

    CssHtmlTree.propertyNames.sort();
    CssHtmlTree.propertyNames.push.apply(CssHtmlTree.propertyNames,
      mozProps.sort());

    this._createPropertyViews().then(null, e => {
      if (!this.styleInspector) {
        console.warn("The creation of property views was cancelled because the " +
          "computed-view was destroyed before it was done creating views");
      } else {
        console.error(e);
      }
    });
  },

  




  get matchedProperties()
  {
    return this._matchedProperties || new Set;
  },

  




  focusWindow: function(aEvent)
  {
    let win = this.styleDocument.defaultView;
    win.focus();
  },

  


  _buildContextMenu: function()
  {
    let doc = this.styleDocument.defaultView.parent.document;

    this._contextmenu = this.styleDocument.createElementNS(XUL_NS, "menupopup");
    this._contextmenu.addEventListener("popupshowing", this._contextMenuUpdate);
    this._contextmenu.id = "computed-view-context-menu";

    
    this.menuitemSelectAll = createMenuItem(this._contextmenu, {
      label: "computedView.contextmenu.selectAll",
      accesskey: "computedView.contextmenu.selectAll.accessKey",
      command: this._onSelectAll
    });

    
    this.menuitemCopy = createMenuItem(this._contextmenu, {
      label: "computedView.contextmenu.copy",
      accesskey: "computedView.contextmenu.copy.accessKey",
      command: this._onCopy
    });

    
    this.menuitemCopyColor = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.copyColor",
      accesskey: "ruleView.contextmenu.copyColor.accessKey",
      command: this._onCopyColor
    });

    
    this.menuitemCopyUrl = createMenuItem(this._contextmenu, {
      label: "styleinspector.contextmenu.copyUrl",
      accesskey: "styleinspector.contextmenu.copyUrl.accessKey",
      command: this._onCopyUrl
    });

    
    this.menuitemCopyImageDataUrl = createMenuItem(this._contextmenu, {
      label: "styleinspector.contextmenu.copyImageDataUrl",
      accesskey: "styleinspector.contextmenu.copyImageDataUrl.accessKey",
      command: this._onCopyImageDataUrl
    });

    
    this.menuitemSources= createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.showOrigSources",
      accesskey: "ruleView.contextmenu.showOrigSources.accessKey",
      command: this._onToggleOrigSources,
      type: "checkbox"
    });

    let popupset = doc.documentElement.querySelector("popupset");
    if (!popupset) {
      popupset = doc.createElementNS(XUL_NS, "popupset");
      doc.documentElement.appendChild(popupset);
    }
    popupset.appendChild(this._contextmenu);
  },

  



  _contextMenuUpdate: function()
  {
    let win = this.styleDocument.defaultView;
    let disable = win.getSelection().isCollapsed;
    this.menuitemCopy.disabled = disable;

    let showOrig = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    this.menuitemSources.setAttribute("checked", showOrig);

    this.menuitemCopyColor.hidden = !this._isColorPopup();
    this.menuitemCopyUrl.hidden = !this._isImageUrlPopup();
    this.menuitemCopyImageDataUrl.hidden = !this._isImageUrlPopup();
  },

  






  _isColorPopup: function () {
    this._colorToCopy = "";


    let container = this._getPopupNodeContainer();
    if (!container) {
      return false;
    }

    let isColorNode = el => el.dataset && "color" in el.dataset;

    while (!isColorNode(container)) {
      container = container.parentNode;
      if (!container) {
        return false;
      }
    }

    this._colorToCopy = container.dataset["color"];
    return true;
  },

  



  _isImageUrlPopup: function () {
    this._imageUrlToCopy = "";

    let container = this._getPopupNodeContainer();
    let isImageUrlNode = this._isImageUrlNode(container);
    if (isImageUrlNode) {
      this._imageUrlToCopy = container.href;
    }

    return isImageUrlNode;
  },

  




  _isImageUrlNode: function (node) {
    let nodeInfo = this.getNodeInfo(node);
    if (!nodeInfo) {
      return false
    }
    return nodeInfo.type == overlays.VIEW_NODE_IMAGE_URL_TYPE;
  },

  




  _getPopupNodeContainer: function () {
    let container = null;
    let node = this.popupNode;

    if (node) {
      let isTextNode = node.nodeType == node.TEXT_NODE;
      container = isTextNode ? node.parentElement : node;
    }

    return container;
  },

  


  _onContextMenu: function(event) {
    try {
      this.popupNode = event.explicitOriginalTarget;
      this.styleDocument.defaultView.focus();
      this._contextmenu.openPopupAtScreen(event.screenX, event.screenY, true);
    } catch(e) {
      console.error(e);
    }
  },

  


  _onSelectAll: function()
  {
    try {
      let win = this.styleDocument.defaultView;
      let selection = win.getSelection();

      selection.selectAllChildren(this.styleDocument.documentElement);
    } catch(e) {
      console.error(e);
    }
  },

  _onClick: function(event) {
    let target = event.target;

    if (target.nodeName === "a") {
      event.stopPropagation();
      event.preventDefault();
      let browserWin = this.inspector.target.tab.ownerDocument.defaultView;
      browserWin.openUILinkIn(target.href, "tab");
    }
  },

  _onCopyColor: function() {
    clipboardHelper.copyString(this._colorToCopy);
  },

  


  _onCopyUrl: function() {
    clipboardHelper.copyString(this._imageUrlToCopy);
  },

  


  _onCopyImageDataUrl: Task.async(function*() {
    let message;
    try {
      let inspectorFront = this.inspector.inspector;
      let data = yield inspectorFront.getImageDataFromURL(this._imageUrlToCopy);
      message = yield data.data.string();
    } catch (e) {
      message = CssHtmlTree.l10n("styleinspector.copyImageDataUrlError");
    }

    clipboardHelper.copyString(message);
  }),

  




  _onCopy: function(event)
  {
    try {
      let win = this.styleDocument.defaultView;
      let text = win.getSelection().toString().trim();

      
      
      let textArray = text.split(/[\r\n]+/);
      let result = "";

      
      if (textArray.length > 1) {
        for (let prop of textArray) {
          if (CssHtmlTree.propertyNames.indexOf(prop) !== -1) {
            
            result += prop;
          } else {
            
            result += ": " + prop;
            if (result.length > 0) {
              result += ";\n";
            }
          }
        }
      } else {
        
        result = textArray[0];
      }

      clipboardHelper.copyString(result);

      if (event) {
        event.preventDefault();
      }
    } catch(e) {
      console.error(e);
    }
  },

  


  _onToggleOrigSources: function()
  {
    let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    Services.prefs.setBoolPref(PREF_ORIG_SOURCES, !isEnabled);
  },

  


  destroy: function CssHtmlTree_destroy()
  {
    this.viewedElement = null;
    this._outputParser = null;

    gDevTools.off("pref-changed", this._handlePrefChange);

    this._prefObserver.off(PREF_ORIG_SOURCES, this._updateSourceLinks);
    this._prefObserver.destroy();

    
    if (this._createViewsProcess) {
      this._createViewsProcess.cancel();
    }
    if (this._refreshProcess) {
      this._refreshProcess.cancel();
    }

    
    if (this._contextmenu) {
      
      this.menuitemCopy.removeEventListener("command", this._onCopy);
      this.menuitemCopy = null;

      
      this.menuitemSelectAll.removeEventListener("command", this._onSelectAll);
      this.menuitemSelectAll = null;

      
      this.menuitemCopyColor.removeEventListener("command", this._onCopyColor);
      this.menuitemCopyColor = null;

      
      this.menuitemCopyUrl.removeEventListener("command", this._onCopyUrl);
      this.menuitemCopyUrl = null;

      
      this.menuitemCopyImageDataUrl.removeEventListener("command", this._onCopyImageDataUrl);
      this.menuitemCopyImageDataUrl = null;

      
      this._contextmenu.removeEventListener("popupshowing", this._contextMenuUpdate);
      this._contextmenu.parentNode.removeChild(this._contextmenu);
      this._contextmenu = null;
    }

    this.popupNode = null;

    this.tooltips.destroy();
    this.highlighters.destroy();

    
    this.styleDocument.removeEventListener("mousedown", this.focusWindow);
    this.element.removeEventListener("click", this._onClick);
    this.element.removeEventListener("copy", this._onCopy);
    this.element.removeEventListener("contextmenu", this._onContextMenu);
    this.searchField.removeEventListener("input", this._onFilterStyles);
    this.searchField.removeEventListener("keypress", this._onFilterKeyPress);
    this.searchField.removeEventListener("contextmenu", this._onFilterTextboxContextMenu);
    this.searchClearButton.removeEventListener("click", this._onClearSearch);
    this.includeBrowserStylesCheckbox.removeEventListener("command",
      this.includeBrowserStylesChanged);

    
    this.root = null;
    this.element = null;
    this.panel = null;
    this.searchField = null;
    this.searchClearButton = null;
    this.includeBrowserStylesCheckbox = null;

    
    this.styleDocument = null;

    for (let propView of this.propertyViews)  {
      propView.destroy();
    }

    
    this.propertyViews = null;
    this.styleWindow = null;
    this.styleDocument = null;
    this.styleInspector = null;
  }
};

function PropertyInfo(aTree, aName) {
  this.tree = aTree;
  this.name = aName;
}
PropertyInfo.prototype = {
  get value() {
    if (this.tree._computed) {
      let value = this.tree._computed[this.name].value;
      return value;
    }
  }
};

function createMenuItem(aMenu, aAttributes)
{
  let item = aMenu.ownerDocument.createElementNS(XUL_NS, "menuitem");

  item.setAttribute("label", CssHtmlTree.l10n(aAttributes.label));
  if (aAttributes.accesskey) {
    item.setAttribute("accesskey", CssHtmlTree.l10n(aAttributes.accesskey));
  }
  item.addEventListener("command", aAttributes.command);

  if (aAttributes.type) {
    item.setAttribute("type", aAttributes.type);
  }

  aMenu.appendChild(item);

  return item;
}









function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = aTree.getRTLAttr;

  this.link = "https://developer.mozilla.org/CSS/" + aName;

  this._propertyInfo = new PropertyInfo(aTree, aName);
}

PropertyView.prototype = {
  
  element: null,

  
  propertyHeader: null,

  
  nameNode: null,

  
  valueNode: null,

  
  matchedExpanded: false,

  
  matchedSelectorsContainer: null,

  
  matchedExpander: null,

  
  _matchedSelectorViews: null,

  
  prevViewedElement: null,

  





  get value()
  {
    return this.propertyInfo.value;
  },

  


  get propertyInfo()
  {
    return this._propertyInfo;
  },

  


  get hasMatchedSelectors()
  {
    return this.tree.matchedProperties.has(this.name);
  },

  


  get visible()
  {
    if (!this.tree.viewedElement) {
      return false;
    }

    if (!this.tree.includeBrowserStyles && !this.hasMatchedSelectors) {
      return false;
    }

    let searchTerm = this.tree.searchField.value.toLowerCase();
    let isValidSearchTerm = searchTerm.trim().length > 0;
    if (isValidSearchTerm &&
        this.name.toLowerCase().indexOf(searchTerm) == -1 &&
        this.value.toLowerCase().indexOf(searchTerm) == -1) {
      return false;
    }

    return true;
  },

  



  get propertyHeaderClassName()
  {
    if (this.visible) {
      let isDark = this.tree._darkStripe = !this.tree._darkStripe;
      return isDark ? "property-view row-striped" : "property-view";
    }
    return "property-view-hidden";
  },

  




  get propertyContentClassName()
  {
    if (this.visible) {
      let isDark = this.tree._darkStripe;
      return isDark ? "property-content row-striped" : "property-content";
    }
    return "property-content-hidden";
  },

  



  buildMain: function PropertyView_buildMain()
  {
    let doc = this.tree.styleDocument;

    
    this.onMatchedToggle = this.onMatchedToggle.bind(this);
    this.element = doc.createElementNS(HTML_NS, "div");
    this.element.setAttribute("class", this.propertyHeaderClassName);
    this.element.addEventListener("dblclick", this.onMatchedToggle, false);

    
    this.element.setAttribute("tabindex", "0");
    this.onKeyDown = (aEvent) => {
      let keyEvent = Ci.nsIDOMKeyEvent;
      if (aEvent.keyCode == keyEvent.DOM_VK_F1) {
        this.mdnLinkClick();
      }
      if (aEvent.keyCode == keyEvent.DOM_VK_RETURN ||
        aEvent.keyCode == keyEvent.DOM_VK_SPACE) {
        this.onMatchedToggle(aEvent);
      }
    };
    this.element.addEventListener("keydown", this.onKeyDown, false);

    
    this.matchedExpander = doc.createElementNS(HTML_NS, "div");
    this.matchedExpander.className = "expander theme-twisty";
    this.matchedExpander.addEventListener("click", this.onMatchedToggle, false);
    this.element.appendChild(this.matchedExpander);

    this.focusElement = () => this.element.focus();

    
    this.nameNode = doc.createElementNS(HTML_NS, "div");
    this.nameNode.setAttribute("class", "property-name theme-fg-color5");
    
    
    this.nameNode.setAttribute("tabindex", "");
    this.nameNode.textContent = this.nameNode.title = this.name;
    
    this.onFocus = () => this.element.focus();
    this.nameNode.addEventListener("click", this.onFocus, false);
    this.element.appendChild(this.nameNode);

    
    this.valueNode = doc.createElementNS(HTML_NS, "div");
    this.valueNode.setAttribute("class", "property-value theme-fg-color1");
    
    
    this.valueNode.setAttribute("tabindex", "");
    this.valueNode.setAttribute("dir", "ltr");
    
    this.valueNode.addEventListener("click", this.onFocus, false);
    this.element.appendChild(this.valueNode);

    return this.element;
  },

  buildSelectorContainer: function PropertyView_buildSelectorContainer()
  {
    let doc = this.tree.styleDocument;
    let element = doc.createElementNS(HTML_NS, "div");
    element.setAttribute("class", this.propertyContentClassName);
    this.matchedSelectorsContainer = doc.createElementNS(HTML_NS, "div");
    this.matchedSelectorsContainer.setAttribute("class", "matchedselectors");
    element.appendChild(this.matchedSelectorsContainer);

    return element;
  },

  


  refresh: function PropertyView_refresh()
  {
    this.element.className = this.propertyHeaderClassName;
    this.element.nextElementSibling.className = this.propertyContentClassName;

    if (this.prevViewedElement != this.tree.viewedElement) {
      this._matchedSelectorViews = null;
      this.prevViewedElement = this.tree.viewedElement;
    }

    if (!this.tree.viewedElement || !this.visible) {
      this.valueNode.textContent = this.valueNode.title = "";
      this.matchedSelectorsContainer.parentNode.hidden = true;
      this.matchedSelectorsContainer.textContent = "";
      this.matchedExpander.removeAttribute("open");
      return;
    }

    this.tree.numVisibleProperties++;

    let outputParser = this.tree._outputParser;
    let frag = outputParser.parseCssProperty(this.propertyInfo.name,
      this.propertyInfo.value,
      {
        colorSwatchClass: "computedview-colorswatch",
        colorClass: "computedview-color",
        urlClass: "theme-link"
        
      });
    this.valueNode.innerHTML = "";
    this.valueNode.appendChild(frag);

    this.refreshMatchedSelectors();
  },

  


  refreshMatchedSelectors: function PropertyView_refreshMatchedSelectors()
  {
    let hasMatchedSelectors = this.hasMatchedSelectors;
    this.matchedSelectorsContainer.parentNode.hidden = !hasMatchedSelectors;

    if (hasMatchedSelectors) {
      this.matchedExpander.classList.add("expandable");
    } else {
      this.matchedExpander.classList.remove("expandable");
    }

    if (this.matchedExpanded && hasMatchedSelectors) {
      return this.tree.pageStyle.getMatchedSelectors(this.tree.viewedElement, this.name).then(matched => {
        if (!this.matchedExpanded) {
          return;
        }

        this._matchedSelectorResponse = matched;

        return this._buildMatchedSelectors().then(() => {
          this.matchedExpander.setAttribute("open", "");
          this.tree.inspector.emit("computed-view-property-expanded");
        });
      }).then(null, console.error);
    } else {
      this.matchedSelectorsContainer.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
      this.tree.inspector.emit("computed-view-property-collapsed");
      return promise.resolve(undefined);
    }
  },

  get matchedSelectors()
  {
    return this._matchedSelectorResponse;
  },

  _buildMatchedSelectors: function() {
    let promises = [];
    let frag = this.element.ownerDocument.createDocumentFragment();

    for (let selector of this.matchedSelectorViews) {
      let p = createChild(frag, "p");
      let span = createChild(p, "span", {
        class: "rule-link"
      });
      let link = createChild(span, "a", {
        target: "_blank",
        class: "link theme-link",
        title: selector.href,
        sourcelocation: selector.source,
        tabindex: "0",
        textContent: selector.source
      });
      link.addEventListener("click", selector.openStyleEditor, false);
      link.addEventListener("keydown", selector.maybeOpenStyleEditor, false);

      let status = createChild(p, "span", {
        dir: "ltr",
        class: "rule-text theme-fg-color3 " + selector.statusClass,
        title: selector.statusText,
        textContent: selector.sourceText
      });
      let valueSpan = createChild(status, "span", {
        class: "other-property-value theme-fg-color1"
      });
      valueSpan.appendChild(selector.outputFragment);
      promises.push(selector.ready);
    }

    this.matchedSelectorsContainer.innerHTML = "";
    this.matchedSelectorsContainer.appendChild(frag);
    return promise.all(promises);
  },

  



  get matchedSelectorViews()
  {
    if (!this._matchedSelectorViews) {
      this._matchedSelectorViews = [];
      this._matchedSelectorResponse.forEach(
        function matchedSelectorViews_convert(aSelectorInfo) {
          this._matchedSelectorViews.push(new SelectorView(this.tree, aSelectorInfo));
        }, this);
    }

    return this._matchedSelectorViews;
  },

  



  updateSourceLinks: function PropertyView_updateSourceLinks()
  {
    if (!this._matchedSelectorViews) {
      return;
    }
    for (let view of this._matchedSelectorViews) {
      view.updateSourceLink();
    }
  },

  





  onMatchedToggle: function PropertyView_onMatchedToggle(aEvent)
  {
    if (aEvent.shiftKey) {
      return;
    }
    this.matchedExpanded = !this.matchedExpanded;
    this.refreshMatchedSelectors();
    aEvent.preventDefault();
  },

  


  mdnLinkClick: function PropertyView_mdnLinkClick(aEvent)
  {
    let inspector = this.tree.inspector;

    if (inspector.target.tab) {
      let browserWin = inspector.target.tab.ownerDocument.defaultView;
      browserWin.openUILinkIn(this.link, "tab");
    }
    aEvent.preventDefault();
  },

  


  destroy: function PropertyView_destroy() {
    this.element.removeEventListener("dblclick", this.onMatchedToggle, false);
    this.element.removeEventListener("keydown", this.onKeyDown, false);
    this.element = null;

    this.matchedExpander.removeEventListener("click", this.onMatchedToggle, false);
    this.matchedExpander = null;

    this.nameNode.removeEventListener("click", this.onFocus, false);
    this.nameNode = null;

    this.valueNode.removeEventListener("click", this.onFocus, false);
    this.valueNode = null;
  }
};






function SelectorView(aTree, aSelectorInfo)
{
  this.tree = aTree;
  this.selectorInfo = aSelectorInfo;
  this._cacheStatusNames();

  this.openStyleEditor = this.openStyleEditor.bind(this);
  this.maybeOpenStyleEditor = this.maybeOpenStyleEditor.bind(this);

  this.ready = this.updateSourceLink();
}






SelectorView.STATUS_NAMES = [
  
];

SelectorView.CLASS_NAMES = [
  "parentmatch", "matched", "bestmatch"
];

SelectorView.prototype = {
  








  _cacheStatusNames: function SelectorView_cacheStatusNames()
  {
    if (SelectorView.STATUS_NAMES.length) {
      return;
    }

    for (let status in CssLogic.STATUS) {
      let i = CssLogic.STATUS[status];
      if (i > CssLogic.STATUS.UNMATCHED) {
        let value = CssHtmlTree.l10n("rule.status." + status);
        
        SelectorView.STATUS_NAMES[i] = value.replace(/ /g, '\u00A0');
      }
    }
  },

  


  get statusText()
  {
    return SelectorView.STATUS_NAMES[this.selectorInfo.status];
  },

  


  get statusClass()
  {
    return SelectorView.CLASS_NAMES[this.selectorInfo.status - 1];
  },

  get href()
  {
    if (this._href) {
      return this._href;
    }
    let sheet = this.selectorInfo.rule.parentStyleSheet;
    this._href = sheet ? sheet.href : "#";
    return this._href;
  },

  get sourceText()
  {
    return this.selectorInfo.sourceText;
  },


  get value()
  {
    return this.selectorInfo.value;
  },

  get outputFragment()
  {
    
    
    
    
    let outputParser = this.tree._outputParser;
    let frag = outputParser.parseCssProperty(
      this.selectorInfo.name,
      this.selectorInfo.value, {
      colorSwatchClass: "computedview-colorswatch",
      colorClass: "computedview-color",
      urlClass: "theme-link",
      baseURI: this.selectorInfo.rule.href
    });
    return frag;
  },

  



  updateSourceLink: function()
  {
    return this.updateSource().then((oldSource) => {
      if (oldSource != this.source && this.tree.element) {
        let selector = '[sourcelocation="' + oldSource + '"]';
        let link = this.tree.element.querySelector(selector);
        if (link) {
          link.textContent = this.source;
          link.setAttribute("sourcelocation", this.source);
        }
      }
    });
  },

  


  updateSource: function()
  {
    let rule = this.selectorInfo.rule;
    this.sheet = rule.parentStyleSheet;

    if (!rule || !this.sheet) {
      let oldSource = this.source;
      this.source = CssLogic.l10n("rule.sourceElement");
      this.href = "#";
      return promise.resolve(oldSource);
    }

    let showOrig = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);

    if (showOrig && rule.type != ELEMENT_STYLE) {
      let deferred = promise.defer();

      
      this.source = CssLogic.shortSource(this.sheet) + ":" + rule.line;

      rule.getOriginalLocation().then(({href, line, column}) => {
        let oldSource = this.source;
        this.source = CssLogic.shortSource({href: href}) + ":" + line;
        deferred.resolve(oldSource);
      });

      return deferred.promise;
    }

    let oldSource = this.source;
    this.source = CssLogic.shortSource(this.sheet) + ":" + rule.line;
    return promise.resolve(oldSource);
  },

  


  maybeOpenStyleEditor: function(aEvent)
  {
    let keyEvent = Ci.nsIDOMKeyEvent;
    if (aEvent.keyCode == keyEvent.DOM_VK_RETURN) {
      this.openStyleEditor();
    }
  },

  









  openStyleEditor: function(aEvent)
  {
    let inspector = this.tree.inspector;
    let rule = this.selectorInfo.rule;

    
    
    
    
    
    let sheet = rule.parentStyleSheet;
    if (!sheet || sheet.isSystem) {
      let contentDoc = null;
      if (this.tree.viewedElement.isLocal_toBeDeprecated()) {
        let rawNode = this.tree.viewedElement.rawNode();
        if (rawNode) {
          contentDoc = rawNode.ownerDocument;
        }
      }
      let toolbox = gDevTools.getToolbox(inspector.target);
      toolbox.viewSource(rule.href, rule.line);
      return;
    }

    let location = promise.resolve(rule.location);
    if (Services.prefs.getBoolPref(PREF_ORIG_SOURCES)) {
      location = rule.getOriginalLocation();
    }
    location.then(({source, href, line, column}) => {
      let target = inspector.target;
      if (ToolDefinitions.styleEditor.isTargetSupported(target)) {
        gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
          let sheet = source || href;
          toolbox.getCurrentPanel().selectStyleSheet(sheet, line, column);
        });
      }
    });
  }
};











function createChild(aParent, aTag, aAttributes={}) {
  let elt = aParent.ownerDocument.createElementNS(HTML_NS, aTag);
  for (let attr in aAttributes) {
    if (aAttributes.hasOwnProperty(attr)) {
      if (attr === "textContent") {
        elt.textContent = aAttributes[attr];
      } else if(attr === "child") {
        elt.appendChild(aAttributes[attr]);
      } else {
        elt.setAttribute(attr, aAttributes[attr]);
      }
    }
  }
  aParent.appendChild(elt);
  return elt;
}

exports.CssHtmlTree = CssHtmlTree;
exports.PropertyView = PropertyView;
