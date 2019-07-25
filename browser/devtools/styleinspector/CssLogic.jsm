







































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const RX_UNIVERSAL_SELECTOR = /\s*\*\s*/g;
const RX_NOT = /:not\((.*?)\)/g;
const RX_PSEUDO_CLASS_OR_ELT = /(:[\w-]+\().*?\)/g;
const RX_CONNECTORS = /\s*[\s>+~]\s*/g;
const RX_ID = /\s*#\w+\s*/g;
const RX_CLASS_OR_ATTRIBUTE = /\s*(?:\.\w+|\[.+?\])\s*/g;
const RX_PSEUDO = /\s*:?:([\w-]+)(\(?\)?)\s*/g;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["CssLogic", "CssSelector"];

function CssLogic()
{
  
  _propertyInfos: {};
}




CssLogic.FILTER = {
  ALL: "all", 
  UA: "ua",   
};








CssLogic.MEDIA = {
  ALL: "all",
  SCREEN: "screen",
};








CssLogic.STATUS = {
  BEST: 3,
  MATCHED: 2,
  PARENT_MATCH: 1,
  UNMATCHED: 0,
  UNKNOWN: -1,
};

CssLogic.prototype = {
  
  viewedElement: null,
  viewedDocument: null,

  
  _sheets: null,

  
  _sheetsCached: false,

  
  _ruleCount: 0,

  
  _computedStyle: null,

  
  _sourceFilter: CssLogic.FILTER.ALL,

  
  
  _passId: 0,

  
  
  _matchId: 0,

  _matchedRules: null,
  _matchedSelectors: null,
  _unmatchedSelectors: null,

  domUtils: Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils),

  


  reset: function CssLogic_reset()
  {
    this._propertyInfos = {};
    this._ruleCount = 0;
    this._sheetIndex = 0;
    this._sheets = {};
    this._sheetsCached = false;
    this._matchedRules = null;
    this._matchedSelectors = null;
    this._unmatchedSelectors = null;
  },

  





  highlight: function CssLogic_highlight(aViewedElement)
  {
    if (!aViewedElement) {
      this.viewedElement = null;
      this.viewedDocument = null;
      this._computedStyle = null;
      this.reset();
      return;
    }

    this.viewedElement = aViewedElement;

    let doc = this.viewedElement.ownerDocument;
    if (doc != this.viewedDocument) {
      
      this.viewedDocument = doc;

      
      this._cacheSheets();
    } else {
      
      this._propertyInfos = {};
    }

    this._matchedRules = null;
    this._matchedSelectors = null;
    this._unmatchedSelectors = null;
    let win = this.viewedDocument.defaultView;
    this._computedStyle = win.getComputedStyle(this.viewedElement, "");
  },

  



  get sourceFilter() {
    return this._sourceFilter;
  },

  





  set sourceFilter(aValue) {
    let oldValue = this._sourceFilter;
    this._sourceFilter = aValue;

    let ruleCount = 0;

    
    this.forEachSheet(function(aSheet) {
      aSheet._sheetAllowed = -1;
      if (aSheet.contentSheet && aSheet.sheetAllowed) {
        ruleCount += aSheet.ruleCount;
      }
    }, this);

    this._ruleCount = ruleCount;

    
    
    let needFullUpdate = (oldValue == CssLogic.FILTER.UA ||
        aValue == CssLogic.FILTER.UA);

    if (needFullUpdate) {
      this._matchedRules = null;
      this._matchedSelectors = null;
      this._unmatchedSelectors = null;
      this._propertyInfos = {};
    } else {
      
      for each (let propertyInfo in this._propertyInfos) {
        propertyInfo.needRefilter = true;
      }
    }
  },

  








  getPropertyInfo: function CssLogic_getPropertyInfo(aProperty)
  {
    if (!this.viewedElement) {
      return {};
    }

    let info = this._propertyInfos[aProperty];
    if (!info) {
      info = new CssPropertyInfo(this, aProperty);
      this._propertyInfos[aProperty] = info;
    }

    return info;
  },

  



  _cacheSheets: function CssLogic_cacheSheets()
  {
    this._passId++;
    this.reset();

    
    Array.prototype.forEach.call(this.viewedDocument.styleSheets,
        this._cacheSheet, this);

    this._sheetsCached = true;
  },

  








  _cacheSheet: function CssLogic_cacheSheet(aDomSheet)
  {
    if (aDomSheet.disabled) {
      return;
    }

    
    if (!this.mediaMatches(aDomSheet)) {
      return;
    }

    
    let cssSheet = this.getSheet(aDomSheet, this._sheetIndex++);
    if (cssSheet._passId != this._passId) {
      cssSheet._passId = this._passId;

      
      Array.prototype.forEach.call(aDomSheet.cssRules, function(aDomRule) {
        if (aDomRule.type == Ci.nsIDOMCSSRule.IMPORT_RULE && aDomRule.styleSheet &&
            this.mediaMatches(aDomRule)) {
          this._cacheSheet(aDomRule.styleSheet);
        }
      }, this);
    }
  },

  




  get sheets()
  {
    if (!this._sheetsCached) {
      this._cacheSheets();
    }

    let sheets = [];
    this.forEachSheet(function (aSheet) {
      if (aSheet.contentSheet) {
        sheets.push(aSheet);
      }
    }, this);

    return sheets;
  },

  









  getSheet: function CL_getSheet(aDomSheet, aIndex)
  {
    let cacheId = "";

    if (aDomSheet.href) {
      cacheId = aDomSheet.href;
    } else if (aDomSheet.ownerNode && aDomSheet.ownerNode.ownerDocument) {
      cacheId = aDomSheet.ownerNode.ownerDocument.location;
    }

    let sheet = null;
    let sheetFound = false;

    if (cacheId in this._sheets) {
      for (let i = 0, numSheets = this._sheets[cacheId].length; i < numSheets; i++) {
        sheet = this._sheets[cacheId][i];
        if (sheet.domSheet === aDomSheet) {
          if (aIndex != -1) {
            sheet.index = aIndex;
          }
          sheetFound = true;
          break;
        }
      }
    }

    if (!sheetFound) {
      if (!(cacheId in this._sheets)) {
        this._sheets[cacheId] = [];
      }

      sheet = new CssSheet(this, aDomSheet, aIndex);
      if (sheet.sheetAllowed && sheet.contentSheet) {
        this._ruleCount += sheet.ruleCount;
      }

      this._sheets[cacheId].push(sheet);
    }

    return sheet;
  },

  







  forEachSheet: function CssLogic_forEachSheet(aCallback, aScope)
  {
    for each (let sheet in this._sheets) {
      sheet.forEach(aCallback, aScope);
    }
  },

  










  forSomeSheets: function CssLogic_forSomeSheets(aCallback, aScope)
  {
    for each (let sheets in this._sheets) {
      if (sheets.some(aCallback, aScope)) {
        return true;
      }
    }
    return false;
  },

  










  get ruleCount()
  {
    if (!this._sheetsCached) {
      this._cacheSheets();
    }

    return this._ruleCount;
  },

  















  processMatchedSelectors: function CL_processMatchedSelectors(aCallback, aScope)
  {
    if (this._matchedSelectors) {
      if (aCallback) {
        this._passId++;
        this._matchedSelectors.forEach(function(aValue) {
          aCallback.call(aScope, aValue[0], aValue[1]);
          aValue[0]._cssRule._passId = this._passId;
        }, this);
      }
      return;
    }

    if (!this._matchedRules) {
      this._buildMatchedRules();
    }

    this._matchedSelectors = [];
    this._unmatchedSelectors = null;
    this._passId++;

    for (let i = 0; i < this._matchedRules.length; i++) {
      let rule = this._matchedRules[i][0];
      let status = this._matchedRules[i][1];

      rule.selectors.forEach(function (aSelector) {
        if (aSelector._matchId !== this._matchId &&
            (aSelector.elementStyle ||
             this._selectorMatchesElement(aSelector))) {
          aSelector._matchId = this._matchId;
          this._matchedSelectors.push([ aSelector, status ]);
          if (aCallback) {
            aCallback.call(aScope, aSelector, status);
          }
        }
      }, this);

      rule._passId = this._passId;
    }
  },

  








  _selectorMatchesElement: function CL__selectorMatchesElement(aSelector)
  {
    let element = this.viewedElement;
    do {
      if (element.mozMatchesSelector(aSelector)) {
        return true;
      }
    } while ((element = element.parentNode) &&
             element.nodeType === Ci.nsIDOMNode.ELEMENT_NODE);

    return false;
  },

  











  processUnmatchedSelectors: function CL_processUnmatchedSelectors(aCallback, aScope)
  {
    if (this._unmatchedSelectors) {
      if (aCallback) {
        this._unmatchedSelectors.forEach(aCallback, aScope);
      }
      return;
    }

    if (!this._matchedSelectors) {
      this.processMatchedSelectors();
    }

    this._unmatchedSelectors = [];

    this.forEachSheet(function (aSheet) {
      
      if (!aSheet.contentSheet || aSheet.disabled || !aSheet.mediaMatches) {
        return;
      }

      aSheet.forEachRule(function (aRule) {
        aRule.selectors.forEach(function (aSelector) {
          if (aSelector._matchId !== this._matchId) {
            this._unmatchedSelectors.push(aSelector);
            if (aCallback) {
              aCallback.call(aScope, aSelector);
            }
          }
        }, this);
      }, this);
    }, this);
  },

  







  hasMatchedSelectors: function CL_hasMatchedSelectors(aProperties)
  {
    if (!this._matchedRules) {
      this._buildMatchedRules();
    }

    let result = {};

    this._matchedRules.some(function(aValue) {
      let rule = aValue[0];
      let status = aValue[1];
      aProperties = aProperties.filter(function(aProperty) {
        
        
        if (rule.getPropertyValue(aProperty) &&
            (status == CssLogic.STATUS.MATCHED ||
             (status == CssLogic.STATUS.PARENT_MATCH &&
              this.domUtils.isInheritedProperty(aProperty)))) {
          result[aProperty] = true;
          return false;
        }
        return true; 
      }.bind(this));
      return aProperties.length == 0;
    }, this);

    return result;
  },

  





  _buildMatchedRules: function CL__buildMatchedRules()
  {
    let domRules;
    let element = this.viewedElement;
    let filter = this.sourceFilter;
    let sheetIndex = 0;

    this._matchId++;
    this._passId++;
    this._matchedRules = [];

    if (!element) {
      return;
    }

    do {
      let status = this.viewedElement === element ?
                   CssLogic.STATUS.MATCHED : CssLogic.STATUS.PARENT_MATCH;

      try {
        domRules = this.domUtils.getCSSStyleRules(element);
      } catch (ex) {
        Services.console.
          logStringMessage("CL__buildMatchedRules error: " + ex);
        continue;
      }

      for (let i = 0, n = domRules.Count(); i < n; i++) {
        let domRule = domRules.GetElementAt(i);
        if (domRule.type !== Ci.nsIDOMCSSRule.STYLE_RULE) {
          continue;
        }

        let sheet = this.getSheet(domRule.parentStyleSheet, -1);
        if (sheet._passId !== this._passId) {
          sheet.index = sheetIndex++;
          sheet._passId = this._passId;
        }

        if (filter === CssLogic.FILTER.ALL && !sheet.contentSheet) {
          continue;
        }

        let rule = sheet.getRule(domRule);
        if (rule._passId === this._passId) {
          continue;
        }

        rule._matchId = this._matchId;
        rule._passId = this._passId;
        this._matchedRules.push([rule, status]);
      }


      
      if (element.style.length > 0) {
        let rule = new CssRule(null, { style: element.style }, element);
        rule._matchId = this._matchId;
        rule._passId = this._passId;
        this._matchedRules.push([rule, status]);
      }
    } while ((element = element.parentNode) &&
              element.nodeType === Ci.nsIDOMNode.ELEMENT_NODE);
  },

  










  hasUnmatchedSelectors: function CL_hasUnmatchedSelectors(aProperties)
  {
    if (!this._matchedRules) {
      this._buildMatchedRules();
    }

    let result = {};

    this.forSomeSheets(function (aSheet) {
      if (!aSheet.contentSheet || aSheet.disabled || !aSheet.mediaMatches) {
        return false;
      }

      return aSheet.forSomeRules(function (aRule) {
        let unmatched = aRule._matchId !== this._matchId ||
                        this._ruleHasUnmatchedSelector(aRule);
        if (!unmatched) {
          return false;
        }

        aProperties = aProperties.filter(function(aProperty) {
          if (!aRule.getPropertyValue(aProperty)) {
            
            
            return true;
          }

          result[aProperty] = true;

          
          
          
          return false;
        });

        return aProperties.length == 0;
      }, this);
    }, this);

    aProperties.forEach(function(aProperty) { result[aProperty] = false; });

    return result;
  },

  









  _ruleHasUnmatchedSelector: function CL__ruleHasUnmatchedSelector(aRule)
  {
    if (!aRule._cssSheet && aRule.sourceElement) {
      
      return false;
    }

    let element = this.viewedElement;
    let selectors = aRule.selectors;

    do {
      selectors = selectors.filter(function(aSelector) {
        return !element.mozMatchesSelector(aSelector);
      });

      if (selectors.length == 0) {
        break;
      }
    } while ((element = element.parentNode) &&
             element.nodeType === Ci.nsIDOMNode.ELEMENT_NODE);

    return selectors.length > 0;
  },

  






  mediaMatches: function CL_mediaMatches(aDomObject)
  {
    let mediaText = aDomObject.media.mediaText;
    return !mediaText || this.viewedDocument.defaultView.
                         matchMedia(mediaText).matches;
   },
};











