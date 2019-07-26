












this.EXPORTED_SYMBOLS = [
  "ElementManager",
  "CLASS_NAME",
  "SELECTOR",
  "ID",
  "NAME",
  "LINK_TEXT",
  "PARTIAL_LINK_TEXT",
  "TAG",
  "XPATH"
];

let uuidGen = Components.classes["@mozilla.org/uuid-generator;1"]
             .getService(Components.interfaces.nsIUUIDGenerator);

this.CLASS_NAME = "class name";
this.SELECTOR = "css selector";
this.ID = "id";
this.NAME = "name";
this.LINK_TEXT = "link text";
this.PARTIAL_LINK_TEXT = "partial link text";
this.TAG = "tag name";
this.XPATH = "xpath";

function ElementException(msg, num, stack) {
  this.message = msg;
  this.code = num;
  this.stack = stack;
}


this.ElementManager = function ElementManager(notSupported) {
  this.searchTimeout = 0;
  this.seenItems = {};
  this.timer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
  this.elementStrategies = [CLASS_NAME, SELECTOR, ID, NAME, LINK_TEXT, PARTIAL_LINK_TEXT, TAG, XPATH];
  for (let i = 0; i < notSupported.length; i++) {
    this.elementStrategies.splice(this.elementStrategies.indexOf(notSupported[i]), 1);
  }
}

