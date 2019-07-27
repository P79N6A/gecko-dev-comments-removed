








"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function CSSUnprefixingService() {
}

CSSUnprefixingService.prototype = {
  
  classID:        Components.ID("{f0729490-e15c-4a2f-a3fb-99e1cc946b42}"),
  _xpcom_factory: XPCOMUtils.generateSingletonFactory(CSSUnprefixingService),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICSSUnprefixingService]),

  
  generateUnprefixedDeclaration: function(aPropName, aRightHalfOfDecl,
                                          aUnprefixedDecl ) {

    
    
    
    
    aPropName = aPropName.toLowerCase();
    aRightHalfOfDecl = aRightHalfOfDecl.toLowerCase();

    
    
    
    const propertiesThatAreJustAliases = {
      "-webkit-background-size":   "background-size",
      "-webkit-box-flex":          "flex-grow",
      "-webkit-box-ordinal-group": "order",
      "-webkit-box-sizing":        "box-sizing",
      "-webkit-transform":         "transform",
    };

    let unprefixedPropName = propertiesThatAreJustAliases[aPropName];
    if (unprefixedPropName !== undefined) {
      aUnprefixedDecl.value = unprefixedPropName + ":" + aRightHalfOfDecl;
      return true;
    }

    
    
    
    const propertiesThatNeedKeywordMapping = {
      "-webkit-box-align" : {
        unprefixedPropName : "align-items",
        valueMap : {
          "start"    : "flex-start",
          "center"   : "center",
          "end"      : "flex-end",
          "baseline" : "baseline",
          "stretch"  : "stretch"
        }
      },
      "-webkit-box-orient" : {
        unprefixedPropName : "flex-direction",
        valueMap : {
          "horizontal"  : "row",
          "inline-axis" : "row",
          "vertical"    : "column",
          "block-axis"  : "column"
        }
      },
      "-webkit-box-pack" : {
        unprefixedPropName : "justify-content",
        valueMap : {
          "start"    : "flex-start",
          "center"   : "center",
          "end"      : "flex-end",
          "justify"  : "space-between"
        }
      },
    };

    let propInfo = propertiesThatNeedKeywordMapping[aPropName];
    if (typeof(propInfo) != "undefined") {
      
      
      
      
      
      
      
      
      const keywordValuedPropertyRegexp = /^(\s*)([a-z\-]+)(.*)/;
      let parts = keywordValuedPropertyRegexp.exec(aRightHalfOfDecl);
      if (!parts) {
        
        
        return false;
      }

      let mappedKeyword = propInfo.valueMap[parts[2]];
      if (mappedKeyword === undefined) {
        
        
        return false;
      }

      aUnprefixedDecl.value = propInfo.unprefixedPropName + ":" +
        parts[1] + 
        mappedKeyword +
        parts[3]; 

      return true;
    }

    
    
    const propertiesThatNeedStringReplacement = {
      
      
      
      
      
      
      "-webkit-transition": {
        unprefixedPropName : "transition",
        stringMap : {
          "-webkit-transform" : "transform",
        }
      },
    };

    propInfo = propertiesThatNeedStringReplacement[aPropName];
    if (typeof(propInfo) != "undefined") {
      let newRightHalf = aRightHalfOfDecl;
      for (let strToReplace in propInfo.stringMap) {
        let replacement = propInfo.stringMap[strToReplace];
        newRightHalf = newRightHalf.split(strToReplace).join(replacement);
      }
      aUnprefixedDecl.value = propInfo.unprefixedPropName + ":" + newRightHalf;

      return true;
    }

    
    return false;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CSSUnprefixingService]);