CssLogic.getShortName = function CssLogic_getShortName(aElement)
{
  if (!aElement) {
    return "null";
  }
  if (aElement.id) {
    return "#" + aElement.id;
  }
  let priorSiblings = 0;
  let temp = aElement;
  while (temp = temp.previousElementSibling) {
    priorSiblings++;
  }
  return aElement.tagName + "[" + priorSiblings + "]";
};













CssLogic.getShortNamePath = function CssLogic_getShortNamePath(aElement)
{
  let doc = aElement.ownerDocument;
  let reply = [];

  if (!aElement) {
    return reply;
  }

  
  
  do {
    reply.unshift({
      display: CssLogic.getShortName(aElement),
      element: aElement
    });
    aElement = aElement.parentNode;
  } while (aElement && aElement != doc.body && aElement != doc.head && aElement != doc);

  return reply;
};






CssLogic.l10n = function(aName) CssLogic._strings.GetStringFromName(aName);

XPCOMUtils.defineLazyGetter(CssLogic, "_strings", function() Services.strings
        .createBundle("chrome://browser/locale/devtools/styleinspector.properties"));








CssLogic.isContentStylesheet = function CssLogic_isContentStylesheet(aSheet)
{
  
  if (aSheet.ownerNode) {
    return true;
  }

  
  if (aSheet.ownerRule instanceof Ci.nsIDOMCSSImportRule) {
    return CssLogic.isContentStylesheet(aSheet.parentStyleSheet);
  }

  return false;
};