ElementManager.prototype = {
  


  reset: function EM_clear() {
    this.searchTimeout = 0;
    this.seenItems = {};
  },

  








  addToKnownElements: function EM_addToKnownElements(element) {
    for (let i in this.seenItems) {
      if (XPCNativeWrapper(this.seenItems[i]) == XPCNativeWrapper(element)) {
        return i;
      }
    }
    var id = uuidGen.generateUUID().toString();
    this.seenItems[id] = element;
    return id;
  },
  
  










  getKnownElement: function EM_getKnownElement(id, win) {
    let el = this.seenItems[id];
    if (!el) {
      throw new ElementException("Element has not been seen before", 17, null);
    }
    el = el;
    if (!(el.ownerDocument.isEqualNode(win.document))) {
      throw new ElementException("Stale element reference", 10, null);
    }
    return el;
  },
  
  









  wrapValue: function EM_wrapValue(val) {
    let result;
    switch(typeof(val)) {
      case "undefined":
        result = null;
        break;
      case "string":
      case "number":
      case "boolean":
        result = val;
        break;
      case "object":
        if (Object.prototype.toString.call(val) == '[object Array]') {
          result = [];
          for (let i in val) {
            result.push(this.wrapValue(val[i]));
          }
        }
        else if (val == null) {
          result = null;
        }
        else if (val.nodeType == 1) {
          for(let i in this.seenItems) {
            if (this.seenItems[i] == val) {
              result = {'ELEMENT': i};
            }
          }
          result = {'ELEMENT': this.addToKnownElements(val)};
        }
        else {
          result = {};
          for (let prop in val) {
            result[prop] = this.wrapValue(val[prop]);
          }
        }
        break;
    }
    return result;
  },
  
  











  convertWrappedArguments: function EM_convertWrappedArguments(args, win) {
    let converted;
    switch (typeof(args)) {
      case 'number':
      case 'string':
      case 'boolean':
        converted = args;
        break;
      case 'object':
        if (args == null) {
          converted = null;
        }
        else if (Object.prototype.toString.call(args) == '[object Array]') {
          converted = [];
          for (let i in args) {
            converted.push(this.convertWrappedArguments(args[i], win));
          }
        }
        else if (typeof(args['ELEMENT'] === 'string') &&
                 args.hasOwnProperty('ELEMENT')) {
          converted = this.getKnownElement(args['ELEMENT'],  win);
          if (converted == null)
            throw new ElementException("Unknown element: " + args['ELEMENT'], 500, null);
        }
        else {
          converted = {};
          for (let prop in args) {
            converted[prop] = this.convertWrappedArguments(args[prop], win);
          }
        }
        break;
    }
    return converted;
  },
  
  


  
  












  applyNamedArgs: function EM_applyNamedArgs(args) {
    namedArgs = {};
    args.forEach(function(arg) {
      if (typeof(arg['__marionetteArgs']) === 'object') {
        for (let prop in arg['__marionetteArgs']) {
          namedArgs[prop] = arg['__marionetteArgs'][prop];
        }
      }
    });
    return namedArgs;
  },
  
  

























  find: function EM_find(win, values, on_success, on_error, all, command_id) {
    let startTime = values.time ? values.time : new Date().getTime();
    let startNode = (values.element != undefined) ? this.getKnownElement(values.element, win) : win.document;
    if (this.elementStrategies.indexOf(values.using) < 0) {
      throw new ElementException("No such strategy.", 17, null);
    }
    let found = all ? this.findElements(values.using, values.value, win.document, startNode) : this.findElement(values.using, values.value, win.document, startNode);
    if (found) {
      let type = Object.prototype.toString.call(found);
      if ((type == '[object Array]') || (type == '[object HTMLCollection]') || (type == '[object NodeList]')) {
        let ids = []
        for (let i = 0 ; i < found.length ; i++) {
          ids.push(this.addToKnownElements(found[i]));
        }
        on_success(ids, command_id);
      }
      else {
        let id = this.addToKnownElements(found);
        on_success(id, command_id);
      }
      return;
    } else {
      if (this.searchTimeout == 0 || new Date().getTime() - startTime > this.searchTimeout) {
        on_error("Unable to locate element: " + values.value, 7, null, command_id);
      } else {
        values.time = startTime;
        this.timer.initWithCallback(this.find.bind(this, win, values,
                                                   on_success, on_error, all,
                                                   command_id),
                                    100,
                                    Components.interfaces.nsITimer.TYPE_ONE_SHOT);
      }
    }
  },

  












  findByXPath: function EM_findByXPath(root, value, node) {
    return root.evaluate(value, node, null,
            Components.interfaces.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;
  },

  












  findByXPathAll: function EM_findByXPathAll(root, value, node) {
    let values = root.evaluate(value, node, null,
                      Components.interfaces.nsIDOMXPathResult.ORDERED_NODE_ITERATOR_TYPE, null);
    let elements = [];
    let element = values.iterateNext();
    while (element) {
      elements.push(element);
      element = values.iterateNext();
    }
    return elements;
  },
  
  














  findElement: function EM_findElement(using, value, rootNode, startNode) {
    let element;
    switch (using) {
      case ID:
        element = startNode.getElementById ?
                  startNode.getElementById(value) : 
                  this.findByXPath(rootNode, './/*[@id="' + value + '"]', startNode);
        break;
      case NAME:
        element = startNode.getElementsByName ?
                  startNode.getElementsByName(value)[0] : 
                  this.findByXPath(rootNode, './/*[@name="' + value + '"]', startNode);
        break;
      case CLASS_NAME:
        element = startNode.getElementsByClassName(value)[0]; 
        break;
      case TAG:
        element = startNode.getElementsByTagName(value)[0]; 
        break;
      case XPATH:
        element = this.findByXPath(rootNode, value, startNode);
        break;
      case LINK_TEXT:
      case PARTIAL_LINK_TEXT:
        let allLinks = startNode.getElementsByTagName('A');
        for (let i = 0; i < allLinks.length && !element; i++) {
          let text = allLinks[i].text;
          if (PARTIAL_LINK_TEXT == using) {
            if (text.indexOf(value) != -1) {
              element = allLinks[i];
            }
          } else if (text == value) {
            element = allLinks[i];
          }
        }
        break;
      case SELECTOR:
        element = startNode.querySelector(value);
        break;
      default:
        throw new ElementException("No such strategy", 500, null);
    }
    return element;
  },

  














  findElements: function EM_findElements(using, value, rootNode, startNode) {
    let elements = [];
    switch (using) {
      case ID:
        value = './/*[@id="' + value + '"]';
      case XPATH:
        elements = this.findByXPathAll(rootNode, value, startNode);
        break;
      case NAME:
        elements = startNode.getElementsByName ?
                   startNode.getElementsByName(value) : 
                   this.findByXPathAll(rootNode, './/*[@name="' + value + '"]', startNode);
        break;
      case CLASS_NAME:
        elements = startNode.getElementsByClassName(value);
        break;
      case TAG:
        elements = startNode.getElementsByTagName(value);
        break;
      case LINK_TEXT:
      case PARTIAL_LINK_TEXT:
        let allLinks = rootNode.getElementsByTagName('A');
        for (let i = 0; i < allLinks.length; i++) {
          let text = allLinks[i].text;
          if (PARTIAL_LINK_TEXT == using) {
            if (text.indexOf(value) != -1) {
              elements.push(allLinks[i]);
            }
          } else if (text == value) {
            elements.push(allLinks[i]);
          }
        }
        break;
      case SELECTOR:
        elements = Array.slice(startNode.querySelectorAll(value));
        break;
      default:
        throw new ElementException("No such strategy", 500, null);
    }
    return elements;
  },

  





  setSearchTimeout: function EM_setSearchTimeout(value) {
    this.searchTimeout = parseInt(value);
    if(isNaN(this.searchTimeout)){
      throw new ElementException("Not a Number", 500, null);
    }
  },
}
