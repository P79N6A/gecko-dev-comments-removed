





const {Cc, Ci, Cu} = require("chrome");

let ToolDefinitions = require("main").Tools;
let {CssLogic} = require("devtools/styleinspector/css-logic");
let {ELEMENT_STYLE} = require("devtools/server/actors/styles");
let promise = require("sdk/core/promise");
let {EventEmitter} = require("devtools/shared/event-emitter");
let {colorUtils} = require("devtools/css-color");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/Templater.jsm");

let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});

const FILTER_CHANGED_TIMEOUT = 300;

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
  this.styleWindow = aStyleInspector.window;
  this.styleDocument = aStyleInspector.window.document;
  this.styleInspector = aStyleInspector;
  this.pageStyle = aPageStyle;
  this.propertyViews = [];

  let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
    getService(Ci.nsIXULChromeRegistry);
  this.getRTLAttr = chromeReg.isLocaleRTL("global") ? "rtl" : "ltr";

  
  this.focusWindow = this.focusWindow.bind(this);
  this._onContextMenu = this._onContextMenu.bind(this);
  this._contextMenuUpdate = this._contextMenuUpdate.bind(this);
  this._onSelectAll = this._onSelectAll.bind(this);
  this._onCopy = this._onCopy.bind(this);

  this.styleDocument.addEventListener("copy", this._onCopy);
  this.styleDocument.addEventListener("mousedown", this.focusWindow);
  this.styleDocument.addEventListener("contextmenu", this._onContextMenu);

  
  this.root = this.styleDocument.getElementById("root");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");

  
  this.noResults = this.styleDocument.getElementById("noResults");

  
  this._handlePrefChange = this._handlePrefChange.bind(this);
  gDevTools.on("pref-changed", this._handlePrefChange);

  CssHtmlTree.processTemplate(this.templateRoot, this.root, this);

  
  this.viewedElement = null;

  this._buildContextMenu();
  this.createStyleViews();
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













CssHtmlTree.processTemplate = function CssHtmlTree_processTemplate(aTemplate,
                                  aDestination, aData, aPreserveDestination)
{
  if (!aPreserveDestination) {
    aDestination.innerHTML = "";
  }

  
  
  let duplicated = aTemplate.cloneNode(true);

  
  
  template(duplicated, aData, { allowEval: true });
  while (duplicated.firstChild) {
    aDestination.appendChild(duplicated.firstChild);
  }
};

XPCOMUtils.defineLazyGetter(CssHtmlTree, "_strings", function() Services.strings
        .createBundle("chrome:

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});

