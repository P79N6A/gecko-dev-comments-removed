









































const Cu = Components.utils;
const FILTER_CHANGED_TIMEOUT = 300;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource:///modules/devtools/Templater.jsm");

var EXPORTED_SYMBOLS = ["CssHtmlTree", "PropertyView"];










function CssHtmlTree(aStyleWin, aCssLogic, aPanel)
{
  this.styleWin = aStyleWin;
  this.cssLogic = aCssLogic;
  this.doc = aPanel.ownerDocument;
  this.win = this.doc.defaultView;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;
  this.propertyViews = {};

  
  this.styleDocument = this.styleWin.contentWindow.document;

  
  this.root = this.styleDocument.getElementById("root");
  this.path = this.styleDocument.getElementById("path");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.templatePath = this.styleDocument.getElementById("templatePath");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");
  this.templateProperty = this.styleDocument.getElementById("templateProperty");
  this.panel = aPanel;

  
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





CssHtmlTree.isRTL = function CssHtmlTree_isRTL()
{
  return CssHtmlTree.getRTLAttr == "rtl";
};





XPCOMUtils.defineLazyGetter(CssHtmlTree, "getRTLAttr", function() {
  let mainWindow = Services.wm.getMostRecentWindow("navigator:browser");
  return mainWindow.getComputedStyle(mainWindow.gBrowser).direction;
});

XPCOMUtils.defineLazyGetter(CssHtmlTree, "_strings", function() Services.strings
    .createBundle("chrome:

CssHtmlTree.prototype = {
  htmlComplete: false,

  
  filterChangedTimeout: null,

  
  searchField: null,
  
  
  onlyUserStylesCheckbox: null,

  get showOnlyUserStyles()
  {
    return this.onlyUserStylesCheckbox.checked;
  },

  




  highlight: function CssHtmlTree_highlight(aElement)
  {
    if (this.viewedElement == aElement) {
      return;
    }

    this.viewedElement = aElement;

    CssHtmlTree.processTemplate(this.templatePath, this.path, this);

    if (this.htmlComplete) {
      this.refreshPanel();
    } else {
      CssHtmlTree.processTemplate(this.templateRoot, this.root, this);

      
      
      let i = 0;
      let batchSize = 15;
      let max = CssHtmlTree.propertyNames.length - 1;
      function displayProperties() {
        if (this.viewedElement == aElement && this.panel.isOpen()) {
          
          for (let step = i + batchSize; i < step && i <= max; i++) {
            let name = CssHtmlTree.propertyNames[i];
            let propView = new PropertyView(this, name);
            CssHtmlTree.processTemplate(this.templateProperty,
              this.propertyContainer, propView, true);
            propView.refreshMatchedSelectors();
            propView.refreshUnmatchedSelectors();
            this.propertyViews[name] = propView;
          }
          if (i < max) {
            
            
            this.win.setTimeout(displayProperties.bind(this), 50);
          } else {
            this.htmlComplete = true;
            Services.obs.notifyObservers(null, "StyleInspector-populated", null);
          }
        }
      }
      this.win.setTimeout(displayProperties.bind(this), 50);
    }
  },

  


  refreshPanel: function CssHtmlTree_refreshPanel()
  {
    for each(let propView in this.propertyViews) {
      propView.refresh();
    }
    Services.obs.notifyObservers(null, "StyleInspector-populated", null);
  },

  





  pathClick: function CssHtmlTree_pathClick(aEvent)
  {
    aEvent.preventDefault();
    if (aEvent.target && this.viewedElement != aEvent.target.pathElement) {
      if (this.win.InspectorUI.selection) {
        if (aEvent.target.pathElement != this.win.InspectorUI.selection) {
          let elt = aEvent.target.pathElement;
          this.win.InspectorUI.inspectNode(elt);
          this.panel.selectNode(elt);
        }
      } else {
        this.panel.selectNode(aEvent.target.pathElement);
      }
    }
  },

  




  filterChanged: function CssHtmlTree_filterChanged(aEvent)
  {
    let win = this.styleWin.contentWindow;

    if (this.filterChangedTimeout) {
      win.clearTimeout(this.filterChangedTimeout);
      this.filterChangeTimeout = null;
    }

    this.filterChangedTimeout = win.setTimeout(function() {
      this.refreshPanel();
    }.bind(this), FILTER_CHANGED_TIMEOUT);
  },

  








  onlyUserStylesChanged: function CssHtmltree_onlyUserStylesChanged(aEvent)
  {
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

    
    
    let styles = this.styleWin.contentWindow.getComputedStyle(this.styleDocument.body);
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

  


  destroy: function CssHtmlTree_destroy()
  {
    delete this.viewedElement;

    
    delete this.root;
    delete this.path;
    delete this.templateRoot;
    delete this.templatePath;
    delete this.propertyContainer;
    delete this.templateProperty;
    delete this.panel;

    
    delete this.styleDocument;

    
    delete this.propertyViews;
    delete this.getRTLAttr;
    delete this.styleWin;
    delete this.cssLogic;
    delete this.doc;
    delete this.win;
  },
};









function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;

  this.link = "https://developer.mozilla.org/en/CSS/" + aName;

  this.templateMatchedSelectors = aTree.styleDocument.getElementById("templateMatchedSelectors");
  this.templateUnmatchedSelectors = aTree.styleDocument.getElementById("templateUnmatchedSelectors");
}

PropertyView.prototype = {
  
  element: null,

  
  valueNode: null,

  
  matchedExpanded: false,

  
  unmatchedExpanded: false,

  
  matchedSelectorsContainer: null,

  
  unmatchedSelectorsContainer: null,

  
  matchedExpander: null,

  
  unmatchedExpander: null,

  
  matchedSelectorsTitleNode: null,

  
  unmatchedSelectorsTitleNode: null,

  
  matchedSelectorTable: null,

  
  unmatchedSelectorTable: null,

  
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

  


  get visible()
  {
    if (this.tree.showOnlyUserStyles && this.matchedSelectorCount == 0) {
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
    return this.visible ? "property-view" : "property-view-hidden";
  },

  


  get matchedSelectorCount()
  {
    return this.propertyInfo.matchedSelectors.length;
  },

  


  get unmatchedSelectorCount()
  {
    return this.propertyInfo.unmatchedSelectors.length;
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
      this.unmatchedSelectorsContainer.hidden = true;
      this.matchedSelectorTable.innerHTML = "";
      this.unmatchedSelectorTable.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
      this.unmatchedExpander.removeAttribute("open");
      return;
    }

    this.valueNode.innerHTML = this.propertyInfo.value;
    
    this.refreshMatchedSelectors();
    this.refreshUnmatchedSelectors();
  },

  


  refreshMatchedSelectors: function PropertyView_refreshMatchedSelectors()
  {
    this.matchedSelectorsTitleNode.innerHTML = this.matchedSelectorTitle();
    this.matchedSelectorsContainer.hidden = this.matchedSelectorCount == 0;

    if (this.matchedExpanded && this.matchedSelectorCount > 0) {
      CssHtmlTree.processTemplate(this.templateMatchedSelectors,
        this.matchedSelectorTable, this);
      this.matchedExpander.setAttribute("open", "");
    } else {
      this.matchedSelectorTable.innerHTML = "";
      this.matchedExpander.removeAttribute("open");
    }
  },

  


  refreshUnmatchedSelectors: function PropertyView_refreshUnmatchedSelectors() {
    this.unmatchedSelectorsTitleNode.innerHTML = this.unmatchedSelectorTitle();
    this.unmatchedSelectorsContainer.hidden = this.unmatchedSelectorCount == 0;

    if (this.unmatchedExpanded && this.unmatchedSelectorCount > 0) {
      CssHtmlTree.processTemplate(this.templateUnmatchedSelectors,
          this.unmatchedSelectorTable, this);
      this.unmatchedExpander.setAttribute("open", "");
    } else {
      this.unmatchedSelectorTable.innerHTML = "";
      this.unmatchedExpander.removeAttribute("open");
    }
  },

  





  matchedSelectorTitle: function PropertyView_matchedSelectorTitle()
  {
    let result = "";

    if (this.matchedSelectorCount > 0) {
      let str = CssHtmlTree.l10n("property.numberOfMatchedSelectors");
      result = PluralForm.get(this.matchedSelectorCount, str)
                         .replace("#1", this.matchedSelectorCount);
    }
    return result;
  },

  





  unmatchedSelectorTitle: function PropertyView_unmatchedSelectorTitle()
  {
    let result = "";

    if (this.unmatchedSelectorCount > 0) {
      let str = CssHtmlTree.l10n("property.numberOfUnmatchedSelectors");
      result = PluralForm.get(this.unmatchedSelectorCount, str)
                         .replace("#1", this.unmatchedSelectorCount);
    }
    return result;
  },

  



  get matchedSelectorViews()
  {
    if (!this._matchedSelectorViews) {
      this._matchedSelectorViews = [];
      this.propertyInfo.matchedSelectors.forEach(
        function matchedSelectorViews_convert(aSelectorInfo) {
          this._matchedSelectorViews.push(new SelectorView(aSelectorInfo));
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
          this._unmatchedSelectorViews.push(new SelectorView(aSelectorInfo));
        }, this);
    }

    return this._unmatchedSelectorViews;
  },

  


  matchedSelectorsClick: function PropertyView_matchedSelectorsClick(aEvent)
  {
    this.matchedExpanded = !this.matchedExpanded;
    this.refreshMatchedSelectors();
    aEvent.preventDefault();
  },

  


  unmatchedSelectorsClick: function PropertyView_unmatchedSelectorsClick(aEvent)
  {
    this.unmatchedExpanded = !this.unmatchedExpanded;
    this.refreshUnmatchedSelectors();
    aEvent.preventDefault();
  },
};




function SelectorView(aSelectorInfo)
{
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
    if (CssHtmlTree.isRTL()) {
      return this.selectorInfo.value + " \u2190 " + this.text(aElement);
    } else {
      return this.text(aElement) + " \u2192 " + this.selectorInfo.value;
    }
  },

  text: function SelectorView_text(aElement) {
    let result = this.selectorInfo.selector.text;
    if (this.selectorInfo.elementStyle) {
      if (this.selectorInfo.sourceElement == this.win.InspectorUI.selection) {
        result = "this";
      } else {
        result = CssLogic.getShortName(this.selectorInfo.sourceElement);
        aElement.parentNode.querySelector(".rule-link > a").
          addEventListener("click", function(aEvent) {
            this.win.InspectorUI.inspectNode(this.selectorInfo.sourceElement);
            aEvent.preventDefault();
          }, false);
      }

      result += ".style";
    }
    return result;
  },
};
