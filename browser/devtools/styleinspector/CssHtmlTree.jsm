








































const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource:///modules/devtools/Templater.jsm");

var EXPORTED_SYMBOLS = ["CssHtmlTree"];










function CssHtmlTree(aStyleWin, aCssLogic, aPanel)
{
  this.styleWin = aStyleWin;
  this.cssLogic = aCssLogic;
  this.doc = aPanel.ownerDocument;
  this.win = this.doc.defaultView;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;

  
  this.styleDocument = this.styleWin.contentWindow.document;

  
  this.root = this.styleDocument.getElementById("root");
  this.templateRoot = this.styleDocument.getElementById("templateRoot");
  this.propertyContainer = this.styleDocument.getElementById("propertyContainer");
  this.templateProperty = this.styleDocument.getElementById("templateProperty");
  this.panel = aPanel;

  
  this.viewedElement = null;
  this.viewedDocument = null;

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
  



  highlight: function CssHtmlTree_highlight(aElement)
  {
    if (this.viewedElement == aElement) {
      return;
    }

    this.viewedElement = aElement;

    if (this.viewedElement) {
      this.viewedDocument = this.viewedElement.ownerDocument;
      CssHtmlTree.processTemplate(this.templateRoot, this.root, this);
    } else {
      this.viewedDocument = null;
      this.root.innerHTML = "";
    }

    this.propertyContainer.innerHTML = "";

    
    
    
    let i = 0;
    let batchSize = 25;
    let max = CssHtmlTree.propertyNames.length - 1;
    function displayProperties() {
      if (this.viewedElement == aElement && this.panel.isOpen()) {
        
        for (let step = i + batchSize; i < step && i <= max; i++) {
          let propView = new PropertyView(this, CssHtmlTree.propertyNames[i]);
          CssHtmlTree.processTemplate(
              this.templateProperty, this.propertyContainer, propView, true);
        }
        if (i < max) {
          
          
          this.win.setTimeout(displayProperties.bind(this), 0);
        }
      }
    }
    this.win.setTimeout(displayProperties.bind(this), 0);
  },

  





  pathClick: function CssHtmlTree_pathClick(aEvent)
  {
    aEvent.preventDefault();
    if (aEvent.target && this.viewedElement != aEvent.target.pathElement) {
      this.propertyContainer.innerHTML = "";
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
};









function PropertyView(aTree, aName)
{
  this.tree = aTree;
  this.name = aName;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;

  this.populated = false;
  this.showUnmatched = false;

  this.link = "https://developer.mozilla.org/en/CSS/" + aName;

  this.templateRules = aTree.styleDocument.getElementById("templateRules");

  
  this.element = null;
  
  this.rules = null;

  this.str = {};
}

PropertyView.prototype = {
  






  click: function PropertyView_click(aEvent)
  {
    
    if (aEvent.target.tagName.toLowerCase() == "a") {
      return;
    }

    if (this.element.hasAttribute("open")) {
      this.element.removeAttribute("open");
      return;
    }

    if (!this.populated) {
      let matchedRuleCount = this.propertyInfo.matchedRuleCount;

      if (matchedRuleCount == 0 && this.showUnmatchedLink) {
        this.showUnmatchedLinkClick(aEvent);
      } else {
        CssHtmlTree.processTemplate(this.templateRules, this.rules, this);
      }
      this.populated = true;
    }
    this.element.setAttribute("open", "");
  },

  





  get value()
  {
    return this.propertyInfo.value;
  },

  


  get propertyInfo()
  {
    return this.tree.cssLogic.getPropertyInfo(this.name);
  },

  







  ruleTitle: function PropertyView_ruleTitle(aElement)
  {
    let result = "";
    let matchedRuleCount = this.propertyInfo.matchedRuleCount;

    if (matchedRuleCount > 0) {
      aElement.classList.add("rule-count");
      aElement.firstElementChild.className = "expander";

      let str = CssHtmlTree.l10n("property.numberOfRules");
      result = PluralForm.get(matchedRuleCount, str)
          .replace("#1", matchedRuleCount);
    } else if (this.showUnmatchedLink) {
      aElement.classList.add("rule-unmatched");
      aElement.firstElementChild.className = "expander";

      let unmatchedRuleCount = this.propertyInfo.unmatchedRuleCount;
      let str = CssHtmlTree.l10n("property.numberOfUnmatchedRules");
      result = PluralForm.get(unmatchedRuleCount, str)
          .replace("#1", unmatchedRuleCount);
    }
    return result;
  },

  


  close: function PropertyView_close()
  {
    if (this.rules && this.element) {
      this.element.removeAttribute("open");
    }
  },

  


  reset: function PropertyView_reset()
  {
    this.close();
    this.populated = false;
    this.showUnmatched = false;
    this.element = false;
  },

  


  get selectorViews()
  {
    var all = [];

    function convert(aSelectorInfo) {
      all.push(new SelectorView(aSelectorInfo));
    }

    this.propertyInfo.matchedSelectors.forEach(convert);
    if (this.showUnmatched) {
      this.propertyInfo.unmatchedSelectors.forEach(convert);
    }

    return all;
  },

  




  get showUnmatchedLink()
  {
    return !this.showUnmatched && this.propertyInfo.unmatchedRuleCount > 0;
  },

  



  get showUnmatchedLinkText()
  {
    let smur = CssHtmlTree.l10n("rule.showUnmatchedLink");
    let plural = PluralForm.get(this.propertyInfo.unmatchedRuleCount, smur);
    return plural.replace("#1", this.propertyInfo.unmatchedRuleCount);
  },

  


  showUnmatchedLinkClick: function PropertyView_showUnmatchedLinkClick(aEvent)
  {
    this.showUnmatched = true;
    CssHtmlTree.processTemplate(this.templateRules, this.rules, this);
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
