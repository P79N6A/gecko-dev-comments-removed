









































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

  
  this.siBoundMenuUpdate = this.computedViewMenuUpdate.bind(this);
  this.siBoundCopy = this.computedViewCopy.bind(this);
  this.siBoundCopyDeclaration = this.computedViewCopyDeclaration.bind(this);
  this.siBoundCopyProperty = this.computedViewCopyProperty.bind(this);
  this.siBoundCopyPropertyValue = this.computedViewCopyPropertyValue.bind(this);

  
  this.styleDocument = this.styleWin.contentWindow.document;

  this.styleDocument.addEventListener("copy", this.siBoundCopy);

  
  this.root = this.styleDocument.getElementById("root");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");
  this.panel = aStyleInspector.panel;

  
  this.noResults = this.styleDocument.getElementById("noResults");

  
  this.viewedElement = null;
  this.createStyleViews();
  this.createContextMenu();
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

XPCOMUtils.defineLazyGetter(CssHtmlTree, "HELP_LINK_TITLE", function() {
  return CssHtmlTree.HELP_LINK_TITLE = CssHtmlTree.l10n("helpLinkTitle");
});

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
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
      let fragment = this.doc.createDocumentFragment();
      this._refreshProcess = new UpdateProcess(this.win, CssHtmlTree.propertyNames, {
        onItem: function(aPropertyName) {
          
          let propView = new PropertyView(this, aPropertyName);
          fragment.appendChild(propView.buildMain());
          fragment.appendChild(propView.buildSelectorContainer());

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
    this._refreshProcess = new UpdateProcess(this.win, this.propertyViews, {
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
    this.refreshSourceFilter();
    this.refreshPanel();
  },

  





  refreshSourceFilter: function CssHtmlTree_setSourceFilter()
  {
    this._matchedProperties = null;
    this.cssLogic.sourceFilter = this.showOnlyUserStyles ?
                                 CssLogic.FILTER.ALL :
                                 CssLogic.FILTER.UA;
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

  


  createContextMenu: function SI_createContextMenu()
  {
    let popupSet = this.doc.getElementById("mainPopupSet");

    let menu = this.doc.createElement("menupopup");
    menu.addEventListener("popupshowing", this.siBoundMenuUpdate);
    menu.id = "computed-view-context-menu";
    popupSet.appendChild(menu);

    
    let label = CssHtmlTree.l10n("style.contextmenu.copyselection");
    let accessKey = CssHtmlTree.l10n("style.contextmenu.copyselection.accesskey");
    let item = this.doc.createElement("menuitem");
    item.id = "computed-view-copy";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.siBoundCopy);
    menu.appendChild(item);

    
    label = CssHtmlTree.l10n("style.contextmenu.copydeclaration");
    accessKey = CssHtmlTree.l10n("style.contextmenu.copydeclaration.accesskey");
    item = this.doc.createElement("menuitem");
    item.id = "computed-view-copy-declaration";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.siBoundCopyDeclaration);
    menu.appendChild(item);

    
    label = CssHtmlTree.l10n("style.contextmenu.copyproperty");
    accessKey = CssHtmlTree.l10n("style.contextmenu.copyproperty.accesskey");
    item = this.doc.createElement("menuitem");
    item.id = "computed-view-copy-property";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.siBoundCopyProperty);
    menu.appendChild(item);

    
    label = CssHtmlTree.l10n("style.contextmenu.copypropertyvalue");
    accessKey = CssHtmlTree.l10n("style.contextmenu.copypropertyvalue.accesskey");
    item = this.doc.createElement("menuitem");
    item.id = "computed-view-copy-property-value";
    item.setAttribute("label", label);
    item.setAttribute("accesskey", accessKey);
    item.addEventListener("command", this.siBoundCopyPropertyValue);
    menu.appendChild(item);

    this.styleWin.setAttribute("context", menu.id);
  },

  



  computedViewMenuUpdate: function si_computedViewMenuUpdate()
  {
    let win = this.styleDocument.defaultView;
    let disable = win.getSelection().isCollapsed;
    let menuitem = this.doc.querySelector("#computed-view-copy");
    menuitem.disabled = disable;

    let node = this.doc.popupNode;
    if (!node) {
      return;
    }

    if (!node.classList.contains("property-view")) {
      while (node = node.parentElement) {
        if (node.classList.contains("property-view")) {
          break;
        }
      }
    }
    let disablePropertyItems = !node;
    menuitem = this.doc.querySelector("#computed-view-copy-declaration");
    menuitem.disabled = disablePropertyItems;
    menuitem = this.doc.querySelector("#computed-view-copy-property");
    menuitem.disabled = disablePropertyItems;
    menuitem = this.doc.querySelector("#computed-view-copy-property-value");
    menuitem.disabled = disablePropertyItems;
  },

  




  computedViewCopy: function si_computedViewCopy(aEvent)
  {
    let win = this.styleDocument.defaultView;
    let text = win.getSelection().toString();

    
    
    text = text.replace(/(.+)\r?\n\s+/g, "$1: ");

    
    text = text.replace(CssHtmlTree.HELP_LINK_TITLE, "");
    clipboardHelper.copyString(text);

    if (aEvent) {
      aEvent.preventDefault();
    }
  },

  




  computedViewCopyDeclaration: function si_computedViewCopyDeclaration(aEvent)
  {
    let node = this.doc.popupNode;
    if (!node) {
      return;
    }

    if (!node.classList.contains("property-view")) {
      while (node = node.parentElement) {
        if (node.classList.contains("property-view")) {
          break;
        }
      }
    }
    if (node) {
      let name = node.querySelector(".property-name").textContent;
      let value = node.querySelector(".property-value").textContent;

      clipboardHelper.copyString(name + ": " + value + ";");
    }
  },

  




  computedViewCopyProperty: function si_computedViewCopyProperty(aEvent)
  {
    let node = this.doc.popupNode;
    if (!node) {
      return;
    }

    if (!node.classList.contains("property-view")) {
      while (node = node.parentElement) {
        if (node.classList.contains("property-view")) {
          break;
        }
      }
    }
    if (node) {
      node = node.querySelector(".property-name");
      clipboardHelper.copyString(node.textContent);
    }
  },

  




  computedViewCopyPropertyValue: function si_computedViewCopyPropertyValue(aEvent)
  {
    let node = this.doc.popupNode;
    if (!node) {
      return;
    }

    if (!node.classList.contains("property-view")) {
      while (node = node.parentElement) {
        if (node.classList.contains("property-view")) {
          break;
        }
      }
    }
    if (node) {
      node = node.querySelector(".property-value");
      clipboardHelper.copyString(node.textContent);
    }
  },

  


  destroy: function CssHtmlTree_destroy()
  {
    delete this.viewedElement;

    
    this.onlyUserStylesCheckbox.removeEventListener("command",
      this.onlyUserStylesChanged);
    this.searchField.removeEventListener("command", this.filterChanged);

    
    if (this._refreshProcess) {
      this._refreshProcess.cancel();
    }

    
    let menu = this.doc.querySelector("#computed-view-context-menu");
    if (menu) {
      
      let menuitem = this.doc.querySelector("#computed-view-copy");
      menuitem.removeEventListener("command", this.siBoundCopy);

      
      menuitem = this.doc.querySelector("#computed-view-copy-declaration");
      menuitem.removeEventListener("command", this.siBoundCopyDeclaration);

      
      menuitem = this.doc.querySelector("#computed-view-copy-property");
      menuitem.removeEventListener("command", this.siBoundCopyProperty);

      
      menuitem = this.doc.querySelector("#computed-view-copy-property-value");
      menuitem.removeEventListener("command", this.siBoundCopyPropertyValue);

      menu.removeEventListener("popupshowing", this.siBoundMenuUpdate);
      menu.parentNode.removeChild(menu);
    }

    
    this.styleDocument.removeEventListener("copy", this.siBoundCopy);

    
    delete this.root;
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

  
  nameNode: null,

  
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

  




  get propertyHeaderClassName()
  {
    if (this.visible) {
      this.tree._darkStripe = !this.tree._darkStripe;
      let darkValue = this.tree._darkStripe ?
                      "property-view darkrow" : "property-view";
      return darkValue;
    }
    return "property-view-hidden";
  },

  




  get propertyContentClassName()
  {
    if (this.visible) {
      let darkValue = this.tree._darkStripe ?
                      "property-content darkrow" : "property-content";
      return darkValue;
    }
    return "property-content-hidden";
  },

  buildMain: function PropertyView_buildMain()
  {
    let doc = this.tree.doc;
    this.element = doc.createElementNS(HTML_NS, "tr");
    this.element.setAttribute("class", this.propertyHeaderClassName);

    this.propertyHeader = doc.createElementNS(HTML_NS, "td");
    this.element.appendChild(this.propertyHeader);
    this.propertyHeader.setAttribute("class", "property-header");

    this.matchedExpander = doc.createElementNS(HTML_NS, "div");
    this.matchedExpander.setAttribute("class", "match expander");
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
    this.propertyHeader.appendChild(this.matchedExpander);

    this.nameNode = doc.createElementNS(HTML_NS, "div");
    this.propertyHeader.appendChild(this.nameNode);
    this.nameNode.setAttribute("class", "property-name");
    this.nameNode.textContent = this.name;
    this.nameNode.addEventListener("click", function(aEvent) {
      this.matchedExpander.focus();
    }.bind(this), false);

    let helpcontainer = doc.createElementNS(HTML_NS, "td");
    this.element.appendChild(helpcontainer);
    helpcontainer.setAttribute("class", "helplink-container");

    let helplink = doc.createElementNS(HTML_NS, "a");
    helpcontainer.appendChild(helplink);
    helplink.setAttribute("class", "helplink");
    helplink.setAttribute("title", CssHtmlTree.HELP_LINK_TITLE);
    helplink.textContent = CssHtmlTree.HELP_LINK_TITLE;
    helplink.addEventListener("click", this.mdnLinkClick.bind(this), false);

    this.valueNode = doc.createElementNS(HTML_NS, "td");
    this.element.appendChild(this.valueNode);
    this.valueNode.setAttribute("class", "property-value");
    this.valueNode.setAttribute("dir", "ltr");
    this.valueNode.textContent = this.value;

    return this.element;
  },

  buildSelectorContainer: function PropertyView_buildSelectorContainer()
  {
    let doc = this.tree.doc;
    let element = doc.createElementNS(HTML_NS, "tr");
    element.setAttribute("class", this.propertyContentClassName);
    this.matchedSelectorsContainer = doc.createElementNS(HTML_NS, "td");
    this.matchedSelectorsContainer.setAttribute("colspan", "0");
    this.matchedSelectorsContainer.setAttribute("class", "rulelink");
    element.appendChild(this.matchedSelectorsContainer);

    return element;
  },

  


  refresh: function PropertyView_refresh()
  {
    this.element.className = this.propertyHeaderClassName;
    this.element.nextElementSibling.className = this.propertyContentClassName;

    if (this.prevViewedElement != this.tree.viewedElement) {
      this._matchedSelectorViews = null;
      this._unmatchedSelectorViews = null;
      this.prevViewedElement = this.tree.viewedElement;
    }

    if (!this.tree.viewedElement || !this.visible) {
      this.valueNode.innerHTML = "";
      this.matchedSelectorsContainer.parentNode.hidden = true;
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

  





  matchedExpanderClick: function PropertyView_matchedExpanderClick(aEvent)
  {
    this.matchedExpanded = !this.matchedExpanded;
    this.refreshAllSelectors();
    aEvent.preventDefault();
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
    let rule = this.selectorInfo.selector._cssRule;
    let doc = this.tree.win.content.document;
    let line = this.selectorInfo.ruleLine || 0;
    let cssSheet = rule._cssSheet;
    let contentSheet = false;
    let styleSheet;
    let styleSheets;

    if (cssSheet) {
      styleSheet = cssSheet.domSheet;
      styleSheets = doc.styleSheets;

      
      
      for each (let sheet in styleSheets) {
        if (sheet == styleSheet) {
          contentSheet = true;
          break;
        }
      }
    }

    if (contentSheet) {
      this.tree.win.StyleEditor.openChrome(styleSheet, line);
    } else {
      let href = styleSheet ? styleSheet.href : "";
      let viewSourceUtils = this.tree.win.gViewSourceUtils;

      if (this.selectorInfo.sourceElement) {
        href = this.selectorInfo.sourceElement.ownerDocument.location.href;
      }
      viewSourceUtils.viewSource(href, null, doc, line);
    }
  },
};
