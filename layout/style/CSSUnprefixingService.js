








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

  
  generateUnprefixedGradientValue: function(aPrefixedFuncName,
                                            aPrefixedFuncBody,
                                            aUnprefixedFuncName, 
                                            aUnprefixedFuncBody ) {
    var unprefixedFuncName, newValue;
    if (aPrefixedFuncName == "-webkit-gradient") {
      
      var parts = this.oldGradientParser(aPrefixedFuncBody);
      var type = parts[0].name;
      newValue = this.standardizeOldGradientArgs(type, parts.slice(1));
      unprefixedFuncName = type + "-gradient";
    }else{ 
        
        
        if (aPrefixedFuncName != "-webkit-linear-gradient" &&
            aPrefixedFuncName != "-webkit-radial-gradient") {
          
          return false;
        }
        unprefixedFuncName = aPrefixedFuncName.replace(/-webkit-/, '');

        
        
        newValue = aPrefixedFuncBody.replace(/(top|bottom|left|right)+\s*(top|bottom|left|right)*/, function(str){
            var words = str.split(/\s+/);
            for(var i=0; i<words.length; i++){
                switch(words[i].toLowerCase()){
                    case 'top':
                        words[i] = 'bottom';
                        break;
                    case 'bottom':
                        words[i] = 'top';
                        break;
                    case 'left':
                        words[i] = 'right';
                        break;
                    case 'right':
                        words[i] = 'left';
                }
            }
            str = words.join(' ');
            return ( 'to ' + str);
        });

        newValue = newValue.replace(/\d+deg/, function (val) {
             return (360 - (parseInt(val)-90))+'deg';
         });

    }
    aUnprefixedFuncName.value = unprefixedFuncName;
    aUnprefixedFuncBody.value = newValue;
    return true;
  },

  
  
  oldGradientParser : function(str){
    



    var objs = [{}], path=[], current, word='', separator_chars = [',', '(', ')'];
    current = objs[0], path[0] = objs;
    
    for(var i = 0; i < str.length; i++){
        if(separator_chars.indexOf(str[i]) === -1){
            word += str[i];
        }else{ 
            current.name = word.trim();
            
            word = '';
            if(str[i] === '('){ 
                if(!('args' in current)){
                   current.args = [];
                }
                current.args.push({});
                path.push(current.args);
                current = current.args[current.args.length - 1];
                path.push(current);
            }else if(str[i] === ')'){ 
                current = path.pop(); 
                current = path.pop(); 
            }else{
                path.pop(); 
                var current_parent = path[path.length - 1] || objs; 
                current_parent.push({}); 
                current = current_parent[current_parent.length - 1]; 
                path.push(current);

            }
        }
    }

    return objs;
  },

  








  standardizeOldGradientArgs : function(type, args){
    var stdArgStr = "";
    var stops = [];
    if(/^linear/.test(type)){
        
        var points = [].concat(args[0].name.split(/\s+/), args[1].name.split(/\s+/)); 
        
        
        var rxPercTest = /\d+\%/;
        if(rxPercTest.test(points[0]) || points[0] == 0){
            var startX = parseInt(points[0]), startY = parseInt(points[1]), endX = parseInt(points[2]), endY = parseInt(points[3]);
            stdArgStr +=  ((Math.atan2(endY- startY, endX - startX)) * (180 / Math.PI)+90) + 'deg';
        }else{
            if(points[1] === points[3]){ 
                stdArgStr += 'to ' + points[2];
            }else if(points[0] === points[2]){ 
                stdArgStr += 'to ' + points[3];
            }else if(points[1] === 'top'){ 
                stdArgStr += '135deg';
            }else{
                stdArgStr += '45deg';
            }
        }

    }else if(/^radial/i.test(type)){ 
        stdArgStr += 'circle ' + args[3].name.replace(/(\d+)$/, '$1px') + ' at ' + args[0].name.replace(/(\d+) /, '$1px ').replace(/(\d+)$/, '$1px');
    }

    var toColor;
    for(var j = type === 'linear' ? 2 : 4; j < args.length; j++){
        var position, color, colorIndex;
        if(args[j].name === 'color-stop'){
            position = args[j].args[0].name;
            colorIndex = 1;
        }else if (args[j].name === 'to') {
            position = '100%';
            colorIndex = 0;
        }else if (args[j].name === 'from') {
            position = '0%';
            colorIndex = 0;
        };
        if (position.indexOf('%') === -1) { 
            position = (parseFloat(position) * 100) +'%';
        };
        color = args[j].args[colorIndex].name;
        if (args[j].args[colorIndex].args) { 
            color += '(' + this.colorValue(args[j].args[colorIndex].args) + ')';
        };
        if (args[j].name === 'from'){
            stops.unshift(color + ' ' + position);
        }else if(args[j].name === 'to'){
            toColor = color;
        }else{
            stops.push(color + ' ' + position);
        }
    }

    
    for(var j = 0; j < stops.length; j++){
        stdArgStr += ', ' + stops[j];
    }
    if(toColor){
        stdArgStr += ', ' + toColor + ' 100%';
    }
    return stdArgStr;
  },

  colorValue: function(obj){
    var ar = [];
    for (var i = 0; i < obj.length; i++) {
        ar.push(obj[i].name);
    };
    return ar.join(', ');
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CSSUnprefixingService]);