CssLogic.shortSource = function CssLogic_shortSource(aSheet)
{
  
  if (!aSheet || !aSheet.href) {
    return CssLogic.l10n("rule.sourceInline");
  }

  
  let url = {};
  try {
    url = Services.io.newURI(aSheet.href, null, null);
    url = url.QueryInterface(Ci.nsIURL);
  } catch (ex) {
    
  }

  if (url.fileName) {
    return url.fileName;
  }

  if (url.filePath) {
    return url.filePath;
  }

  if (url.query) {
    return url.query;
  }

  let dataUrl = aSheet.href.match(/^(data:[^,]*),/);
  return dataUrl ? dataUrl[1] : aSheet.href;
}











function CssSheet(aCssLogic, aDomSheet, aIndex)
{
  this._cssLogic = aCssLogic;
  this.domSheet = aDomSheet;
  this.index = this.contentSheet ? aIndex : -100 * aIndex;

  
  this._href = null;
  
  this._shortSource = null;

  
  this._sheetAllowed = null;

  
  this._rules = {};

  this._ruleCount = -1;
}

CssSheet.prototype = {
  _passId: null,
  _contentSheet: null,
  _mediaMatches: null,

  





  get contentSheet()
  {
    if (this._contentSheet === null) {
      this._contentSheet = CssLogic.isContentStylesheet(this.domSheet);
    }
    return this._contentSheet;
  },

  



  get disabled()
  {
    return this.domSheet.disabled;
  },

  




  get mediaMatches()
  {
    if (this._mediaMatches === null) {
      this._mediaMatches = this._cssLogic.mediaMatches(this.domSheet);
    }
    return this._mediaMatches;
  },

  






  get href()
  {
    if (!this._href) {
      this._href = this.domSheet.href;
      if (!this._href) {
        this._href = this.domSheet.ownerNode.ownerDocument.location;
      }
    }

    return this._href;
  },

  




  get shortSource()
  {
    if (this._shortSource) {
      return this._shortSource;
    }

    this._shortSource = CssLogic.shortSource(this.domSheet);
    return this._shortSource;
  },

  





  get sheetAllowed()
  {
    if (this._sheetAllowed !== null) {
      return this._sheetAllowed;
    }

    this._sheetAllowed = true;

    let filter = this._cssLogic.sourceFilter;
    if (filter === CssLogic.FILTER.ALL && !this.contentSheet) {
      this._sheetAllowed = false;
    }
    if (filter !== CssLogic.FILTER.ALL && filter !== CssLogic.FILTER.UA) {
      this._sheetAllowed = (filter === this.href);
    }

    return this._sheetAllowed;
  },

  




  get ruleCount()
  {
    return this._ruleCount > -1 ?
        this._ruleCount :
        this.domSheet.cssRules.length;
  },

  









  getRule: function CssSheet_getRule(aDomRule)
  {
    let cacheId = aDomRule.type + aDomRule.selectorText;

    let rule = null;
    let ruleFound = false;

    if (cacheId in this._rules) {
      for (let i = 0, rulesLen = this._rules[cacheId].length; i < rulesLen; i++) {
        rule = this._rules[cacheId][i];
        if (rule._domRule === aDomRule) {
          ruleFound = true;
          break;
        }
      }
    }

    if (!ruleFound) {
      if (!(cacheId in this._rules)) {
        this._rules[cacheId] = [];
      }

      rule = new CssRule(this, aDomRule);
      this._rules[cacheId].push(rule);
    }

    return rule;
  },

  












  forEachRule: function CssSheet_forEachRule(aCallback, aScope)
  {
    let ruleCount = 0;
    let domRules = this.domSheet.cssRules;

    function _iterator(aDomRule) {
      if (aDomRule.type == Ci.nsIDOMCSSRule.STYLE_RULE) {
        aCallback.call(aScope, this.getRule(aDomRule));
        ruleCount++;
      } else if (aDomRule.type == Ci.nsIDOMCSSRule.MEDIA_RULE &&
          aDomRule.cssRules && this._cssLogic.mediaMatches(aDomRule)) {
        Array.prototype.forEach.call(aDomRule.cssRules, _iterator, this);
      }
    }

    Array.prototype.forEach.call(domRules, _iterator, this);

    this._ruleCount = ruleCount;
  },

  















  forSomeRules: function CssSheet_forSomeRules(aCallback, aScope)
  {
    let domRules = this.domSheet.cssRules;
    function _iterator(aDomRule) {
      if (aDomRule.type == Ci.nsIDOMCSSRule.STYLE_RULE) {
        return aCallback.call(aScope, this.getRule(aDomRule));
      } else if (aDomRule.type == Ci.nsIDOMCSSRule.MEDIA_RULE &&
          aDomRule.cssRules && this._cssLogic.mediaMatches(aDomRule)) {
        return Array.prototype.some.call(aDomRule.cssRules, _iterator, this);
      }
    }
    return Array.prototype.some.call(domRules, _iterator, this);
  },

  toString: function CssSheet_toString()
  {
    return "CssSheet[" + this.shortSource + "]";
  },
};














