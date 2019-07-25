









































const Cu = Components.utils;
const FILTER_CHANGED_TIMEOUT = 300;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource:///modules/devtools/Templater.jsm");

var EXPORTED_SYMBOLS = ["CssHtmlTree", "PropertyView"];




















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
    if (this.cancelled) {
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
    while(!this.cancelled) {
      
      let next = this.iter.next();
      this.onItem(next[1]);
      if ((Date.now() - time) > this.threshold) {
        this.onBatch();
        return;
      }
    }
  }
};









function CssHtmlTree(aStyleInspector)
{
  this.styleWin = aStyleInspector.iframe;
  this.styleInspector = aStyleInspector;
  this.cssLogic = aStyleInspector.cssLogic;
  this.doc = aStyleInspector.document;
  this.win = aStyleInspector.window;
  this.getRTLAttr = this.win.getComputedStyle(this.win.gBrowser).direction;
  this.propertyViews = [];

  
  this.styleDocument = this.styleWin.contentWindow.document;

  
  this.root = this.styleDocument.getElementById("root");
  this.path = this.styleDocument.getElementById("path");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.templatePath = this.styleDocument.getElementById("templatePath");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");
  this.panel = aStyleInspector.panel;

  
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
  new Templater().processNode(duplicated, aData);
  while (duplicated.firstChild) {
    aDestination.appendChild(duplicated.firstChild);
  }
};