CssHtmlTree.prototype = {
  
  _matchedProperties: null,

  
  _filterChangedTimeout: null,

  
  searchField: null,

  
  includeBrowserStylesCheckbox: null,

  
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
    if (data.pref == "devtools.defaultColorUnit" && this._computed) {
      this.refreshPanel();
    }
  },

  






  highlight: function(aElement) {
    if (!aElement) {
      if (this._refreshProcess) {
        this._refreshProcess.cancel();
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
        
        this.propertyContainer.appendChild(fragment);
        this.noResults.hidden = this.numVisibleProperties > 0;
        deferred.resolve(undefined);
      }
    });

    this._createViewsProcess.schedule();
    return deferred.promise;
  },

  


  refreshPanel: function CssHtmlTree_refreshPanel()
  {
    return promise.all([
      this._createPropertyViews(),
      this.pageStyle.getComputed(this.viewedElement, {
        filter: this._sourceFilter,
        onlyMatched: !this.includeBrowserStyles,
        markMatched: true
      })
    ]).then(([createViews, computed]) => {
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

      let display = this.propertyContainer.style.display;

      let deferred = promise.defer();
      this._refreshProcess = new UpdateProcess(this.styleWindow, this.propertyViews, {
        onItem: (aPropView) => {
          aPropView.refresh();
        },
        onCancel: () => {
          deferred.reject("refresh cancelled");
        },
        onDone: () => {
          this._refreshProcess = null;
          this.noResults.hidden = this.numVisibleProperties > 0;
          this.styleInspector.inspector.emit("computed-view-refreshed");
          deferred.resolve(undefined);
        }
      });
      this._refreshProcess.schedule();
      return deferred.promise;
    }).then(null, (err) => console.error(err));
  },

  




  filterChanged: function CssHtmlTree_filterChanged(aEvent)
  {
    let win = this.styleWindow;

    if (this._filterChangedTimeout) {
      win.clearTimeout(this._filterChangedTimeout);
    }

    this._filterChangedTimeout = win.setTimeout(function() {
      this.refreshPanel();
      this._filterChangeTimeout = null;
    }.bind(this), FILTER_CHANGED_TIMEOUT);
  },

  




  includeBrowserStylesChanged:
  function CssHtmltree_includeBrowserStylesChanged(aEvent)
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
      if (prop.charAt(0) == "-") {
        mozProps.push(prop);
      } else {
        CssHtmlTree.propertyNames.push(prop);
      }
    }

    CssHtmlTree.propertyNames.sort();
    CssHtmlTree.propertyNames.push.apply(CssHtmlTree.propertyNames,
      mozProps.sort());

    this._createPropertyViews();
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
  },

  


  _onContextMenu: function(event) {
    try {
      this.styleDocument.defaultView.focus();

      this._contextmenu.openPopup(
          event.target.ownerDocument.documentElement,
          "overlap", event.clientX, event.clientY, true, false, null);
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

      clipboardHelper.copyString(result, this.styleDocument);

      if (event) {
        event.preventDefault();
      }
    } catch(e) {
      console.error(e);
    }
  },

  


  destroy: function CssHtmlTree_destroy()
  {
    delete this.viewedElement;

    
    this.includeBrowserStylesCheckbox.removeEventListener("command",
      this.includeBrowserStylesChanged);
    this.searchField.removeEventListener("command", this.filterChanged);
    gDevTools.off("pref-changed", this._handlePrefChange);

    
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

      
      this._contextmenu.removeEventListener("popupshowing", this._contextMenuUpdate);
      this._contextmenu.parentNode.removeChild(this._contextmenu);
      this._contextmenu = null;
    }

    
    this.styleDocument.removeEventListener("contextmenu", this._onContextMenu);
    this.styleDocument.removeEventListener("copy", this._onCopy);
    this.styleDocument.removeEventListener("mousedown", this.focusWindow);

    
    delete this.root;
    delete this.propertyContainer;
    delete this.panel;

    
    delete this.styleDocument;

    
    delete this.propertyViews;
    delete this.styleWindow;
    delete this.styleDocument;
    delete this.styleInspector;
  },
};

function PropertyInfo(aTree, aName) {
  this.tree = aTree;
  this.name = aName;
}
PropertyInfo.prototype = {
  get value() {
    if (this.tree._computed) {
      let value = this.tree._computed[this.name].value;
      return colorUtils.processCSSString(value);
    }
  }
};

function createMenuItem(aMenu, aAttributes)
{
  let item = aMenu.ownerDocument.createElementNS(XUL_NS, "menuitem");

  item.setAttribute("label", CssHtmlTree.l10n(aAttributes.label));
  item.setAttribute("accesskey", CssHtmlTree.l10n(aAttributes.accesskey));
  item.addEventListener("command", aAttributes.command);

  aMenu.appendChild(item);

  return item;
}