function CssRule(aCssSheet, aDomRule, aElement)
{
  this._cssSheet = aCssSheet;
  this._domRule = aDomRule;

  let parentRule = aDomRule.parentRule;
  if (parentRule && parentRule.type == Ci.nsIDOMCSSRule.MEDIA_RULE) {
    this.mediaText = parentRule.media.mediaText;
  }

  if (this._cssSheet) {
    
    this._selectors = null;
    this.line = this._cssSheet._cssLogic.domUtils.getRuleLine(this._domRule);
    this.source = this._cssSheet.shortSource + ":" + this.line;
    if (this.mediaText) {
      this.source += " @media " + this.mediaText;
    }
    this.href = this._cssSheet.href;
    this.contentRule = this._cssSheet.contentSheet;
  } else if (aElement) {
    this._selectors = [ new CssSelector(this, "@element.style") ];
    this.line = -1;
    this.source = CssLogic.l10n("rule.sourceElement");
    this.href = "#";
    this.contentRule = true;
    this.sourceElement = aElement;
  }
}

CssRule.prototype = {
  _passId: null,

  mediaText: "",

  get isMediaRule()
  {
    return !!this.mediaText;
  },

  





  get sheetAllowed()
  {
    return this._cssSheet ? this._cssSheet.sheetAllowed : true;
  },

  





  get sheetIndex()
  {
    return this._cssSheet ? this._cssSheet.index : 0;
  },

  






  getPropertyValue: function(aProperty)
  {
    return this._domRule.style.getPropertyValue(aProperty);
  },

  






  getPropertyPriority: function(aProperty)
  {
    return this._domRule.style.getPropertyPriority(aProperty);
  },

  





  get selectors()
  {
    if (this._selectors) {
      return this._selectors;
    }

    
    this._selectors = [];

    if (!this._domRule.selectorText) {
      return this._selectors;
    }

    let selector = this._domRule.selectorText.trim();
    if (!selector) {
      return this._selectors;
    }

    let nesting = 0;
    let currentSelector = [];

    
    
    
    for (let i = 0, selLen = selector.length; i < selLen; i++) {
      let c = selector.charAt(i);
      switch (c) {
        case ",":
          if (nesting == 0 && currentSelector.length > 0) {
            let selectorStr = currentSelector.join("").trim();
            if (selectorStr) {
              this._selectors.push(new CssSelector(this, selectorStr));
            }
            currentSelector = [];
          } else {
            currentSelector.push(c);
          }
          break;

        case "(":
          nesting++;
          currentSelector.push(c);
          break;

        case ")":
          nesting--;
          currentSelector.push(c);
          break;

        default:
          currentSelector.push(c);
          break;
      }
    }

    
    if (nesting == 0 && currentSelector.length > 0) {
      let selectorStr = currentSelector.join("").trim();
      if (selectorStr) {
        this._selectors.push(new CssSelector(this, selectorStr));
      }
    }

    return this._selectors;
  },

  toString: function CssRule_toString()
  {
    return "[CssRule " + this._domRule.selectorText + "]";
  },
};









