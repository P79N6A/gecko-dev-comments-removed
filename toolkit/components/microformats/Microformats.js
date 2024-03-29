



this.EXPORTED_SYMBOLS = ["Microformats", "adr", "tag", "hCard", "hCalendar", "geo"];

this.Microformats = {
  
  list: [],
  
  
  __iterator__: function () {
    for (let i=0; i < this.list.length; i++) {
      yield this.list[i];
    }
  },
  
















  get: function(name, rootElement, options, targetArray) {
    function isAncestor(haystack, needle) {
      var parent = needle;
      while (parent = parent.parentNode) {
        
        
        if (parent == needle.parentNode) {
          return true;
        }
      }
      return false;
    }
    if (!Microformats[name] || !rootElement) {
      return;
    }
    targetArray = targetArray || [];

    
    
    var defaultView = rootElement.defaultView || rootElement.ownerDocument.defaultView;
    var rootDocument = rootElement.ownerDocument || rootElement;

    
    if (!options || !options.hasOwnProperty("recurseExternalFrames") || options.recurseExternalFrames) {
      if (defaultView && defaultView.frames.length > 0) {
        for (let i=0; i < defaultView.frames.length; i++) {
          if (isAncestor(rootDocument, defaultView.frames[i].frameElement)) {
            Microformats.get(name, defaultView.frames[i].document, options, targetArray);
          }
        }
      }
    }

    
    var microformatNodes = [];
    if (Microformats[name].className) {
      microformatNodes = Microformats.getElementsByClassName(rootElement,
                                        Microformats[name].className);
      
      
      if ((microformatNodes.length == 0) && Microformats[name].alternateClassName) {
        var altClass = Microformats.getElementsByClassName(rootElement, Microformats[name].alternateClassName);
        if (altClass.length > 0) {
          microformatNodes.push(rootElement); 
        }
      }
    } else if (Microformats[name].attributeValues) {
      microformatNodes =
        Microformats.getElementsByAttribute(rootElement,
                                            Microformats[name].attributeName,
                                            Microformats[name].attributeValues);
      
    }
    

    function isVisible(node, checkChildren) {
      if (node.getBoundingClientRect) {
        var box = node.getBoundingClientRect();
      } else {
        var box = node.ownerDocument.getBoxObjectFor(node);
      }
      
      if ((box.height == 0) || (box.width == 0)) {
        if (checkChildren && node.childNodes.length > 0) {
          for(let i=0; i < node.childNodes.length; i++) {
            if (node.childNodes[i].nodeType == Components.interfaces.nsIDOMNode.ELEMENT_NODE) {
              
              
              if (isVisible(node.childNodes[i], false)) {
                return true;
              }
            }
          }
        }
        return false
      }
      return true;
    }
    
    
    
    for (let i = 0; i < microformatNodes.length; i++) {
      
      if (!options || !options.hasOwnProperty("showHidden") || !options.showHidden) {
        if (microformatNodes[i].ownerDocument) {
          if (!isVisible(microformatNodes[i], true)) {
            continue;
          }
        }
      }
      try {
        if (options && options.debug) {
          
          
          targetArray.push(new Microformats[name].mfObject(microformatNodes[i], false));
        } else {
          targetArray.push(new Microformats[name].mfObject(microformatNodes[i], true));
        }
      } catch (ex) {
        
        
      }
    }
    return targetArray;
  },
  














  count: function(name, rootElement, options) {
    var mfArray = Microformats.get(name, rootElement, options);
    if (mfArray) {
      return mfArray.length;
    }
    return 0;
  },
  






  isMicroformat: function(node) {
    for (let i in Microformats)
    {
      if (Microformats[i].className) {
        if (Microformats.matchClass(node, Microformats[i].className)) {
            return true;
        }
      } else {
        var attribute;
        if (attribute = node.getAttribute(Microformats[i].attributeName)) {
          var attributeList = Microformats[i].attributeValues.split(" ");
          for (let j=0; j < attributeList.length; j++) {
            if (attribute.match("(^|\\s)" + attributeList[j] + "(\\s|$)")) {
              return true;
            }
          }
        }
      }
    }
    return false;
  },
  








  getParent: function(node) {
    var xpathExpression;
    var xpathResult;

    xpathExpression = "ancestor::*[";
    for (let i=0; i < Microformats.list.length; i++) {
      var mfname = Microformats.list[i];
      if (i != 0) {
        xpathExpression += " or ";
      }
      if (Microformats[mfname].className) {
        xpathExpression += "contains(concat(' ', @class, ' '), ' " + Microformats[mfname].className + " ')";
      } else {
        var attributeList = Microformats[mfname].attributeValues.split(" ");
        for (let j=0; j < attributeList.length; j++) {
          if (j != 0) {
            xpathExpression += " or ";
          }
          xpathExpression += "contains(concat(' ', @" + Microformats[mfname].attributeName + ", ' '), ' " + attributeList[j] + " ')";
        }
      }
    }
    xpathExpression += "][1]";
    xpathResult = (node.ownerDocument || node).evaluate(xpathExpression, node, null,  Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE, null);
    if (xpathResult.singleNodeValue) {
      xpathResult.singleNodeValue.microformat = mfname;
      return xpathResult.singleNodeValue;
    }
    return null;
  },
  







  getNamesFromNode: function(node) {
    var microformatNames = [];
    var xpathExpression;
    var xpathResult;
    for (let i in Microformats)
    {
      if (Microformats[i]) {
        if (Microformats[i].className) {
          if (Microformats.matchClass(node, Microformats[i].className)) {
            microformatNames.push(i);
            continue;
          }
        } else if (Microformats[i].attributeValues) {
          var attribute;
          if (attribute = node.getAttribute(Microformats[i].attributeName)) {
            var attributeList = Microformats[i].attributeValues.split(" ");
            for (let j=0; j < attributeList.length; j++) {
              
              if (attribute.match("(^|\\s)" + attributeList[j] + "(\\s|$)")) {
                microformatNames.push(i);
                break;
              }
            }
          }
        }
      }
    }
    return microformatNames.join(" ");
  },
  





  debug: function debug(microformatObject) {
    function dumpObject(item, indent)
    {
      if (!indent) {
        indent = "";
      }
      var toreturn = "";
      var testArray = [];
      
      for (let i in item)
      {
        if (testArray[i]) {
          continue;
        }
        if (typeof item[i] == "object") {
          if ((i != "node") && (i != "resolvedNode")) {
            if (item[i] && item[i].semanticType) {
              toreturn += indent + item[i].semanticType + " [" + i + "] { \n";
            } else {
              toreturn += indent + "object " + i + " { \n";
            }
            toreturn += dumpObject(item[i], indent + "\t");
            toreturn += indent + "}\n";
          }
        } else if ((typeof item[i] != "function") && (i != "semanticType")) {
          if (item[i]) {
            toreturn += indent + i + "=" + item[i] + "\n";
          }
        }
      }
      if (!toreturn && item) {
        toreturn = item.toString();
      }
      return toreturn;
    }
    return dumpObject(microformatObject);
  },
  add: function add(microformat, microformatDefinition) {
    
    if (!Microformats[microformat]) {
      Microformats.list.push(microformat);
    }
    Microformats[microformat] = microformatDefinition;
    microformatDefinition.mfObject.prototype.debug =
      function(microformatObject) {
        return Microformats.debug(microformatObject)
      };
  },
  remove: function remove(microformat) {
    if (Microformats[microformat]) {
      var list = Microformats.list;
      var index = list.indexOf(microformat, 1);
      if (index != -1) {
        list.splice(index, 1);
      }
      delete Microformats[microformat];
    }
  },
  
  parser: {
    











    defaultGetter: function(propnode, parentnode, datatype) {
      function collapseWhitespace(instring) {
        
        outstring = instring.replace(/[\n\r\t]/gi, ' ');
        
        outstring = outstring.replace(/\s{2,}/gi, ' ');
        
        outstring = outstring.replace(/\s{2,}/gi, '');
        
        outstring = outstring.replace(/^\s+/, '');
        
        outstring = outstring.replace(/\s+$/, '');
        return outstring;
      }
      
      
      if (((((propnode.localName.toLowerCase() == "abbr") || (propnode.localName.toLowerCase() == "html:abbr")) && !propnode.namespaceURI) || 
         ((propnode.localName.toLowerCase() == "abbr") && (propnode.namespaceURI == "http://www.w3.org/1999/xhtml"))) && (propnode.hasAttribute("title"))) {
        return propnode.getAttribute("title");
      } else if ((propnode.nodeName.toLowerCase() == "img") && (propnode.hasAttribute("alt"))) {
        return propnode.getAttribute("alt");
      } else if ((propnode.nodeName.toLowerCase() == "area") && (propnode.hasAttribute("alt"))) {
        return propnode.getAttribute("alt");
      } else if ((propnode.nodeName.toLowerCase() == "textarea") ||
                 (propnode.nodeName.toLowerCase() == "select") ||
                 (propnode.nodeName.toLowerCase() == "input")) {
        return propnode.value;
      } else {
        var values = Microformats.getElementsByClassName(propnode, "value");
        
        for (let i = values.length-1; i >= 0; i--) {
          if (values[i].parentNode != propnode) {
            values.splice(i,1);
          }
        }
        if (values.length > 0) {
          var value = "";
          for (let j=0;j<values.length;j++) {
            value += Microformats.parser.defaultGetter(values[j], propnode, datatype);
          }
          return collapseWhitespace(value);
        }
        var s;
        if (datatype == "HTML") {
          s = propnode.innerHTML;
        } else {
          if (propnode.innerText) {
            s = propnode.innerText;
          } else {
            s = propnode.textContent;
          }
        }
        
        
        if (!Microformats.matchClass(propnode, "value")) {
          s = collapseWhitespace(s);
        }
        if (s.length > 0) {
          return s;
        }
      }
    },
    









    dateTimeGetter: function(propnode, parentnode) {
      var date = Microformats.parser.textGetter(propnode, parentnode);
      if (date) {
        return Microformats.parser.normalizeISO8601(date);
      }
    },
    









    uriGetter: function(propnode, parentnode) {
      var pairs = {"a":"href", "img":"src", "object":"data", "area":"href"};
      var name = propnode.nodeName.toLowerCase();
      if (pairs.hasOwnProperty(name)) {
        return propnode[pairs[name]];
      }
      return Microformats.parser.textGetter(propnode, parentnode);
    },
    











    telGetter: function(propnode, parentnode) {
      var pairs = {"a":"href", "object":"data", "area":"href"};
      var name = propnode.nodeName.toLowerCase();
      if (pairs.hasOwnProperty(name)) {
        var protocol;
        if (propnode[pairs[name]].indexOf("tel:") == 0) {
          protocol = "tel:";
        }
        if (propnode[pairs[name]].indexOf("fax:") == 0) {
          protocol = "fax:";
        }
        if (propnode[pairs[name]].indexOf("modem:") == 0) {
          protocol = "modem:";
        }
        if (protocol) {
          if (propnode[pairs[name]].indexOf('?') > 0) {
            return unescape(propnode[pairs[name]].substring(protocol.length, propnode[pairs[name]].indexOf('?')));
          } else {
            return unescape(propnode[pairs[name]].substring(protocol.length));
          }
        }
      }
     
      if (Microformats.matchClass(propnode, "value")) {
        return Microformats.parser.textGetter(parentnode, parentnode);
      } else {
        
        if (!parentnode && (Microformats.getElementsByClassName(propnode, "type").length > 0)) {
          var tempNode = propnode.cloneNode(true);
          var typeNodes = Microformats.getElementsByClassName(tempNode, "type");
          for (let i=0; i < typeNodes.length; i++) {
            typeNodes[i].parentNode.removeChild(typeNodes[i]);
          }
          return Microformats.parser.textGetter(tempNode);
        }
        return Microformats.parser.textGetter(propnode, parentnode);
      }
    },
    










    emailGetter: function(propnode, parentnode) {
      if ((propnode.nodeName.toLowerCase() == "a") || (propnode.nodeName.toLowerCase() == "area")) {
        var mailto = propnode.href;
        
        if (mailto.indexOf('?') > 0) {
          return unescape(mailto.substring("mailto:".length, mailto.indexOf('?')));
        } else {
          return unescape(mailto.substring("mailto:".length));
        }
      } else {
        
        
        
        if (Microformats.matchClass(propnode, "value")) {
          return Microformats.parser.textGetter(parentnode, parentnode);
        } else {
          
          if (!parentnode && (Microformats.getElementsByClassName(propnode, "type").length > 0)) {
            var tempNode = propnode.cloneNode(true);
            var typeNodes = Microformats.getElementsByClassName(tempNode, "type");
            for (let i=0; i < typeNodes.length; i++) {
              typeNodes[i].parentNode.removeChild(typeNodes[i]);
            }
            return Microformats.parser.textGetter(tempNode);
          }
          return Microformats.parser.textGetter(propnode, parentnode);
        }
      }
    },
    










    textGetter: function(propnode, parentnode) {
      return Microformats.parser.defaultGetter(propnode, parentnode, "text");
    },
    








    HTMLGetter: function(propnode, parentnode) {
      
      
      
      function mfHTML(value) {
        this.valueOf = function() {return value ? value.valueOf() : "";}
        this.toString = function() {return value ? value.toString() : "";}
      }
      mfHTML.prototype = new String;
      mfHTML.prototype.toHTML = function() {
        return Microformats.parser.defaultGetter(propnode, parentnode, "HTML");
      }
      return new mfHTML(Microformats.parser.defaultGetter(propnode, parentnode, "text"));
    },
    










    datatypeHelper: function(prop, node, parentnode) {
      var result;
      var datatype = prop.datatype;
      switch (datatype) {
        case "dateTime":
          result = Microformats.parser.dateTimeGetter(node, parentnode);
          break;
        case "anyURI":
          result = Microformats.parser.uriGetter(node, parentnode);
          break;
        case "email":
          result = Microformats.parser.emailGetter(node, parentnode);
          break;
        case "tel":
          result = Microformats.parser.telGetter(node, parentnode);
          break;
        case "HTML":
          result = Microformats.parser.HTMLGetter(node, parentnode);
          break;
        case "float":
          var asText = Microformats.parser.textGetter(node, parentnode);
          if (!isNaN(asText)) {
            result = parseFloat(asText);
          }
          break;
        case "custom":
          result = prop.customGetter(node, parentnode);
          break;
        case "microformat":
          try {
            result = new Microformats[prop.microformat].mfObject(node, true);
          } catch (ex) {
            
            
            
            
            
            
            
            if (ex != "Node is not a microformat (" + prop.microformat + ")") {
              break;
            }
          }
          if (result != undefined) {
            if (prop.microformat_property) {
              result = result[prop.microformat_property];
            }
            break;
          }
        default:
          result = Microformats.parser.textGetter(node, parentnode);
          break;
      }
      
      
      if (prop.values && (result != undefined)) {
        var validType = false;
        for (let value in prop.values) {
          if (result.toLowerCase() == prop.values[value]) {
            result = result.toLowerCase();
            validType = true;
            break;
          }
        }
        if (!validType) {
          return;
        }
      }
      return result;
    },
    newMicroformat: function(object, in_node, microformat, validate) {
      
      if (!Microformats[microformat]) {
        throw("Invalid microformat - " + microformat);
      }
      if (in_node.ownerDocument) {
        if (Microformats[microformat].attributeName) {
          if (!(in_node.hasAttribute(Microformats[microformat].attributeName))) {
            throw("Node is not a microformat (" + microformat + ")");
          }
        } else {
          if (!Microformats.matchClass(in_node, Microformats[microformat].className)) {
            throw("Node is not a microformat (" + microformat + ")");
          }
        }
      }
      var node = in_node;
      if ((Microformats[microformat].className) && in_node.ownerDocument) {
        node = Microformats.parser.preProcessMicroformat(in_node);
      }

      for (let i in Microformats[microformat].properties) {
        object.__defineGetter__(i, Microformats.parser.getMicroformatPropertyGenerator(node, microformat, i, object));
      }
      
      
      object.node = in_node;
      
      object.resolvedNode = node; 
      object.semanticType = microformat;
      if (validate) {
        Microformats.parser.validate(node, microformat);
      }
    },
    getMicroformatPropertyGenerator: function getMicroformatPropertyGenerator(node, name, property, microformat)
    {
      return function() {
        var result = Microformats.parser.getMicroformatProperty(node, name, property);


        return result;
      };
    },
    getPropertyInternal: function getPropertyInternal(propnode, parentnode, propobj, propname, mfnode) {
      var result;
      if (propobj.subproperties) {
        for (let subpropname in propobj.subproperties) {
          var subpropnodes;
          var subpropobj = propobj.subproperties[subpropname];
          if (subpropobj.rel == true) {
            subpropnodes = Microformats.getElementsByAttribute(propnode, "rel", subpropname);
          } else {
            subpropnodes = Microformats.getElementsByClassName(propnode, subpropname);
          }
          var resultArray = [];
          var subresult;
          for (let i = 0; i < subpropnodes.length; i++) {
            subresult = Microformats.parser.getPropertyInternal(subpropnodes[i], propnode,
                                                                subpropobj,
                                                                subpropname, mfnode);
            if (subresult != undefined) {
              resultArray.push(subresult);
              
              if (!subpropobj.plural) {
                break;
              }
            }
          }
          if (resultArray.length == 0) {
            subresult = Microformats.parser.getPropertyInternal(propnode, null,
                                                                subpropobj,
                                                                subpropname, mfnode);
            if (subresult != undefined) {
              resultArray.push(subresult);
            }
          }
          if (resultArray.length > 0) {
            result = result || {};
            if (subpropobj.plural) {
              result[subpropname] = resultArray;
            } else {
              result[subpropname] = resultArray[0];
            }
          }
        }
      }
      if (!parentnode || (!result && propobj.subproperties)) {
        if (propobj.virtual) {
          if (propobj.virtualGetter) {
            result = propobj.virtualGetter(mfnode || propnode);
          } else {
            result = Microformats.parser.datatypeHelper(propobj, propnode);
          }
        }
      } else if (!result) {
        result = Microformats.parser.datatypeHelper(propobj, propnode, parentnode);
      }
      return result;
    },
    getMicroformatProperty: function getMicroformatProperty(in_mfnode, mfname, propname) {
      var mfnode = in_mfnode;
      
      
      
      
      
      if (!in_mfnode.origNode && Microformats[mfname].className && in_mfnode.ownerDocument) {
        mfnode = Microformats.parser.preProcessMicroformat(in_mfnode);
      }
      
      var propobj;
      
      if (Microformats[mfname].properties[propname]) {
        propobj = Microformats[mfname].properties[propname];
      } else {
        
        return;
      }
      
      
      var propnodes;
      if (propobj.rel == true) {
        propnodes = Microformats.getElementsByAttribute(mfnode, "rel", propname);
      } else {
        propnodes = Microformats.getElementsByClassName(mfnode, propname);
      }
      for (let i=propnodes.length-1; i >= 0; i--) {
        
        
        
        var parentnode;
        var node = propnodes[i];
        var xpathExpression = "";
        for (let j=0; j < Microformats.list.length; j++) {
          
          if ((mfname == "hCard") && ((Microformats.list[j] == "adr") || (Microformats.list[j] == "geo"))) {
            continue;
          }
          if (Microformats[Microformats.list[j]].className) {
            if (xpathExpression.length == 0) {
              xpathExpression = "ancestor::*[";
            } else {
              xpathExpression += " or ";
            }
            xpathExpression += "contains(concat(' ', @class, ' '), ' " + Microformats[Microformats.list[j]].className + " ')";
          }
        }
        xpathExpression += "][1]";
        var xpathResult = (node.ownerDocument || node).evaluate(xpathExpression, node, null,  Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE, null);
        if (xpathResult.singleNodeValue) {
          xpathResult.singleNodeValue.microformat = mfname;
          parentnode = xpathResult.singleNodeValue;
        }
        
        
        
        if (parentnode != mfnode) {
          var mfNameString = Microformats.getNamesFromNode(parentnode);
          var mfNames = mfNameString.split(" ");
          var j;
          for (j=0; j < mfNames.length; j++) {
            
            if (Microformats[mfNames[j]].properties[propname]) {
              propnodes.splice(i,1);
              break;
            }
          }
        }
      }
      if (propnodes.length > 0) {
        var resultArray = [];
        for (let i = 0; i < propnodes.length; i++) {
          var subresult = Microformats.parser.getPropertyInternal(propnodes[i],
                                                                  mfnode,
                                                                  propobj,
                                                                  propname);
          if (subresult != undefined) {
            resultArray.push(subresult);
            
            if (!propobj.plural) {
              return resultArray[0];
            }
          }
        }
        if (resultArray.length > 0) {
          return resultArray;
        }
      } else {
        
        
        if (propobj.virtual) {
          return Microformats.parser.getPropertyInternal(mfnode, null,
                                                         propobj, propname);
        }
      }
      return;
    },
    










    preProcessMicroformat: function preProcessMicroformat(in_mfnode) {
      var mfnode;
      if ((in_mfnode.nodeName.toLowerCase() == "td") && (in_mfnode.hasAttribute("headers"))) {
        mfnode = in_mfnode.cloneNode(true);
        mfnode.origNode = in_mfnode;
        var headers = in_mfnode.getAttribute("headers").split(" ");
        for (let i = 0; i < headers.length; i++) {
          var tempNode = in_mfnode.ownerDocument.createElement("span");
          var headerNode = in_mfnode.ownerDocument.getElementById(headers[i]);
          if (headerNode) {
            tempNode.innerHTML = headerNode.innerHTML;
            tempNode.className = headerNode.className;
            mfnode.appendChild(tempNode);
          }
        }
      } else {
        mfnode = in_mfnode;
      }
      var includes = Microformats.getElementsByClassName(mfnode, "include");
      if (includes.length > 0) {
        
        if (!mfnode.origNode) {
          mfnode = in_mfnode.cloneNode(true);
          mfnode.origNode = in_mfnode;
        }
        includes = Microformats.getElementsByClassName(mfnode, "include");
        var includeId;
        var include_length = includes.length;
        for (let i = include_length -1; i >= 0; i--) {
          if (includes[i].nodeName.toLowerCase() == "a") {
            includeId = includes[i].getAttribute("href").substr(1);
          }
          if (includes[i].nodeName.toLowerCase() == "object") {
            includeId = includes[i].getAttribute("data").substr(1);
          }
          if (in_mfnode.ownerDocument.getElementById(includeId)) {
            includes[i].parentNode.replaceChild(in_mfnode.ownerDocument.getElementById(includeId).cloneNode(true), includes[i]);
          }
        }
      }
      return mfnode;
    },
    validate: function validate(mfnode, mfname) {
      var error = "";
      if (Microformats[mfname].validate) {
        return Microformats[mfname].validate(mfnode);
      } else if (Microformats[mfname].required) {
        for (let i=0;i<Microformats[mfname].required.length;i++) {
          if (!Microformats.parser.getMicroformatProperty(mfnode, mfname, Microformats[mfname].required[i])) {
            error += "Required property " + Microformats[mfname].required[i] + " not specified\n";
          }
        }
        if (error.length > 0) {
          throw(error);
        }
        return true;
      }
    },
    
    
    normalizeISO8601: function normalizeISO8601(string)
    {
      var dateArray = string.match(/(\d\d\d\d)(?:-?(\d\d)(?:-?(\d\d)(?:[T ](\d\d)(?::?(\d\d)(?::?(\d\d)(?:\.(\d+))?)?)?(?:([-+Z])(?:(\d\d)(?::?(\d\d))?)?)?)?)?)?/);
  
      var dateString;
      var tzOffset = 0;
      if (!dateArray) {
        return;
      }
      if (dateArray[1]) {
        dateString = dateArray[1];
        if (dateArray[2]) {
          dateString += "-" + dateArray[2];
          if (dateArray[3]) {
            dateString += "-" + dateArray[3];
            if (dateArray[4]) {
              dateString += "T" + dateArray[4];
              if (dateArray[5]) {
                dateString += ":" + dateArray[5];
              } else {
                dateString += ":" + "00";
              }
              if (dateArray[6]) {
                dateString += ":" + dateArray[6];
              } else {
                dateString += ":" + "00";
              }
              if (dateArray[7]) {
                dateString += "." + dateArray[7];
              }
              if (dateArray[8]) {
                dateString += dateArray[8];
                if ((dateArray[8] == "+") || (dateArray[8] == "-")) {
                  if (dateArray[9]) {
                    dateString += dateArray[9];
                    if (dateArray[10]) {
                      dateString += dateArray[10];
                    }
                  }
                }
              }
            }
          }
        }
      }
      return dateString;
    }
  },
  








  dateFromISO8601: function dateFromISO8601(string) {
    var dateArray = string.match(/(\d\d\d\d)(?:-?(\d\d)(?:-?(\d\d)(?:[T ](\d\d)(?::?(\d\d)(?::?(\d\d)(?:\.(\d+))?)?)?(?:([-+Z])(?:(\d\d)(?::?(\d\d))?)?)?)?)?)?/);
  
    var date = new Date(dateArray[1], 0, 1);
    date.time = false;

    if (dateArray[2]) {
      date.setMonth(dateArray[2] - 1);
    }
    if (dateArray[3]) {
      date.setDate(dateArray[3]);
    }
    if (dateArray[4]) {
      date.setHours(dateArray[4]);
      date.time = true;
      if (dateArray[5]) {
        date.setMinutes(dateArray[5]);
        if (dateArray[6]) {
          date.setSeconds(dateArray[6]);
          if (dateArray[7]) {
            date.setMilliseconds(Number("0." + dateArray[7]) * 1000);
          }
        }
      }
    }
    if (dateArray[8]) {
      if (dateArray[8] == "-") {
        if (dateArray[9] && dateArray[10]) {
          date.setHours(date.getHours() + parseInt(dateArray[9], 10));
          date.setMinutes(date.getMinutes() + parseInt(dateArray[10], 10));
        }
      } else if (dateArray[8] == "+") {
        if (dateArray[9] && dateArray[10]) {
          date.setHours(date.getHours() - parseInt(dateArray[9], 10));
          date.setMinutes(date.getMinutes() - parseInt(dateArray[10], 10));
        }
      }
      
      
      if (dateArray[8]) {
        var tzOffset = date.getTimezoneOffset();
        if (tzOffset < 0) {
          date.setMinutes(date.getMinutes() + tzOffset); 
        } else if (tzOffset > 0) {
          date.setMinutes(date.getMinutes() - tzOffset); 
        }
      }
    }
    return date;
  },
  








  iso8601FromDate: function iso8601FromDate(date, punctuation) {
    var string = date.getFullYear().toString();
    if (punctuation) {
      string += "-";
    }
    string += (date.getMonth() + 1).toString().replace(/\b(\d)\b/g, '0$1');
    if (punctuation) {
      string += "-";
    }
    string += date.getDate().toString().replace(/\b(\d)\b/g, '0$1');
    if (date.time) {
      string += "T";
      string += date.getHours().toString().replace(/\b(\d)\b/g, '0$1');
      if (punctuation) {
        string += ":";
      }
      string += date.getMinutes().toString().replace(/\b(\d)\b/g, '0$1');
      if (punctuation) {
        string += ":";
      }
      string += date.getSeconds().toString().replace(/\b(\d)\b/g, '0$1');
      if (date.getMilliseconds() > 0) {
        if (punctuation) {
          string += ".";
        }
        string += date.getMilliseconds().toString();
      }
    }
    return string;
  },
  simpleEscape: function simpleEscape(s)
  {
    s = s.replace(/\&/g, '%26');
    s = s.replace(/\#/g, '%23');
    s = s.replace(/\+/g, '%2B');
    s = s.replace(/\-/g, '%2D');
    s = s.replace(/\=/g, '%3D');
    s = s.replace(/\'/g, '%27');
    s = s.replace(/\,/g, '%2C');


    s = s.replace(/ /g, '+');
    return s;
  },
  










  getElementsByClassName: function getElementsByClassName(rootNode, className)
  {
    var returnElements = [];

    if ((rootNode.ownerDocument || rootNode).getElementsByClassName) {
    
      var col = rootNode.getElementsByClassName(className);
      for (let i = 0; i < col.length; i++) {
        returnElements[i] = col[i];
      }
    } else if ((rootNode.ownerDocument || rootNode).evaluate) {
    
      var xpathExpression;
      xpathExpression = ".//*[contains(concat(' ', @class, ' '), ' " + className + " ')]";
      var xpathResult = (rootNode.ownerDocument || rootNode).evaluate(xpathExpression, rootNode, null, 0, null);

      var node;
      while (node = xpathResult.iterateNext()) {
        returnElements.push(node);
      }
    } else {
    
      className = className.replace(/\-/g, "\\-");
      var elements = rootNode.getElementsByTagName("*");
      for (let i=0;i<elements.length;i++) {
        if (elements[i].className.match("(^|\\s)" + className + "(\\s|$)")) {
          returnElements.push(elements[i]);
        }
      }
    }
    return returnElements;
  },
  










  getElementsByAttribute: function getElementsByAttribute(rootNode, attributeName, attributeValues)
  {
    var attributeList = attributeValues.split(" ");

    var returnElements = [];

    if ((rootNode.ownerDocument || rootNode).evaluate) {
    
      
      var xpathExpression = ".//*[";
      for (let i = 0; i < attributeList.length; i++) {
        if (i != 0) {
          xpathExpression += " or ";
        }
        xpathExpression += "contains(concat(' ', @" + attributeName + ", ' '), ' " + attributeList[i] + " ')";
      }
      xpathExpression += "]"; 

      var xpathResult = (rootNode.ownerDocument || rootNode).evaluate(xpathExpression, rootNode, null, 0, null);

      var node;
      while (node = xpathResult.iterateNext()) {
        returnElements.push(node);
      }
    } else {
    
    }
    return returnElements;
  },
  matchClass: function matchClass(node, className) {
    var classValue = node.getAttribute("class");
    return (classValue && classValue.match("(^|\\s)" + className + "(\\s|$)"));
  }
};



this.adr = function adr(node, validate) {
  if (node) {
    Microformats.parser.newMicroformat(this, node, "adr", validate);
  }
}

adr.prototype.toString = function() {
  var address_text = "";
  var start_parens = false;
  if (this["street-address"]) {
    address_text += this["street-address"][0];
  } else if (this["extended-address"]) {
    address_text += this["extended-address"];
  }
  if (this["locality"]) {
    if (this["street-address"] || this["extended-address"]) {
      address_text += " (";
      start_parens = true;
    }
    address_text += this["locality"];
  }
  if (this["region"]) {
    if ((this["street-address"] || this["extended-address"]) && (!start_parens)) {
      address_text += " (";
      start_parens = true;
    } else if (this["locality"]) {
      address_text += ", ";
    }
    address_text += this["region"];
  }
  if (this["country-name"]) {
    if ((this["street-address"] || this["extended-address"]) && (!start_parens)) {
      address_text += " (";
      start_parens = true;
      address_text += this["country-name"];
    } else if ((!this["locality"]) && (!this["region"])) {
      address_text += this["country-name"];
    } else if (((!this["locality"]) && (this["region"])) || ((this["locality"]) && (!this["region"]))) {
      address_text += ", ";
      address_text += this["country-name"];
    }
  }
  if (start_parens) {
    address_text += ")";
  }
  return address_text;
}

var adr_definition = {
  mfObject: adr,
  className: "adr",
  properties: {
    "type" : {
      plural: true,
      values: ["work", "home", "pref", "postal", "dom", "intl", "parcel"]
    },
    "post-office-box" : {
    },
    "street-address" : {
      plural: true
    },
    "extended-address" : {
    },
    "locality" : {
    },
    "region" : {
    },
    "postal-code" : {
    },
    "country-name" : {
    }
  },
  validate: function(node) {
    var xpathExpression = "count(descendant::*[" +
                                              "contains(concat(' ', @class, ' '), ' post-office-box ')" +
                                              " or contains(concat(' ', @class, ' '), ' street-address ')" +
                                              " or contains(concat(' ', @class, ' '), ' extended-address ')" +
                                              " or contains(concat(' ', @class, ' '), ' locality ')" +
                                              " or contains(concat(' ', @class, ' '), ' region ')" +
                                              " or contains(concat(' ', @class, ' '), ' postal-code ')" +
                                              " or contains(concat(' ', @class, ' '), ' country-name')" +
                                              "])";
    var xpathResult = (node.ownerDocument || node).evaluate(xpathExpression, node, null,  Components.interfaces.nsIDOMXPathResult.ANY_TYPE, null).numberValue;
    if (xpathResult == 0) {
      throw("Unable to create microformat");
    }
    return true;
  }
};

Microformats.add("adr", adr_definition);

this.hCard = function hCard(node, validate) {
  if (node) {
    Microformats.parser.newMicroformat(this, node, "hCard", validate);
  }
}
hCard.prototype.toString = function() {
  if (this.resolvedNode) {
    
    
    
    var fns = Microformats.getElementsByClassName(this.node, "fn");
    if (fns.length === 0) {
      if (this.fn) {
        if (this.org && this.org[0]["organization-name"] && (this.fn != this.org[0]["organization-name"])) {
          return this.fn + " (" + this.org[0]["organization-name"] + ")";
        }
      }
    }
  }
  return this.fn;
}

var hCard_definition = {
  mfObject: hCard,
  className: "vcard",
  required: ["fn"],
  properties: {
    "adr" : {
      plural: true,
      datatype: "microformat",
      microformat: "adr"
    },
    "agent" : {
      plural: true,
      datatype: "microformat",
      microformat: "hCard"
    },
    "bday" : {
      datatype: "dateTime"
    },
    "class" : {
    },
    "category" : {
      plural: true,
      datatype: "microformat",
      microformat: "tag",
      microformat_property: "tag"
    },
    "email" : {
      subproperties: {
        "type" : {
          plural: true,
          values: ["internet", "x400", "pref"]
        },
        "value" : {
          datatype: "email",
          virtual: true
        }
      },
      plural: true   
    },
    "fn" : {
      required: true
    },
    "geo" : {
      datatype: "microformat",
      microformat: "geo"
    },
    "key" : {
      plural: true
    },
    "label" : {
      plural: true
    },
    "logo" : {
      plural: true,
      datatype: "anyURI"
    },
    "mailer" : {
      plural: true
    },
    "n" : {
      subproperties: {
        "honorific-prefix" : {
          plural: true
        },
        "given-name" : {
          plural: true
        },
        "additional-name" : {
          plural: true
        },
        "family-name" : {
          plural: true
        },
        "honorific-suffix" : {
          plural: true
        }
      },
      virtual: true,
      
      
      virtualGetter: function(mfnode) {
        var fn = Microformats.parser.getMicroformatProperty(mfnode, "hCard", "fn");
        var orgs = Microformats.parser.getMicroformatProperty(mfnode, "hCard", "org");
        var given_name = [];
        var family_name = [];
        if (fn && (!orgs || (orgs.length > 1) || (fn != orgs[0]["organization-name"]))) {
          var fns = fn.split(" ");
          if (fns.length === 2) {
            if (fns[0].charAt(fns[0].length-1) == ',') {
              given_name[0] = fns[1];
              family_name[0] = fns[0].substr(0, fns[0].length-1);
            } else if (fns[1].length == 1) {
              given_name[0] = fns[1];
              family_name[0] = fns[0];
            } else if ((fns[1].length == 2) && (fns[1].charAt(fns[1].length-1) == '.')) {
              given_name[0] = fns[1];
              family_name[0] = fns[0];
            } else {
              given_name[0] = fns[0];
              family_name[0] = fns[1];
            }
            return {"given-name" : given_name, "family-name" : family_name};
          }
        }
      }
    },
    "nickname" : {
      plural: true,
      virtual: true,
      
      
      virtualGetter: function(mfnode) {
        var fn = Microformats.parser.getMicroformatProperty(mfnode, "hCard", "fn");
        var orgs = Microformats.parser.getMicroformatProperty(mfnode, "hCard", "org");
        var given_name;
        var family_name;
        if (fn && (!orgs || (orgs.length) > 1 || (fn != orgs[0]["organization-name"]))) {
          var fns = fn.split(" ");
          if (fns.length === 1) {
            return [fns[0]];
          }
        }
        return;
      }
    },
    "note" : {
      plural: true,
      datatype: "HTML"
    },
    "org" : {
      subproperties: {
        "organization-name" : {
          virtual: true
        },
        "organization-unit" : {
          plural: true
        }
      },
      plural: true
    },
    "photo" : {
      plural: true,
      datatype: "anyURI"
    },
    "rev" : {
      datatype: "dateTime"
    },
    "role" : {
      plural: true
    },
    "sequence" : {
    },
    "sort-string" : {
    },
    "sound" : {
      plural: true
    },
    "title" : {
      plural: true
    },
    "tel" : {
      subproperties: {
        "type" : {
          plural: true,
          values: ["msg", "home", "work", "pref", "voice", "fax", "cell", "video", "pager", "bbs", "car", "isdn", "pcs"]
        },
        "value" : {
          datatype: "tel",
          virtual: true
        }
      },
      plural: true
    },
    "tz" : {
    },
    "uid" : {
      datatype: "anyURI"
    },
    "url" : {
      plural: true,
      datatype: "anyURI"
    }
  }
};

Microformats.add("hCard", hCard_definition);

this.hCalendar = function hCalendar(node, validate) {
  if (node) {
    Microformats.parser.newMicroformat(this, node, "hCalendar", validate);
  }
}
hCalendar.prototype.toString = function() {
  if (this.resolvedNode) {
    
    
    
    var summaries = Microformats.getElementsByClassName(this.node, "summary");
    if (summaries.length === 0) {
      if (this.summary) {
        if (this.dtstart) {
          return this.summary + " (" + Microformats.dateFromISO8601(this.dtstart).toLocaleString() + ")";
        }
      }
    }
  }
  if (this.dtstart) {
    return this.summary;
  }
  return;
}

var hCalendar_definition = {
  mfObject: hCalendar,
  className: "vevent",
  required: ["summary", "dtstart"],
  properties: {
    "category" : {
      plural: true,
      datatype: "microformat",
      microformat: "tag",
      microformat_property: "tag"
    },
    "class" : {
      values: ["public", "private", "confidential"]
    },
    "description" : {
      datatype: "HTML"
    },
    "dtstart" : {
      datatype: "dateTime"
    },
    "dtend" : {
      datatype: "dateTime"
    },
    "dtstamp" : {
      datatype: "dateTime"
    },
    "duration" : {
    },
    "geo" : {
      datatype: "microformat",
      microformat: "geo"
    },
    "location" : {
      datatype: "microformat",
      microformat: "hCard"
    },
    "status" : {
      values: ["tentative", "confirmed", "cancelled"]
    },
    "summary" : {},
    "transp" : {
      values: ["opaque", "transparent"]
    },
    "uid" : {
      datatype: "anyURI"
    },
    "url" : {
      datatype: "anyURI"
    },
    "last-modified" : {
      datatype: "dateTime"
    },
    "rrule" : {
      subproperties: {
        "interval" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "interval");
          }
        },
        "freq" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "freq");
          }
        },
        "bysecond" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "bysecond");
          }
        },
        "byminute" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "byminute");
          }
        },
        "byhour" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "byhour");
          }
        },
        "bymonthday" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "bymonthday");
          }
        },
        "byyearday" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "byyearday");
          }
        },
        "byweekno" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "byweekno");
          }
        },
        "bymonth" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "bymonth");
          }
        },
        "byday" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "byday");
          }
        },
        "until" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "until");
          }
        },
        "count" : {
          virtual: true,
          
          virtualGetter: function(mfnode) {
            return Microformats.hCalendar.properties.rrule.retrieve(mfnode, "count");
          }
        }
      },
      retrieve: function(mfnode, property) {
        var value = Microformats.parser.textGetter(mfnode);
        var rrule;
        rrule = value.split(';');
        for (let i=0; i < rrule.length; i++) {
          if (rrule[i].match(property)) {
            return rrule[i].split('=')[1];
          }
        }
      }
    }
  }
};