XPCOMUtils.defineLazyGetter(CssHtmlTree, "_strings", function() Services.strings
        .createBundle("chrome:

XPCOMUtils.defineLazyGetter(CssHtmlTree, "HELP_LINK_TITLE", function() {
  return CssHtmlTree.HELP_LINK_TITLE = CssHtmlTree.l10n("helpLinkTitle");
});

CssHtmlTree.prototype = {
  
  _matchedProperties: null,
  _unmatchedProperties: null,

  htmlComplete: false,

  
  _filterChangedTimeout: null,

  
  searchField: null,

  
  onlyUserStylesCheckbox: null,

  
  _panelRefreshTimeout: null,

  
  _darkStripe: true,

  
  numVisibleProperties: 0,

  get showOnlyUserStyles()
  {
    return this.onlyUserStylesCheckbox.checked;
  },

  




  highlight: function CssHtmlTree_highlight(aElement)
  {
    this.viewedElement = aElement;
    this._unmatchedProperties = null;
    this._matchedProperties = null;

    CssHtmlTree.processTemplate(this.templatePath, this.path, this);

    if (this.htmlComplete) {
      this.refreshPanel();
    } else {
      if (this._refreshProcess) {
        this._refreshProcess.cancel();
      }

      CssHtmlTree.processTemplate(this.templateRoot, this.root, this);

      this.numVisibleProperties = 0;
      let fragment = this.doc.createDocumentFragment();
      this._refreshProcess = new UpdateProcess(this.win, CssHtmlTree.propertyNames, {
        onItem: function(aPropertyName) {
          
          if (this.viewedElement != aElement || !this.styleInspector.isOpen()) {
            return false;
          }
          let propView = new PropertyView(this, aPropertyName);
          fragment.appendChild(propView.build());
          if (propView.visible) {
            this.numVisibleProperties++;
          }
          propView.refreshAllSelectors();
          this.propertyViews.push(propView);
        }.bind(this),
        onDone: function() {
          
          this.htmlComplete = true;
          this.propertyContainer.appendChild(fragment);
          this.noResults.hidden = this.numVisibleProperties > 0;
          this._refreshProcess = null;
          Services.obs.notifyObservers(null, "StyleInspector-populated", null);
        }.bind(this)});

      this._refreshProcess.schedule();
    }
  },

  


  refreshPanel: function CssHtmlTree_refreshPanel()
  {
    if (this._refreshProcess) {
      this._refreshProcess.cancel();
    }

    this.noResults.hidden = true;

    
    this.numVisibleProperties = 0;

    
    this._darkStripe = true;

    let display = this.propertyContainer.style.display;
    this._refreshProcess = new UpdateProcess(this.win, this.propertyViews, {
      onItem: function(aPropView) {
        aPropView.refresh();
      }.bind(this),
      onDone: function() {
        this._refreshProcess = null;
        this.noResults.hidden = this.numVisibleProperties > 0
        Services.obs.notifyObservers(null, "StyleInspector-populated", null);
      }.bind(this)
    });
    this._refreshProcess.schedule();
  },

  





  pathClick: function CssHtmlTree_pathClick(aEvent)
  {
    aEvent.preventDefault();
    if (aEvent.target && this.viewedElement != aEvent.target.pathElement) {
      this.styleInspector.selectFromPath(aEvent.target.pathElement);
    }
  },

  




  filterChanged: function CssHtmlTree_filterChanged(aEvent)
  {
    let win = this.styleWin.contentWindow;

    if (this._filterChangedTimeout) {
      win.clearTimeout(this._filterChangedTimeout);
    }

    this._filterChangedTimeout = win.setTimeout(function() {
      this.refreshPanel();
      this._filterChangeTimeout = null;
    }.bind(this), FILTER_CHANGED_TIMEOUT);
  },

  








  onlyUserStylesChanged: function CssHtmltree_onlyUserStylesChanged(aEvent)
  {
    this._matchedProperties = null;
    this.cssLogic.sourceFilter = this.showOnlyUserStyles ?
                                 CssLogic.FILTER.ALL :
                                 CssLogic.FILTER.UA;
    this.refreshPanel();
  },

  






  get pathElements()
  {
    return CssLogic.getShortNamePath(this.viewedElement);
  },

  


  createStyleViews: function CssHtmlTree_createStyleViews()
  {
    if (CssHtmlTree.propertyNames) {
      return;
    }

    CssHtmlTree.propertyNames = [];

    
    
    let styles = this.styleWin.contentWindow.getComputedStyle(this.styleDocument.documentElement);
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

  






  hasUnmatchedSelectors: function CssHtmlTree_hasUnmatchedSelectors(aProperty)
  {
    
    
    if (!this._unmatchedProperties) {
      let properties = [];
      CssHtmlTree.propertyNames.forEach(function(aName) {
        if (!this.matchedProperties[aName]) {
          properties.push(aName);
        }
      }, this);

      if (properties.indexOf(aProperty) == -1) {
        properties.push(aProperty);
      }

      this._unmatchedProperties = this.cssLogic.hasUnmatchedSelectors(properties);
    }

    
    if (!(aProperty in this._unmatchedProperties)) {
      let result = this.cssLogic.hasUnmatchedSelectors([aProperty]);
      this._unmatchedProperties[aProperty] = result[aProperty];
    }

    return this._unmatchedProperties[aProperty];
  },

  


  destroy: function CssHtmlTree_destroy()
  {
    delete this.viewedElement;

    
    this.onlyUserStylesCheckbox.removeEventListener("command",
      this.onlyUserStylesChanged);
    this.searchField.removeEventListener("command", this.filterChanged);

    
    delete this.root;
    delete this.path;
    delete this.templatePath;
    delete this.propertyContainer;
    delete this.panel;

    
    delete this.styleDocument;

    
    delete this.propertyViews;
    delete this.styleWin;
    delete this.cssLogic;
    delete this.doc;
    delete this.win;
    delete this.styleInspector;
  },
};









function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = aTree.getRTLAttr;

  this.link = "https://developer.mozilla.org/en/CSS/" + aName;

  this.templateMatchedSelectors = aTree.styleDocument.getElementById("templateMatchedSelectors");
}

PropertyView.prototype = {
  
  element: null,

  
  propertyHeader: null,

  
  valueNode: null,

  
  matchedExpanded: false,

  
  unmatchedExpanded: false,

  
  unmatchedSelectorTable: null,

  
  matchedSelectorsContainer: null,

  
  matchedExpander: null,

  
  unmatchedExpander: null,

  
  unmatchedSelectorsContainer: null,

  
  unmatchedTitleBlock: null,

  
  _matchedSelectorViews: null,

  
  _unmatchedSelectorViews: null,

  
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

  


  get hasUnmatchedSelectors()
  {
    return this.name in this.tree.hasUnmatchedSelectors;
  },

  


  get visible()
  {
    if (this.tree.showOnlyUserStyles && !this.hasMatchedSelectors) {
      return false;
    }

    let searchTerm = this.tree.searchField.value.toLowerCase();
    if (searchTerm && this.name.toLowerCase().indexOf(searchTerm) == -1 &&
      this.value.toLowerCase().indexOf(searchTerm) == -1) {
      return false;
    }

    return true;
  },

  




  get className()
  {
    if (this.visible) {
      this.tree._darkStripe = !this.tree._darkStripe;
      let darkValue = this.tree._darkStripe ?
                      "property-view darkrow" : "property-view";
      return darkValue;
    }
    return "property-view-hidden";
  },

  build: function PropertyView_build()
  {
    let doc = this.tree.doc;
    this.element = doc.createElementNS(HTML_NS, "div");
    this.element.setAttribute("class", this.className);

    this.propertyHeader = doc.createElementNS(XUL_NS, "hbox");
    this.element.appendChild(this.propertyHeader);
    this.propertyHeader.setAttribute("class", "property-header");
    this.propertyHeader.addEventListener("click", this.propertyHeaderClick.bind(this), false);

    this.matchedExpander = doc.createElementNS(HTML_NS, "div");
    this.propertyHeader.appendChild(this.matchedExpander);
    this.matchedExpander.setAttribute("class", "match expander");

    let name = doc.createElementNS(HTML_NS, "div");
    this.propertyHeader.appendChild(name);
    name.setAttribute("class", "property-name");
    name.textContent = this.name;

    let helpcontainer = doc.createElementNS(HTML_NS, "div");
    this.propertyHeader.appendChild(helpcontainer);
    helpcontainer.setAttribute("class", "helplink-container");

    let helplink = doc.createElementNS(HTML_NS, "a");
    helpcontainer.appendChild(helplink);
    helplink.setAttribute("class", "helplink");
    helplink.setAttribute("title", CssHtmlTree.HELP_LINK_TITLE);
    helplink.textContent = CssHtmlTree.HELP_LINK_TITLE;
    helplink.addEventListener("click", this.mdnLinkClick.bind(this), false);

    this.valueNode = doc.createElementNS(HTML_NS, "div");
    this.propertyHeader.appendChild(this.valueNode);
    this.valueNode.setAttribute("class", "property-value");
    this.valueNode.setAttribute("dir", "ltr");
    this.valueNode.textContent = this.value;

    this.matchedSelectorsContainer = doc.createElementNS(HTML_NS, "div");
    this.element.appendChild(this.matchedSelectorsContainer);
    this.matchedSelectorsContainer.setAttribute("class", "rulelink");

    return this.element;
  },

  


  refresh: function PropertyView_refresh()
  {
    this.element.className = this.className;

    if (this.prevViewedElement != this.tree.viewedElement) {
      this._matchedSelectorViews = null;
      this._unmatchedSelectorViews = null;
      this.prevViewedElement = this.tree.viewedElement;
    }

    if (!this.tree.viewedElement || !this.visible) {
      this.valueNode.innerHTML = "";
      this.matchedSelectorsContainer.hidden = true;
      this.matchedSelectorsContainer.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
      return;
    }

    this.tree.numVisibleProperties++;
    this.valueNode.innerHTML = this.propertyInfo.value;
    this.refreshAllSelectors();
  },

  


  refreshMatchedSelectors: function PropertyView_refreshMatchedSelectors()
  {
    let hasMatchedSelectors = this.hasMatchedSelectors;
    this.matchedSelectorsContainer.hidden = !hasMatchedSelectors;

    if (hasMatchedSelectors) {
      this.propertyHeader.classList.add("expandable");
    } else {
      this.propertyHeader.classList.remove("expandable");
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

  


  refreshUnmatchedSelectors: function PropertyView_refreshUnmatchedSelectors()
  {
    let hasMatchedSelectors = this.hasMatchedSelectors;

    this.unmatchedSelectorTable.hidden = !this.unmatchedExpanded;

    if (hasMatchedSelectors) {
      this.unmatchedSelectorsContainer.hidden = !this.matchedExpanded ||
        !this.hasUnmatchedSelectors;
      this.unmatchedTitleBlock.hidden = false;
    } else {
      this.unmatchedSelectorsContainer.hidden = !this.unmatchedExpanded;
      this.unmatchedTitleBlock.hidden = true;
    }

    if (this.unmatchedExpanded && this.hasUnmatchedSelectors) {
      CssHtmlTree.processTemplate(this.templateUnmatchedSelectors,
        this.unmatchedSelectorTable, this);
      if (!hasMatchedSelectors) {
        this.matchedExpander.setAttribute("open", "");
        this.unmatchedSelectorTable.classList.add("only-unmatched");
      } else {
        this.unmatchedExpander.setAttribute("open", "");
        this.unmatchedSelectorTable.classList.remove("only-unmatched");
      }
    } else {
      if (!hasMatchedSelectors) {
        this.matchedExpander.removeAttribute("open");
      }
      this.unmatchedExpander.removeAttribute("open");
      this.unmatchedSelectorTable.innerHTML = "";
    }
  },

  


  refreshAllSelectors: function PropertyView_refreshAllSelectors()
  {
    this.refreshMatchedSelectors();
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

    



  get unmatchedSelectorViews()
  {
    if (!this._unmatchedSelectorViews) {
      this._unmatchedSelectorViews = [];
      this.propertyInfo.unmatchedSelectors.forEach(
        function unmatchedSelectorViews_convert(aSelectorInfo) {
          this._unmatchedSelectorViews.push(new SelectorView(this.tree, aSelectorInfo));
        }, this);
    }

    return this._unmatchedSelectorViews;
  },

  






  propertyHeaderClick: function PropertyView_propertyHeaderClick(aEvent)
  {
    if (aEvent.target.className != "helplink") {
      this.matchedExpanded = !this.matchedExpanded;
      this.refreshAllSelectors();
      aEvent.preventDefault();
    }
  },

  


  unmatchedSelectorsClick: function PropertyView_unmatchedSelectorsClick(aEvent)
  {
    this.unmatchedExpanded = !this.unmatchedExpanded;
    this.refreshUnmatchedSelectors();
    aEvent.preventDefault();
  },

  


  mdnLinkClick: function PropertyView_mdnLinkClick(aEvent)
  {
    this.tree.win.openUILinkIn(this.link, "tab");
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
  "unmatched", "parentmatch", "matched", "bestmatch"
];

SelectorView.prototype = {
  








  _cacheStatusNames: function SelectorView_cacheStatusNames()
  {
    if (SelectorView.STATUS_NAMES.length) {
      return;
    }

    for (let status in CssLogic.STATUS) {
      let i = CssLogic.STATUS[status];
      if (i > -1) {
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
    return SelectorView.CLASS_NAMES[this.selectorInfo.status];
  },

  


  humanReadableText: function SelectorView_humanReadableText(aElement)
  {
    if (this.tree.getRTLAttr == "rtl") {
      return this.selectorInfo.value + " \u2190 " + this.text(aElement);
    } else {
      return this.text(aElement) + " \u2192 " + this.selectorInfo.value;
    }
  },

  text: function SelectorView_text(aElement) {
    let result = this.selectorInfo.selector.text;
    if (this.selectorInfo.elementStyle) {
      let source = this.selectorInfo.sourceElement;
      let IUI = this.tree.styleInspector.IUI;
      if (IUI && IUI.selection == source) {
        result = "this";
      } else {
        result = CssLogic.getShortName(source);
      }

      aElement.parentNode.querySelector(".rule-link > a").
        addEventListener("click", function(aEvent) {
          this.tree.styleInspector.selectFromPath(source);
          aEvent.preventDefault();
        }.bind(this), false);
      result += ".style";
    }

    return result;
  },
};