function CssSelector(aCssRule, aSelector)
{
  this._cssRule = aCssRule;
  this.text = aSelector;
  this.elementStyle = this.text == "@element.style";
  this._specificity = null;
}

CssSelector.prototype = {
  _matchId: null,

  





  get source()
  {
    return this._cssRule.source;
  },

  






  get sourceElement()
  {
    return this._cssRule.sourceElement;
  },

  





  get href()
  {
    return this._cssRule.href;
  },

  





  get contentRule()
  {
    return this._cssRule.contentRule;
  },

  





  get sheetAllowed()
  {
    return this._cssRule.sheetAllowed;
  },

  





  get sheetIndex()
  {
    return this._cssRule.sheetIndex;
  },

  





  get ruleLine()
  {
    return this._cssRule.line;
  },

  



  get pseudoElements()
  {
    if (!CssSelector._pseudoElements) {
      let pseudos = CssSelector._pseudoElements = new Set();
      pseudos.add("after");
      pseudos.add("before");
      pseudos.add("first-letter");
      pseudos.add("first-line");
      pseudos.add("selection");
      pseudos.add("-moz-focus-inner");
      pseudos.add("-moz-focus-outer");
      pseudos.add("-moz-list-bullet");
      pseudos.add("-moz-list-number");
      pseudos.add("-moz-math-anonymous");
      pseudos.add("-moz-math-stretchy");
      pseudos.add("-moz-progress-bar");
      pseudos.add("-moz-selection");
    }
    return CssSelector._pseudoElements;
  },

  








  get specificity()
  {
    if (this._specificity) {
      return this._specificity;
    }

    let specificity = {
      ids: 0,
      classes: 0,
      tags: 0
    };

    let text = this.text;

    if (!this.elementStyle) {
      
      
      text = text.replace(RX_UNIVERSAL_SELECTOR, "");

      
      
      text = text.replace(RX_NOT, " $1");

      
      text = text.replace(RX_PSEUDO_CLASS_OR_ELT, " $1)");

      
      text = text.replace(RX_CONNECTORS, " ");

      text.split(/\s/).forEach(function(aSimple) {
        
        aSimple = aSimple.replace(RX_ID, function() {
          specificity.ids++;
          return "";
        });

        
        aSimple = aSimple.replace(RX_CLASS_OR_ATTRIBUTE, function() {
          specificity.classes++;
          return "";
        });

        aSimple = aSimple.replace(RX_PSEUDO, function(aDummy, aPseudoName) {
          if (this.pseudoElements.has(aPseudoName)) {
            
            specificity.tags++;
          } else {
            
            specificity.classes++;
          }
          return "";
        }.bind(this));

        if (aSimple) {
          specificity.tags++;
        }
      }, this);
    }
    this._specificity = specificity;

    return this._specificity;
  },

  toString: function CssSelector_toString()
  {
    return this.text;
  },
};