Microformats.add("hCalendar", hCalendar_definition);

this.geo = function geo(node, validate) {
  if (node) {
    Microformats.parser.newMicroformat(this, node, "geo", validate);
  }
}
geo.prototype.toString = function() {
  if (this.latitude != undefined) {
    if (!isFinite(this.latitude) || (this.latitude > 360) || (this.latitude < -360)) {
      return;
    }
  }
  if (this.longitude != undefined) {
    if (!isFinite(this.longitude) || (this.longitude > 360) || (this.longitude < -360)) {
      return;
    }
  }

  if ((this.latitude != undefined) && (this.longitude != undefined)) {
    var s;
    if ((this.node.localName.toLowerCase() == "abbr") || (this.node.localName.toLowerCase() == "html:abbr")) {
      s = this.node.textContent;
    }

    if (s) {
      return s;
    }

    
    var xpathExpression = "ancestor::*[contains(concat(' ', @class, ' '), ' vcard ')]";
    var xpathResult = this.node.ownerDocument.evaluate(xpathExpression, this.node, null,  Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE, null);
    if (xpathResult.singleNodeValue) {
      var hcard = new hCard(xpathResult.singleNodeValue);
      if (hcard.fn) {
        return hcard.fn;
      }
    }
    
    xpathExpression = "ancestor::*[contains(concat(' ', @class, ' '), ' vevent ')]";
    xpathResult = this.node.ownerDocument.evaluate(xpathExpression, this.node, null,  Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE, xpathResult);
    if (xpathResult.singleNodeValue) {
      var hcal = new hCalendar(xpathResult.singleNodeValue);
      if (hcal.summary) {
        return hcal.summary;
      }
    }
    if (s) {
      return s;
    } else {
      return this.latitude + ", " + this.longitude;
    }
  }
}

