



















































var EXPORTED_SYMBOLS = ["style"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

var style = {

  


  initialize: function CSS_initialize()
  {
    this.domUtils = Cc["@mozilla.org/inspector/dom-utils;1"].
      getService(Ci["inIDOMUtils"]);
  },

  





  isSystemStyleSheet: function CSS_isSystemStyleSheet(aSheet)
  {
    if (!aSheet)
      return true;

    let url = aSheet.href;

    if (!url)
      return false;
    if (url.length == 0)
      return true;
    if (url[0] == 'h') 
      return false;
    if (url.substr(0, 9) == "resource:")
      return true;
    if (url.substr(0, 7) == "chrome:")
      return true;
    if (url  == "XPCSafeJSObjectWrapper.cpp")
      return true;
    if (url.substr(0, 6) == "about:")
      return true;

    return false;
  },

  






  parseCSSProperties: function CSS_parseCSSProps(aStyle)
  {
    let properties = [];
    let lines = aStyle.cssText.match(/(?:[^;\(]*(?:\([^\)]*?\))?[^;\(]*)*;?/g);
    let propRE = /\s*([^:\s]*)\s*:\s*(.*?)\s*(! important)?;?$/;
    let line, i = 0;
    while(line = lines[i++]) {
      let match = propRE.exec(line);
      if (!match)
        continue;
      let name = match[1];
      let value = match[2];
      let important = !!match[3]; 
      properties.unshift({name: name, value: value, important: important});
    }

    return properties;
  },

  









  markOverriddenProperties: function CSS_markOverriddenProperties(aProps, aUsedProps, aInherit)
  {
    for (let i = 0; i < aProps.length; ++i) {
      let prop = aProps[i];
      if (aUsedProps.hasOwnProperty(prop.name)) {
        
        let deadProps = aUsedProps[prop.name];
        for (let j = 0; j < deadProps.length; ++j) {
          let deadProp = deadProps[j];
          if (!deadProp.disabled && !deadProp.wasInherited &&
              deadProp.important && !prop.important) {
            prop.overridden = true;  
          } else if (!prop.disabled) {
            deadProp.overridden = true;  
          } else {
            aUsedProps[prop.name] = [];
          }

          prop.wasInherited = aInherit ? true : false;
          
          aUsedProps[prop.name].push(prop);
        }
      }
    }
  },

  






  sortProperties: function CSS_sortProperties(properties)
  {
    properties.sort(function(a, b)
    {
      if (a.name < b.name) {
        return -1;
      }
      if (a.name > b.name) {
        return 1;
      }
      return 0;
    });
  },

  











  getStyleProperties: function CSS_getStyleProperties(aNode, aRules, aUsedProps, aInherit)
  {
    let properties = this.parseCSSProperties(aNode.style, aInherit);

    this.sortProperties(properties);
    this.markOverriddenProperties(properties, aUsedProps, aInherit);

    if (properties.length) {
      aRules.push({rule: aNode, selector: "element.style",
        properties: properties, inherited: aInherit});
    }
  },

  





  getRuleProperties: function CSS_getRuleProperties(aRule)
  {
    let style = aRule.style;
    return this.parseCSSProperties(style);
  },

  










  getInheritedRules: function CSS_getInheritedRules(aNode, aSections, aUsedProps)
  {
    let parent = aNode.parentNode;
    if (parent && parent.nodeType == 1) {
      this.getInheritedRules(parent, aSections, aUsedProps);

      let rules = [];
      this.getElementRules(parent, rules, aUsedProps, true);

      if (rules.length) {
        aSections.unshift({element: parent, rules: rules});
      }
    }
  },

  












  getElementRules: function CSS_getElementRules(aNode, aRules, aUsedProps, aInherit)
  {
    let inspectedRules;

    try {
      inspectedRules = this.domUtils.getCSSStyleRules(aNode);
    } catch (ex) {
      Services.console.logStringMessage(ex);
    }

    if (!inspectedRules)
      return;

    for (let i = 0; i < inspectedRules.Count(); ++i) {
      let rule = inspectedRules.GetElementAt(i);
      let href = rule.parentStyleSheet.href;

      if (!href) {
        
        href = aNode.ownerDocument.location.href;
      }

      let isSystemSheet = this.isSystemStyleSheet(rule.parentStyleSheet);

      if (isSystemSheet)
        continue;

      let properties = this.getRuleProperties(rule, aInherit);
      if (aInherit && !properties.length)
        continue;

      let line = this.domUtils.getRuleLine(rule);
      let ruleId = rule.selectorText + " " + href + " (" + line + ")";

      let sourceLink = "view-source:" + href + "#" + line;

      this.markOverriddenProperties(properties, aUsedProps, aInherit);

      aRules.unshift(
        {rule: rule,
         id: ruleId,
         selector: rule.selectorText,
         properties: properties,
         inherited: aInherit,
         sourceLink: sourceLink,
         isSystemSheet: isSystemSheet});
    }

    if (aNode.style) {
      this.getStyleProperties(aNode, aRules, aUsedProps, aInherit);
    }
  },
};