function CssPropertyInfo(aCssLogic, aProperty)
{
  this._cssLogic = aCssLogic;
  this.property = aProperty;
  this._value = "";

  
  
  this._matchedRuleCount = 0;

  
  
  
  
  this._matchedSelectors = null;
  this._unmatchedSelectors = null;
}

CssPropertyInfo.prototype = {
  






  get value()
  {
    if (!this._value && this._cssLogic._computedStyle) {
      try {
        this._value = this._cssLogic._computedStyle.getPropertyValue(this.property);
      } catch (ex) {
        Services.console.logStringMessage('Error reading computed style for ' +
          this.property);
        Services.console.logStringMessage(ex);
      }
    }

    return this._value;
  },

  





  get matchedRuleCount()
  {
    if (!this._matchedSelectors) {
      this._findMatchedSelectors();
    } else if (this.needRefilter) {
      this._refilterSelectors();
    }

    return this._matchedRuleCount;
  },

  





  get unmatchedRuleCount()
  {
    if (!this._unmatchedSelectors) {
      this._findUnmatchedSelectors();
    } else if (this.needRefilter) {
      this._refilterSelectors();
    }

    return this._unmatchedRuleCount;
  },

  







  get matchedSelectors()
  {
    if (!this._matchedSelectors) {
      this._findMatchedSelectors();
    } else if (this.needRefilter) {
      this._refilterSelectors();
    }

    return this._matchedSelectors;
  },

  







  get unmatchedSelectors()
  {
    if (!this._unmatchedSelectors) {
      this._findUnmatchedSelectors();
    } else if (this.needRefilter) {
      this._refilterSelectors();
    }

    return this._unmatchedSelectors;
  },

  






  _findMatchedSelectors: function CssPropertyInfo_findMatchedSelectors()
  {
    this._matchedSelectors = [];
    this._matchedRuleCount = 0;
    this.needRefilter = false;

    this._cssLogic.processMatchedSelectors(this._processMatchedSelector, this);

    
    this._matchedSelectors.sort(function(aSelectorInfo1, aSelectorInfo2) {
      if (aSelectorInfo1.status > aSelectorInfo2.status) {
        return -1;
      } else if (aSelectorInfo2.status > aSelectorInfo1.status) {
        return 1;
      } else {
        return aSelectorInfo1.compareTo(aSelectorInfo2);
      }
    });

    
    if (this._matchedSelectors.length > 0 &&
        this._matchedSelectors[0].status > CssLogic.STATUS.UNMATCHED) {
      this._matchedSelectors[0].status = CssLogic.STATUS.BEST;
    }
  },

  






  _processMatchedSelector: function CssPropertyInfo_processMatchedSelector(aSelector, aStatus)
  {
    let cssRule = aSelector._cssRule;
    let value = cssRule.getPropertyValue(this.property);
    if (value &&
        (aStatus == CssLogic.STATUS.MATCHED ||
         (aStatus == CssLogic.STATUS.PARENT_MATCH &&
          this._cssLogic.domUtils.isInheritedProperty(this.property)))) {
      let selectorInfo = new CssSelectorInfo(aSelector, this.property, value,
          aStatus);
      this._matchedSelectors.push(selectorInfo);
      if (this._cssLogic._passId !== cssRule._passId && cssRule.sheetAllowed) {
        this._matchedRuleCount++;
      }
    }
  },

  




  _findUnmatchedSelectors: function CssPropertyInfo_findUnmatchedSelectors()
  {
    this._unmatchedSelectors = [];
    this._unmatchedRuleCount = 0;
    this.needRefilter = false;
    this._cssLogic._passId++;

    this._cssLogic.processUnmatchedSelectors(this._processUnmatchedSelector,
        this);

    
    this._unmatchedSelectors.sort(function(aSelectorInfo1, aSelectorInfo2) {
      return aSelectorInfo1.compareTo(aSelectorInfo2);
    });
  },

  






  _processUnmatchedSelector: function CPI_processUnmatchedSelector(aSelector)
  {
    let cssRule = aSelector._cssRule;
    let value = cssRule.getPropertyValue(this.property);
    if (value) {
      let selectorInfo = new CssSelectorInfo(aSelector, this.property, value,
          CssLogic.STATUS.UNMATCHED);
      this._unmatchedSelectors.push(selectorInfo);
      if (this._cssLogic._passId != cssRule._passId) {
        if (cssRule.sheetAllowed) {
          this._unmatchedRuleCount++;
        }
        cssRule._passId = this._cssLogic._passId;
      }
    }
  },

  




  _refilterSelectors: function CssPropertyInfo_refilterSelectors()
  {
    let passId = ++this._cssLogic._passId;
    let ruleCount = 0;

    let iterator = function(aSelectorInfo) {
      let cssRule = aSelectorInfo.selector._cssRule;
      if (cssRule._passId != passId) {
        if (cssRule.sheetAllowed) {
          ruleCount++;
        }
        cssRule._passId = passId;
      }
    };

    if (this._matchedSelectors) {
      this._matchedSelectors.forEach(iterator);
      this._matchedRuleCount = ruleCount;
    }

    if (this._unmatchedSelectors) {
      ruleCount = 0;
      this._unmatchedSelectors.forEach(iterator);
      this._unmatchedRuleCount = ruleCount;
    }

    this.needRefilter = false;
  },

  toString: function CssPropertyInfo_toString()
  {
    return "CssPropertyInfo[" + this.property + "]";
  },
};















