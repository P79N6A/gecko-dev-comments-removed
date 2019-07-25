









































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = ["CssLogic"];

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
  BEST: 2,
  MATCHED: 1,
  PARENT_MATCH: 0,
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
      if (!aSheet.systemSheet && aSheet.sheetAllowed) {
        ruleCount += aSheet.ruleCount;
      }
    }, this);

    this._ruleCount = ruleCount;

    
    
    let needFullUpdate = (oldValue == CssLogic.FILTER.UA ||
        aValue == CssLogic.FILTER.UA);

    if (needFullUpdate) {
      this._matchedRules = null;
      this._matchedSelectors = null;
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
      if (!aSheet.systemSheet) {
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
      if (sheet.sheetAllowed && !sheet.systemSheet) {
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

  







  hasMatchedSelectors: function CL_hasMatchedSelectors(aProperties)
  {
    if (!this._matchedRules) {
      this._buildMatchedRules();
    }

    let result = {};

    this._matchedRules.some(function(aValue) {
      let rule = aValue[0];
      aProperties = aProperties.filter(function(aProperty) {
        if (rule.getPropertyValue(aProperty)) {
          
          
          result[aProperty] = true;
          return false;
        }
        return true; 
      });
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

        if (filter !== CssLogic.FILTER.UA && sheet.systemSheet) {
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








CssLogic.isSystemStyleSheet = function CssLogic_isSystemStyleSheet(aSheet)
{
  if (!aSheet) {
    return true;
  }

  let url = aSheet.href;

  if (!url) return false;
  if (url.length === 0) return true;

  
  if (url[0] === 'h') return false;
  if (url.substr(0, 9) === "resource:") return true;
  if (url.substr(0, 7) === "chrome:") return true;
  if (url === "XPCSafeJSObjectWrapper.cpp") return true;
  if (url.substr(0, 6) === "about:") return true;

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

  return aSheet.href;
}











function CssSheet(aCssLogic, aDomSheet, aIndex)
{
  this._cssLogic = aCssLogic;
  this.domSheet = aDomSheet;
  this.index = this.systemSheet ? -100 * aIndex : aIndex;

  
  this._href = null;
  
  this._shortSource = null;

  
  this._sheetAllowed = null;

  
  this._rules = {};

  this._ruleCount = -1;
}

CssSheet.prototype = {
  _passId: null,
  _systemSheet: null,
  _mediaMatches: null,

  





  get systemSheet()
  {
    if (this._systemSheet === null) {
      this._systemSheet = CssLogic.isSystemStyleSheet(this.domSheet);
    }
    return this._systemSheet;
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
    if (filter === CssLogic.FILTER.ALL && this.systemSheet) {
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

  if (this._cssSheet) {
    
    this._selectors = null;
    this.line = this._cssSheet._cssLogic.domUtils.getRuleLine(this._domRule);
    this.source = this._cssSheet.shortSource + ":" + this.line;
    this.href = this._cssSheet.href;
    this.systemRule = this._cssSheet.systemSheet;
  } else if (aElement) {
    this._selectors = [ new CssSelector(this, "@element.style") ];
    this.line = -1;
    this.source = CssLogic.l10n("rule.sourceElement");
    this.href = "#";
    this.systemRule = false;
    this.sourceElement = aElement;
  }
}

CssRule.prototype = {
  _passId: null,

  





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

  





  get systemRule()
  {
    return this._cssRule.systemRule;
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

  








  get specificity()
  {
    if (this._specificity) {
      return this._specificity;
    }

    let specificity = {};

    specificity.ids = 0;
    specificity.classes = 0;
    specificity.tags = 0;

    
    
    if (!this.elementStyle) {
      this.text.split(/[ >+]/).forEach(function(aSimple) {
        
        if (!aSimple) {
          return;
        }
        
        
        specificity.ids += (aSimple.match(/#/g) || []).length;
        
        specificity.classes += (aSimple.match(/\./g) || []).length;
        specificity.classes += (aSimple.match(/\[/g) || []).length;
        
        specificity.tags += (aSimple.match(/:/g) || []).length;
        
        
        let tag = aSimple.split(/[#.[:]/)[0];
        if (tag && tag != "*") {
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

  







  get matchedSelectors()
  {
    if (!this._matchedSelectors) {
      this._findMatchedSelectors();
    } else if (this.needRefilter) {
      this._refilterSelectors();
    }

    return this._matchedSelectors;
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
        this._matchedSelectors[0].status > CssLogic.STATUS.UNKNOWN) {
      this._matchedSelectors[0].status = CssLogic.STATUS.BEST;
    }
  },

  






  _processMatchedSelector: function CssPropertyInfo_processMatchedSelector(aSelector, aStatus)
  {
    let cssRule = aSelector._cssRule;
    let value = cssRule.getPropertyValue(this.property);
    if (value) {
      let selectorInfo = new CssSelectorInfo(aSelector, this.property, value,
          aStatus);
      this._matchedSelectors.push(selectorInfo);
      if (this._cssLogic._passId !== cssRule._passId && cssRule.sheetAllowed) {
        this._matchedRuleCount++;
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

  







  let scorePrefix = this.systemRule ? 0 : 2;
  if (this.elementStyle) {
    scorePrefix++;
  }
  if (this.important) {
    scorePrefix += this.systemRule ? 1 : 2;
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

  





  get systemRule()
  {
    return this.selector.systemRule;
  },

  






  compareTo: function CssSelectorInfo_compareTo(aThat)
  {
    if (this.systemRule && !aThat.systemRule) return 1;
    if (!this.systemRule && aThat.systemRule) return -1;

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