var geo_definition = {
  mfObject: geo,
  className: "geo",
  required: ["latitude","longitude"],
  properties: {
    "latitude" : {
      datatype: "float",
      virtual: true,
      
      virtualGetter: function(mfnode) {
        var value = Microformats.parser.textGetter(mfnode);
        var latlong;
        if (value.match(';')) {
          latlong = value.split(';');
          if (latlong[0]) {
            if (!isNaN(latlong[0])) {
              return parseFloat(latlong[0]);
            }
          }
        }
      }
    },
    "longitude" : {
      datatype: "float",
      virtual: true,
      
      virtualGetter: function(mfnode) {
        var value = Microformats.parser.textGetter(mfnode);
        var latlong;
        if (value.match(';')) {
          latlong = value.split(';');
          if (latlong[1]) {
            if (!isNaN(latlong[1])) {
              return parseFloat(latlong[1]);
            }
          }
        }
      }
    }
  },
  validate: function(node) {
    var latitude = Microformats.parser.getMicroformatProperty(node, "geo", "latitude");
    var longitude = Microformats.parser.getMicroformatProperty(node, "geo", "longitude");
    if (latitude != undefined) {
      if (!isFinite(latitude) || (latitude > 360) || (latitude < -360)) {
        throw("Invalid latitude");
      }
    } else {
      throw("No latitude specified");
    }
    if (longitude != undefined) {
      if (!isFinite(longitude) || (longitude > 360) || (longitude < -360)) {
        throw("Invalid longitude");
      }
    } else {
      throw("No longitude specified");
    }
    return true;
  }
};

