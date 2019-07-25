








































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
  this.panel = aPanel;

  
  this.viewedElement = null;
  this.viewedDocument = null;

  this.createStyleGroupViews();
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










CssHtmlTree.processTemplate = function CssHtmlTree_processTemplate(aTemplate, aDestination, aData)
{
  aDestination.innerHTML = "";

  
  
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
    this.viewedElement = aElement;

    
    
    let close = !aElement;
    this.styleGroups.forEach(function(group) group.reset(close));

    if (this.viewedElement) {
      this.viewedDocument = this.viewedElement.ownerDocument;
      CssHtmlTree.processTemplate(this.templateRoot, this.root, this);
    } else {
      this.viewedDocument = null;
      this.root.innerHTML = "";
    }
  },

  





  pathClick: function CssHtmlTree_pathClick(aEvent)
  {
    aEvent.preventDefault();
    if (aEvent.target && aEvent.target.pathElement) {
      if (this.win.InspectorUI.selection) {
        if (aEvent.target.pathElement != this.win.InspectorUI.selection) {
          this.win.InspectorUI.inspectNode(aEvent.target.pathElement);
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

  


  _getPropertiesByGroup: function CssHtmlTree_getPropertiesByGroup()
  {
    return {
      text: [
        "color",                    
        "color-interpolation",      
        "color-interpolation-filters", 
        "direction",                
        "fill",                     
        "fill-opacity",             
        "fill-rule",                
        "filter",                   
        "flood-color",              
        "flood-opacity",            
        "font-family",              
        "font-size",                
        "font-size-adjust",         
        "font-stretch",             
        "font-style",               
        "font-variant",             
        "font-weight",              
        "ime-mode",                 
        "letter-spacing",           
        "lighting-color",           
        "line-height",              
        "opacity",                  
        "quotes",                   
        "stop-color",               
        "stop-opacity",             
        "stroke-opacity",           
        "text-align",               
        "text-anchor",              
        "text-decoration",          
        "text-indent",              
        "text-overflow",            
        "text-rendering",           
        "text-shadow",              
        "text-transform",           
        "vertical-align",           
        "white-space",              
        "word-spacing",             
        "word-wrap",                
        "-moz-column-count",        
        "-moz-column-gap",          
        "-moz-column-rule-color",   
        "-moz-column-rule-style",   
        "-moz-column-rule-width",   
        "-moz-column-width",        
        "-moz-font-feature-settings",  
        "-moz-font-language-override", 
        "-moz-hyphens",                
        "-moz-text-decoration-color",  
        "-moz-text-decoration-style",  
        "-moz-text-decoration-line",   
        "-moz-text-blink",          
        "-moz-tab-size",            
      ],
      list: [
        "list-style-image",         
        "list-style-position",      
        "list-style-type",          
        "marker-end",               
        "marker-mid",               
        "marker-offset",            
        "marker-start",             
      ],
      background: [
        "background-attachment",    
        "background-clip",          
        "background-color",         
        "background-image",         
        "background-origin",        
        "background-position",      
        "background-repeat",        
        "background-size",          
        "-moz-appearance",          
        "-moz-background-inline-policy", 
      ],
      dims: [
        "width",                    
        "height",                   
        "max-width",                
        "max-height",               
        "min-width",                
        "min-height",               
        "margin-top",               
        "margin-right",             
        "margin-bottom",            
        "margin-left",              
        "padding-top",              
        "padding-right",            
        "padding-bottom",           
        "padding-left",             
        "clip",                     
        "clip-path",                
        "clip-rule",                
        "resize",                   
        "stroke-width",             
        "-moz-box-flex",            
        "-moz-box-sizing",          
      ],
      pos: [
        "top",                      
        "right",                    
        "bottom",                   
        "left",                     
        "display",                  
        "float",                    
        "clear",                    
        "position",                 
        "visibility",               
        "overflow",                 
        "overflow-x",               
        "overflow-y",               
        "z-index",                  
        "dominant-baseline",        
        "page-break-after",         
        "page-break-before",        
        "stroke-dashoffset",        
        "unicode-bidi",             
        "-moz-box-align",           
        "-moz-box-direction",       
        "-moz-box-ordinal-group",   
        "-moz-box-orient",          
        "-moz-box-pack",            
        "-moz-float-edge",          
        "-moz-orient",              
        "-moz-stack-sizing",        
      ],
      border: [
        "border-top-width",         
        "border-right-width",       
        "border-bottom-width",      
        "border-left-width",        
        "border-top-color",         
        "border-right-color",       
        "border-bottom-color",      
        "border-left-color",        
        "border-top-style",         
        "border-right-style",       
        "border-bottom-style",      
        "border-left-style",        
        "border-collapse",          
        "border-spacing",           
        "outline-offset",           
        "outline-style",            
        "outline-color",            
        "outline-width",            
        "border-top-left-radius",       
        "border-top-right-radius",      
        "border-bottom-right-radius",   
        "border-bottom-left-radius",    
        "-moz-border-bottom-colors",    
        "-moz-border-image",            
        "-moz-border-left-colors",      
        "-moz-border-right-colors",     
        "-moz-border-top-colors",       
        "-moz-outline-radius-topleft",      
        "-moz-outline-radius-topright",     
        "-moz-outline-radius-bottomright",  
        "-moz-outline-radius-bottomleft",   
      ],
      other: [
        "box-shadow",               
        "caption-side",             
        "content",                  
        "counter-increment",        
        "counter-reset",            
        "cursor",                   
        "empty-cells",              
        "image-rendering",          
        "mask",                     
        "pointer-events",           
        "shape-rendering",          
        "stroke",                   
        "stroke-dasharray",         
        "stroke-linecap",           
        "stroke-linejoin",          
        "stroke-miterlimit",        
        "table-layout",             
        "-moz-animation-delay",     
        "-moz-animation-direction", 
        "-moz-animation-duration",  
        "-moz-animation-fill-mode", 
        "-moz-animation-iteration-count", 
        "-moz-animation-name",            
        "-moz-animation-play-state",      
        "-moz-animation-timing-function", 
        "-moz-backface-visibility",       
        "-moz-binding",                   
        "-moz-force-broken-image-icon",   
        "-moz-image-region",        
        "-moz-perspective",         
        "-moz-perspective-origin",  
        "-moz-transform",           
        "-moz-transform-origin",    
        "-moz-transition-delay",    
        "-moz-transition-duration", 
        "-moz-transition-property", 
        "-moz-transition-timing-function", 
        "-moz-user-focus",          
        "-moz-user-input",          
        "-moz-user-modify",         
        "-moz-user-select",         
        "-moz-window-shadow",       
      ],
    };
  },

  


  createStyleGroupViews: function CssHtmlTree_createStyleGroupViews()
  {
    if (!CssHtmlTree.propertiesByGroup) {
      let pbg = CssHtmlTree.propertiesByGroup = this._getPropertiesByGroup();

      
      let mergedArray = Array.concat(
          pbg.text,
          pbg.list,
          pbg.background,
          pbg.dims,
          pbg.pos,
          pbg.border,
          pbg.other
      );

      
      
      
      let styles = this.styleWin.contentWindow.getComputedStyle(this.styleDocument.body);
      CssHtmlTree.supportedPropertyLookup = {};
      for (let i = 0, numStyles = styles.length; i < numStyles; i++) {
        let prop = styles.item(i);
        CssHtmlTree.supportedPropertyLookup[prop] = true;

        if (mergedArray.indexOf(prop) == -1) {
          pbg.other.push(prop);
        }
      }

      this.propertiesByGroup = CssHtmlTree.propertiesByGroup;
    }

    let pbg = CssHtmlTree.propertiesByGroup;

    
    this.styleGroups = [
      new StyleGroupView(this, "Text_Fonts_and_Color", pbg.text),
      new StyleGroupView(this, "Lists", pbg.list),
      new StyleGroupView(this, "Background", pbg.background),
      new StyleGroupView(this, "Dimensions", pbg.dims),
      new StyleGroupView(this, "Positioning_and_Page_Flow", pbg.pos),
      new StyleGroupView(this, "Borders", pbg.border),
      new StyleGroupView(this, "Effects_and_Other", pbg.other),
    ];
  },
};











function StyleGroupView(aTree, aId, aPropertyNames)
{
  this.tree = aTree;
  this.id = aId;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;
  this.localName = CssHtmlTree.l10n("group." + this.id);

  this.propertyViews = [];
  aPropertyNames.forEach(function(aPropertyName) {
    if (this.isPropertySupported(aPropertyName)) {
      this.propertyViews.push(new PropertyView(this.tree, this, aPropertyName));
    }
  }, this);

  this.populated = false;

  this.templateProperties = this.tree.styleDocument.getElementById("templateProperties");

  
  this.element = null;
  
  this.properties = null;
}

StyleGroupView.prototype = {
  


  click: function StyleGroupView_click()
  {
    
    if (this.element.hasAttribute("open")) {
      this.element.removeAttribute("open");
      return;
    }

    if (!this.populated) {
      CssHtmlTree.processTemplate(this.templateProperties, this.properties, this);
      this.populated = true;
    }

    this.element.setAttribute("open", "");
  },

  


  close: function StyleGroupView_close()
  {
    if (this.element) {
      this.element.removeAttribute("open");
    }
  },

  




  reset: function StyleGroupView_reset(aClosePanel)
  {
    this.close();
    this.populated = false;
    for (let i = 0, numViews = this.propertyViews.length; i < numViews; i++) {
      this.propertyViews[i].reset();
    }

    if (this.properties) {
      if (aClosePanel) {
        if (this.element) {
          this.element.removeChild(this.properties);
        }

        this.properties = null;
      } else {
        while (this.properties.hasChildNodes()) {
          this.properties.removeChild(this.properties.firstChild);
        }
      }
    }
  },

  






  isPropertySupported: function(aProperty) {
    return aProperty && aProperty in CssHtmlTree.supportedPropertyLookup;
  },
};











function PropertyView(aTree, aGroup, aName)
{
  this.tree = aTree;
  this.group = aGroup;
  this.name = aName;
  this.getRTLAttr = CssHtmlTree.getRTLAttr;

  this.populated = false;
  this.showUnmatched = false;

  this.link = "https://developer.mozilla.org/en/CSS/" + aName;

  this.templateRules = this.tree.styleDocument.getElementById("templateRules");

  
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

      let str = CssHtmlTree.l10n("property.numberOfRules");
      result = PluralForm.get(matchedRuleCount, str).replace("#1", matchedRuleCount);
    } else if (this.showUnmatchedLink) {
      aElement.classList.add("rule-unmatched");

      let unmatchedRuleCount = this.propertyInfo.unmatchedRuleCount;
      let str = CssHtmlTree.l10n("property.numberOfUnmatchedRules");
      result = PluralForm.get(unmatchedRuleCount, str).replace("#1", unmatchedRuleCount);
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
