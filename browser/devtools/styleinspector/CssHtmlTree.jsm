





const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const FILTER_CHANGED_TIMEOUT = 300;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource:///modules/devtools/Templater.jsm");
Cu.import("resource:///modules/devtools/ToolDefinitions.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");

this.EXPORTED_SYMBOLS = ["CssHtmlTree", "PropertyView"];




















function UpdateProcess(aWin, aGenerator, aOptions)
{
  this.win = aWin;
  this.iter = Iterator(aGenerator);
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









this.CssHtmlTree = function CssHtmlTree(aStyleInspector)
{
  this.styleWindow = aStyleInspector.window;
  this.styleDocument = aStyleInspector.window.document;
  this.styleInspector = aStyleInspector;
  this.cssLogic = aStyleInspector.cssLogic;
  this.propertyViews = [];

  let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
    getService(Ci.nsIXULChromeRegistry);
  this.getRTLAttr = chromeReg.isLocaleRTL("global") ? "rtl" : "ltr";

  
  this.siFocusWindow = this.focusWindow.bind(this);
  this.siBoundCopy = this.computedViewCopy.bind(this);

  this.styleDocument.addEventListener("copy", this.siBoundCopy);
  this.styleDocument.addEventListener("mousedown", this.siFocusWindow);

  
  this.root = this.styleDocument.getElementById("root");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");

  
  this.noResults = this.styleDocument.getElementById("noResults");

  
  this.viewedElement = null;
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

  htmlComplete: false,

  
  _filterChangedTimeout: null,

  
  searchField: null,

  
  includeBrowserStylesCheckbox: null,

  
  _panelRefreshTimeout: null,

  
  _darkStripe: true,

  
  numVisibleProperties: 0,

  get includeBrowserStyles()
  {
    return this.includeBrowserStylesCheckbox.checked;
  },

  




  highlight: function CssHtmlTree_highlight(aElement)
  {
    this.viewedElement = aElement;
    this._matchedProperties = null;

    if (!aElement) {
      if (this._refreshProcess) {
        this._refreshProcess.cancel();
      }
      return;
    }

    if (this.htmlComplete) {
      this.refreshSourceFilter();
      this.refreshPanel();
    } else {
      if (this._refreshProcess) {
        this._refreshProcess.cancel();
      }

      CssHtmlTree.processTemplate(this.templateRoot, this.root, this);

      
      
      this.refreshSourceFilter();
      this.numVisibleProperties = 0;
      let fragment = this.styleDocument.createDocumentFragment();
      this._refreshProcess = new UpdateProcess(this.styleWindow, CssHtmlTree.propertyNames, {
        onItem: function(aPropertyName) {
          
          let propView = new PropertyView(this, aPropertyName);
          fragment.appendChild(propView.buildMain());
          fragment.appendChild(propView.buildSelectorContainer());

          if (propView.visible) {
            this.numVisibleProperties++;
          }
          propView.refreshMatchedSelectors();
          this.propertyViews.push(propView);
        }.bind(this),
        onDone: function() {
          
          this.htmlComplete = true;
          this.propertyContainer.appendChild(fragment);
          this.noResults.hidden = this.numVisibleProperties > 0;
          this._refreshProcess = null;

          
          if (this._needsRefresh) {
            delete this._needsRefresh;
            this.refreshPanel();
          } else {
            Services.obs.notifyObservers(null, "StyleInspector-populated", null);
          }
        }.bind(this)});

      this._refreshProcess.schedule();
    }
  },

  


  refreshPanel: function CssHtmlTree_refreshPanel()
  {
    
    
    if (!this.htmlComplete) {
      if (this._refreshProcess) {
        this._needsRefresh = true;
      }
      return;
    }

    if (this._refreshProcess) {
      this._refreshProcess.cancel();
    }

    this.noResults.hidden = true;

    
    this.numVisibleProperties = 0;

    
    this._darkStripe = true;

    let display = this.propertyContainer.style.display;
    this._refreshProcess = new UpdateProcess(this.styleWindow, this.propertyViews, {
      onItem: function(aPropView) {
        aPropView.refresh();
      }.bind(this),
      onDone: function() {
        this._refreshProcess = null;
        this.noResults.hidden = this.numVisibleProperties > 0;
        Services.obs.notifyObservers(null, "StyleInspector-populated", null);
      }.bind(this)
    });
    this._refreshProcess.schedule();
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
    this.cssLogic.sourceFilter = this.includeBrowserStyles ?
                                 CssLogic.FILTER.UA :
                                 CssLogic.FILTER.ALL;
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
  },

  





  get matchedProperties()
  {
    if (!this._matchedProperties) {
      this._matchedProperties =
        this.cssLogic.hasMatchedSelectors(CssHtmlTree.propertyNames);
    }
    return this._matchedProperties;
  },

  




  focusWindow: function si_focusWindow(aEvent)
  {
    let win = this.styleDocument.defaultView;
    win.focus();
  },

  




  computedViewCopy: function si_computedViewCopy(aEvent)
  {
    let win = this.styleDocument.defaultView;
    let text = win.getSelection().toString();

    
    
    text = text.replace(/(.+)\r\n(.+)/g, "$1: $2;");
    text = text.replace(/(.+)\n(.+)/g, "$1: $2;");

    let outerDoc = this.styleInspector.outerIFrame.ownerDocument;
    clipboardHelper.copyString(text, outerDoc);

    if (aEvent) {
      aEvent.preventDefault();
    }
  },

  


  destroy: function CssHtmlTree_destroy()
  {
    delete this.viewedElement;

    
    this.includeBrowserStylesCheckbox.removeEventListener("command",
      this.includeBrowserStylesChanged);
    this.searchField.removeEventListener("command", this.filterChanged);

    
    if (this._refreshProcess) {
      this._refreshProcess.cancel();
    }

    
    let outerDoc = this.styleInspector.outerIFrame.ownerDocument;
    let menu = outerDoc.querySelector("#computed-view-context-menu");
    if (menu) {
      
      let menuitem = outerDoc.querySelector("#computed-view-copy");
      menuitem.removeEventListener("command", this.siBoundCopy);

      
      menuitem = outerDoc.querySelector("#computed-view-copy-declaration");
      menuitem.removeEventListener("command", this.siBoundCopyDeclaration);

      
      menuitem = outerDoc.querySelector("#computed-view-copy-property");
      menuitem.removeEventListener("command", this.siBoundCopyProperty);

      
      menuitem = outerDoc.querySelector("#computed-view-copy-property-value");
      menuitem.removeEventListener("command", this.siBoundCopyPropertyValue);

      menu.removeEventListener("popupshowing", this.siBoundMenuUpdate);
      menu.parentNode.removeChild(menu);
    }

    
    this.styleDocument.removeEventListener("copy", this.siBoundCopy);
    this.styleDocument.removeEventListener("mousedown", this.siFocusWindow);

    
    delete this.root;
    delete this.propertyContainer;
    delete this.panel;

    
    delete this.styleDocument;

    
    delete this.propertyViews;
    delete this.styleWindow;
    delete this.styleDocument;
    delete this.cssLogic;
    delete this.styleInspector;
  },
};









this.PropertyView = function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = aTree.getRTLAttr;

  this.link = "https://developer.mozilla.org/CSS/" + aName;

  this.templateMatchedSelectors = aTree.styleDocument.getElementById("templateMatchedSelectors");
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
    return this.tree.cssLogic.getPropertyInfo(this.name);
  },

  


  get hasMatchedSelectors()
  {
    return this.name in this.tree.matchedProperties;
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
      this.tree._darkStripe = !this.tree._darkStripe;
      let darkValue = this.tree._darkStripe ?
                      "property-view theme-bg-darker" : "property-view";
      return darkValue;
    }
    return "property-view-hidden";
  },

  




  get propertyContentClassName()
  {
    if (this.visible) {
      let darkValue = this.tree._darkStripe ?
                      "property-content theme-bg-darker" : "property-content";
      return darkValue;
    }
    return "property-content-hidden";
  },

  buildMain: function PropertyView_buildMain()
  {
    let doc = this.tree.styleDocument;
    this.element = doc.createElementNS(HTML_NS, "div");
    this.element.setAttribute("class", this.propertyHeaderClassName);

    this.matchedExpander = doc.createElementNS(HTML_NS, "div");
    this.matchedExpander.className = "expander theme-twisty";
    this.matchedExpander.setAttribute("tabindex", "0");
    this.matchedExpander.addEventListener("click",
      this.matchedExpanderClick.bind(this), false);
    this.matchedExpander.addEventListener("keydown", function(aEvent) {
      let keyEvent = Ci.nsIDOMKeyEvent;
      if (aEvent.keyCode == keyEvent.DOM_VK_F1) {
        this.mdnLinkClick();
      }
      if (aEvent.keyCode == keyEvent.DOM_VK_RETURN ||
        aEvent.keyCode == keyEvent.DOM_VK_SPACE) {
        this.matchedExpanderClick(aEvent);
      }
    }.bind(this), false);
    this.element.appendChild(this.matchedExpander);

    this.nameNode = doc.createElementNS(HTML_NS, "div");
    this.element.appendChild(this.nameNode);
    this.nameNode.setAttribute("class", "property-name theme-fg-color5");
    this.nameNode.textContent = this.nameNode.title = this.name;
    this.nameNode.addEventListener("click", function(aEvent) {
      this.matchedExpander.focus();
    }.bind(this), false);

    this.valueNode = doc.createElementNS(HTML_NS, "div");
    this.element.appendChild(this.valueNode);
    this.valueNode.setAttribute("class", "property-value theme-fg-color1");
    this.valueNode.setAttribute("dir", "ltr");
    this.valueNode.textContent = this.valueNode.title = this.value;

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
      CssHtmlTree.processTemplate(this.templateMatchedSelectors,
        this.matchedSelectorsContainer, this);
      this.matchedExpander.setAttribute("open", "");
    } else {
      this.matchedSelectorsContainer.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
    }
  },

  



  get matchedSelectorViews()
  {
    if (!this._matchedSelectorViews) {
      this._matchedSelectorViews = [];
      this.propertyInfo.matchedSelectors.forEach(
        function matchedSelectorViews_convert(aSelectorInfo) {
          this._matchedSelectorViews.push(new SelectorView(this.tree, aSelectorInfo));
        }, this);
    }

    return this._matchedSelectorViews;
  },

  





  matchedExpanderClick: function PropertyView_matchedExpanderClick(aEvent)
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

  


  text: function SelectorView_text(aElement) {
    let result = this.selectorInfo.selector.text;
    if (this.selectorInfo.elementStyle) {
      let source = this.selectorInfo.sourceElement;
      let inspector = this.tree.styleInspector.inspector;

      if (inspector.selection.node == source) {
        result = "this";
      } else {
        result = CssLogic.getShortName(source);
      }
      result += ".style";
    }

    return result;
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
    let contentDoc = inspector.selection.document;
    let cssSheet = this.selectorInfo.selector._cssRule._cssSheet;
    let line = this.selectorInfo.ruleLine || 0;
    let contentSheet = false;
    let styleSheet;
    let styleSheets;

    
    
    
    
    
    
    
    
    if (cssSheet) {
      styleSheet = cssSheet.domSheet;
      styleSheets = contentDoc.styleSheets;

      
      
      for each (let sheet in styleSheets) {
        if (sheet == styleSheet) {
          contentSheet = true;
          break;
        }
      }
    }

    if (contentSheet) {
      let target = inspector.target;

      if (styleEditorDefinition.isTargetSupported(target)) {
        gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
          toolbox.getCurrentPanel().selectStyleSheet(styleSheet, line);
        });
      }
    } else {
      let href = styleSheet ? styleSheet.href : "";
      let viewSourceUtils = inspector.viewSourceUtils;

      if (this.selectorInfo.sourceElement) {
        href = this.selectorInfo.sourceElement.ownerDocument.location.href;
      }
      viewSourceUtils.viewSource(href, null, contentDoc, line);
    }
  },
};