function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = aTree.getRTLAttr;

  this.link = "https://developer.mozilla.org/CSS/" + aName;

  this.templateMatchedSelectors = aTree.styleDocument.getElementById("templateMatchedSelectors");
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
    if (!this.tree.includeBrowserStyles && !this.hasMatchedSelectors) {
      return false;
    }

    let searchTerm = this.tree.searchField.value.toLowerCase();
    if (searchTerm && this.name.toLowerCase().indexOf(searchTerm) == -1 &&
      this.value.toLowerCase().indexOf(searchTerm) == -1) {
      return false;
    }

    return true;
  },

  



  get propertyHeaderClassName()
  {
    if (this.visible) {
      let isDark = this.tree._darkStripe = !this.tree._darkStripe;
      return isDark ? "property-view theme-bg-darker" : "property-view";
    }
    return "property-view-hidden";
  },

  




  get propertyContentClassName()
  {
    if (this.visible) {
      let isDark = this.tree._darkStripe;
      return isDark ? "property-content theme-bg-darker" : "property-content";
    }
    return "property-content-hidden";
  },

  



  buildMain: function PropertyView_buildMain()
  {
    let doc = this.tree.styleDocument;

    
    this.element = doc.createElementNS(HTML_NS, "div");
    this.element.setAttribute("class", this.propertyHeaderClassName);
    this.element.addEventListener("dblclick",
      this.onMatchedToggle.bind(this), false);

    
    this.element.setAttribute("tabindex", "0");
    this.element.addEventListener("keydown", (aEvent) => {
      let keyEvent = Ci.nsIDOMKeyEvent;
      if (aEvent.keyCode == keyEvent.DOM_VK_F1) {
        this.mdnLinkClick();
      }
      if (aEvent.keyCode == keyEvent.DOM_VK_RETURN ||
        aEvent.keyCode == keyEvent.DOM_VK_SPACE) {
        this.onMatchedToggle(aEvent);
      }
    }, false);

    
    this.matchedExpander = doc.createElementNS(HTML_NS, "div");
    this.matchedExpander.className = "expander theme-twisty";
    this.matchedExpander.addEventListener("click",
      this.onMatchedToggle.bind(this), false);
    this.element.appendChild(this.matchedExpander);

    
    this.nameNode = doc.createElementNS(HTML_NS, "div");
    this.nameNode.setAttribute("class", "property-name theme-fg-color5");
    
    
    this.nameNode.setAttribute("tabindex", "");
    this.nameNode.textContent = this.nameNode.title = this.name;
    
    this.nameNode.addEventListener("click", () => this.element.focus(), false);
    this.element.appendChild(this.nameNode);

    
    this.valueNode = doc.createElementNS(HTML_NS, "div");
    this.valueNode.setAttribute("class", "property-value theme-fg-color1");
    
    
    this.valueNode.setAttribute("tabindex", "");
    this.valueNode.setAttribute("dir", "ltr");
    this.valueNode.textContent = this.valueNode.title = this.value;
    
    this.valueNode.addEventListener("click", () => this.element.focus(), false);
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
    this.valueNode.textContent = this.valueNode.title = this.propertyInfo.value;
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
        CssHtmlTree.processTemplate(this.templateMatchedSelectors,
          this.matchedSelectorsContainer, this);
        this.matchedExpander.setAttribute("open", "");
        this.tree.styleInspector.inspector.emit("computed-view-property-expanded");
      }).then(null, console.error);
    } else {
      this.matchedSelectorsContainer.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
      this.tree.styleInspector.inspector.emit("computed-view-property-collapsed");
      return promise.resolve(undefined);
    }
  },

  get matchedSelectors()
  {
    return this._matchedSelectorResponse;
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

  





  onMatchedToggle: function PropertyView_onMatchedToggle(aEvent)
  {
    this.matchedExpanded = !this.matchedExpanded;
    this.refreshMatchedSelectors();
    aEvent.preventDefault();
  },

  


  mdnLinkClick: function PropertyView_mdnLinkClick(aEvent)
  {
    let inspector = this.tree.styleInspector.inspector;

    if (inspector.target.tab) {
      let browserWin = inspector.target.tab.ownerDocument.defaultView;
      browserWin.openUILinkIn(this.link, "tab");
    }
    aEvent.preventDefault();
  },
};






function SelectorView(aTree, aSelectorInfo)
{
  this.tree = aTree;
  this.selectorInfo = aSelectorInfo;
  this._cacheStatusNames();

  let rule = this.selectorInfo.rule;
  if (rule && rule.parentStyleSheet) {
    this.sheet = rule.parentStyleSheet;
    this.source = CssLogic.shortSource(this.sheet) + ":" + rule.line;
  } else {
    this.source = CssLogic.l10n("rule.sourceElement");
    this.href = "#";
  }
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
    let val = this.selectorInfo.value;
    return colorUtils.processCSSString(val);
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
    let inspector = this.tree.styleInspector.inspector;
    let rule = this.selectorInfo.rule;
    let line = rule.line || 0;

    
    
    
    
    
    

    let href = rule.href;
    let sheet = rule.parentStyleSheet;
    if (sheet && href && !sheet.isSystem) {
      let target = inspector.target;
      if (ToolDefinitions.styleEditor.isTargetSupported(target)) {
        gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
          toolbox.getCurrentPanel().selectStyleSheet(href, line);
        });
      }
      return;
    }

    let contentDoc = null;
    let rawNode = this.tree.viewedElement.rawNode();
    if (rawNode) {
      contentDoc = rawNode.ownerDocument;
    }

    let viewSourceUtils = inspector.viewSourceUtils;
    viewSourceUtils.viewSource(href, null, contentDoc, line);
  }
};

exports.CssHtmlTree = CssHtmlTree;
exports.PropertyView = PropertyView;