Microformats.add("geo", geo_definition);

this.tag = function tag(node, validate) {
  if (node) {
    Microformats.parser.newMicroformat(this, node, "tag", validate);
  }
}
tag.prototype.toString = function() {
  return this.tag;
}

var tag_definition = {
  mfObject: tag,
  attributeName: "rel",
  attributeValues: "tag",
  properties: {
    "tag" : {
      virtual: true,
      virtualGetter: function(mfnode) {
        if (mfnode.href) {
          var ioService = Components.classes["@mozilla.org/network/io-service;1"].
                                     getService(Components.interfaces.nsIIOService);
          var uri = ioService.newURI(mfnode.href, null, null);
          var url_array = uri.path.split("/");
          for(let i=url_array.length-1; i > 0; i--) {
            if (url_array[i] !== "") {
              var tag
              if (tag = Microformats.tag.validTagName(url_array[i].replace(/\+/g, ' '))) {
                try {
                  return decodeURIComponent(tag);
                } catch (ex) {
                  return unescape(tag);
                }
              }
            }
          }
        }
        return null;
      }
    },
    "link" : {
      virtual: true,
      datatype: "anyURI"
    },
    "text" : {
      virtual: true
    }
  },
  validTagName: function(tag)
  {
    var returnTag = tag;
    if (tag.indexOf('?') != -1) {
      if (tag.indexOf('?') === 0) {
        return false;
      } else {
        returnTag = tag.substr(0, tag.indexOf('?'));
      }
    }
    if (tag.indexOf('#') != -1) {
      if (tag.indexOf('#') === 0) {
        return false;
      } else {
        returnTag = tag.substr(0, tag.indexOf('#'));
      }
    }
    if (tag.indexOf('.html') != -1) {
      if (tag.indexOf('.html') == tag.length - 5) {
        return false;
      }
    }
    return returnTag;
  },
  validate: function(node) {
    var tag = Microformats.parser.getMicroformatProperty(node, "tag", "tag");
    if (!tag) {
      if (node.href) {
        var url_array = node.getAttribute("href").split("/");
        for(let i=url_array.length-1; i > 0; i--) {
          if (url_array[i] !== "") {
            throw("Invalid tag name (" + url_array[i] + ")");
          }
        }
      } else {
        throw("No href specified on tag");
      }
    }
    return true;
  }
};

Microformats.add("tag", tag_definition);