function CssSelectorInfo(aSelector, aProperty, aValue, aStatus)
{
  this.selector = aSelector;
  this.property = aProperty;
  this.value = aValue;
  this.status = aStatus;

  let priority = this.selector._cssRule.getPropertyPriority(this.property);
  this.important = (priority === "important");

  







  let scorePrefix = this.contentRule ? 2 : 0;
  if (this.elementStyle) {
    scorePrefix++;
  }
  if (this.important) {
    scorePrefix += this.contentRule ? 2 : 1;
  }

  this.specificityScore = "" + scorePrefix + this.specificity.ids +
      this.specificity.classes + this.specificity.tags;
}

CssSelectorInfo.prototype = {
  





  get source()
  {
    return this.selector.source;
  },

  






  get sourceElement()
  {
    return this.selector.sourceElement;
  },

  





  get href()
  {
    return this.selector.href;
  },

  





  get elementStyle()
  {
    return this.selector.elementStyle;
  },

  





  get specificity()
  {
    return this.selector.specificity;
  },

  





  get sheetIndex()
  {
    return this.selector.sheetIndex;
  },

  





  get sheetAllowed()
  {
    return this.selector.sheetAllowed;
  },

  





  get ruleLine()
  {
    return this.selector.ruleLine;
  },

  





  get contentRule()
  {
    return this.selector.contentRule;
  },

  






  compareTo: function CssSelectorInfo_compareTo(aThat)
  {
    if (!this.contentRule && aThat.contentRule) return 1;
    if (this.contentRule && !aThat.contentRule) return -1;

    if (this.elementStyle && !aThat.elementStyle) {
      if (!this.important && aThat.important) return 1;
      else return -1;
    }

    if (!this.elementStyle && aThat.elementStyle) {
      if (this.important && !aThat.important) return -1;
      else return 1;
    }

    if (this.important && !aThat.important) return -1;
    if (aThat.important && !this.important) return 1;

    if (this.specificity.ids > aThat.specificity.ids) return -1;
    if (aThat.specificity.ids > this.specificity.ids) return 1;

    if (this.specificity.classes > aThat.specificity.classes) return -1;
    if (aThat.specificity.classes > this.specificity.classes) return 1;

    if (this.specificity.tags > aThat.specificity.tags) return -1;
    if (aThat.specificity.tags > this.specificity.tags) return 1;

    if (this.sheetIndex > aThat.sheetIndex) return -1;
    if (aThat.sheetIndex > this.sheetIndex) return 1;

    if (this.ruleLine > aThat.ruleLine) return -1;
    if (aThat.ruleLine > this.ruleLine) return 1;

    return 0;
  },

  toString: function CssSelectorInfo_toString()
  {
    return this.selector + " -> " + this.value;
  },
};
