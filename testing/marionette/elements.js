



let {utils: Cu} = Components;

Cu.import("chrome://marionette/content/error.js");









this.EXPORTED_SYMBOLS = [
  "Accessibility",
  "ElementManager",
  "CLASS_NAME",
  "SELECTOR",
  "ID",
  "NAME",
  "LINK_TEXT",
  "PARTIAL_LINK_TEXT",
  "TAG",
  "XPATH",
  "ANON",
  "ANON_ATTRIBUTE"
];

const DOCUMENT_POSITION_DISCONNECTED = 1;

const uuidGen = Components.classes["@mozilla.org/uuid-generator;1"]
    .getService(Components.interfaces.nsIUUIDGenerator);

this.CLASS_NAME = "class name";
this.SELECTOR = "css selector";
this.ID = "id";
this.NAME = "name";
this.LINK_TEXT = "link text";
this.PARTIAL_LINK_TEXT = "partial link text";
this.TAG = "tag name";
this.XPATH = "xpath";
this.ANON= "anon";
this.ANON_ATTRIBUTE = "anon attribute";

this.Accessibility = function Accessibility() {
  
  
  this.strict = false;
  
  
  Object.defineProperty(this, 'accessibleRetrieval', {
    configurable: true,
    get: function() {
      delete this.accessibleRetrieval;
      this.accessibleRetrieval = Components.classes[
        '@mozilla.org/accessibleRetrieval;1'].getService(
          Components.interfaces.nsIAccessibleRetrieval);
      return this.accessibleRetrieval;
    }
  });
};

Accessibility.prototype = {
  



  actionableRoles: new Set([
    'pushbutton',
    'checkbutton',
    'combobox',
    'key',
    'link',
    'menuitem',
    'check menu item',
    'radio menu item',
    'option',
    'radiobutton',
    'slider',
    'spinbutton',
    'pagetab',
    'entry',
    'outlineitem'
  ]),

  






  getAccessibleObject(element, mustHaveAccessible = false) {
    let acc = this.accessibleRetrieval.getAccessibleFor(element);
    if (!acc && mustHaveAccessible) {
      this.handleErrorMessage('Element does not have an accessible object');
    }
    return acc;
  },

  




  isActionableRole(accessible) {
    return this.actionableRoles.has(
      this.accessibleRetrieval.getStringRole(accessible.role));
  },

  




  hasActionCount(accessible) {
    return accessible.actionCount > 0;
  },

  




  hasValidName(accessible) {
    return accessible.name && accessible.name.trim();
  },

  





  hasHiddenAttribute(accessible) {
    let hidden;
    try {
      hidden = accessible.attributes.getStringProperty('hidden');
    } finally {
      
      return hidden && hidden === 'true';
    }
  },

  





  matchState(accessible, stateName) {
    let stateToMatch = Components.interfaces.nsIAccessibleStates[stateName];
    let state = {};
    accessible.getState(state, {});
    return !!(state.value & stateToMatch);
  },

  




  isHidden(accessible) {
    while (accessible) {
      if (this.hasHiddenAttribute(accessible)) {
        return true;
      }
      accessible = accessible.parent;
    }
    return false;
  },

  



  handleErrorMessage(message) {
    if (!message) {
      return;
    }
    if (this.strict) {
      throw new ElementNotAccessibleError(message);
    }
    dump(Date.now() + " Marionette: " + message);
  }
};

this.ElementManager = function ElementManager(notSupported) {
  this.seenItems = {};
  this.timer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
  this.elementKey = 'ELEMENT';
  this.w3cElementKey = 'element-6066-11e4-a52e-4f735466cecf';
  this.elementStrategies = [CLASS_NAME, SELECTOR, ID, NAME, LINK_TEXT, PARTIAL_LINK_TEXT, TAG, XPATH, ANON, ANON_ATTRIBUTE];
  for (let i = 0; i < notSupported.length; i++) {
    this.elementStrategies.splice(this.elementStrategies.indexOf(notSupported[i]), 1);
  }
}

ElementManager.prototype = {
  


  reset: function EM_clear() {
    this.seenItems = {};
  },

  








  addToKnownElements: function EM_addToKnownElements(element) {
    for (let i in this.seenItems) {
      let foundEl = null;
      try {
        foundEl = this.seenItems[i].get();
      } catch (e) {}
      if (foundEl) {
        if (XPCNativeWrapper(foundEl) == XPCNativeWrapper(element)) {
          return i;
        }
      } else {
        
        delete this.seenItems[i];
      }
    }
    let id = uuidGen.generateUUID().toString();
    this.seenItems[id] = Components.utils.getWeakReference(element);
    return id;
  },

  










  getKnownElement: function EM_getKnownElement(id, win) {
    let el = this.seenItems[id];
    if (!el) {
      throw new JavaScriptError("Element has not been seen before. Id given was " + id);
    }
    try {
      el = el.get();
    }
    catch(e) {
      el = null;
      delete this.seenItems[id];
    }
    
    let wrappedWin = XPCNativeWrapper(win);
    if (!el ||
        !(XPCNativeWrapper(el).ownerDocument == wrappedWin.document) ||
        (XPCNativeWrapper(el).compareDocumentPosition(wrappedWin.document.documentElement) &
         DOCUMENT_POSITION_DISCONNECTED)) {
      throw new StaleElementReferenceError(
          "The element reference is stale. Either the element " +
          "is no longer attached to the DOM or the page has been refreshed.");
    }
    return el;
  },

  














  wrapValue: function EM_wrapValue(val) {
    let result = null;

    switch (typeof(val)) {
      case "undefined":
        result = null;
        break;

      case "string":
      case "number":
      case "boolean":
        result = val;
        break;

      case "object":
        let type = Object.prototype.toString.call(val);
        if (type == "[object Array]" ||
            type == "[object NodeList]") {
          result = [];
          for (let i = 0; i < val.length; ++i) {
            result.push(this.wrapValue(val[i]));

          }
        }
        else if (val == null) {
          result = null;
        }
        else if (val.nodeType == 1) {
          let elementId = this.addToKnownElements(val);
          result = {'ELEMENT': elementId, 'element-6066-11e4-a52e-4f735466cecf': elementId};
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
        else if (((typeof(args[this.elementKey]) === 'string') && args.hasOwnProperty(this.elementKey)) ||
                 ((typeof(args[this.w3cElementKey]) === 'string') &&
                     args.hasOwnProperty(this.w3cElementKey))) {
          let elementUniqueIdentifier = args[this.w3cElementKey] ? args[this.w3cElementKey] : args[this.elementKey];
          converted = this.getKnownElement(elementUniqueIdentifier,  win);
          if (converted == null) {
            throw new WebDriverError(`Unknown element: ${elementUniqueIdentifier}`);
          }
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
      if (arg && typeof(arg['__marionetteArgs']) === 'object') {
        for (let prop in arg['__marionetteArgs']) {
          namedArgs[prop] = arg['__marionetteArgs'][prop];
        }
      }
    });
    return namedArgs;
  },

  

























  find: function EM_find(win, values, searchTimeout, all, on_success, on_error, command_id) {
    let startTime = values.time ? values.time : new Date().getTime();
    let startNode = (values.element != undefined) ?
                    this.getKnownElement(values.element, win) : win.document;
    if (this.elementStrategies.indexOf(values.using) < 0) {
      throw new InvalidSelectorError(`No such strategy: ${values.using}`);
    }
    let found = all ? this.findElements(values.using, values.value, win.document, startNode) :
                      this.findElement(values.using, values.value, win.document, startNode);
    let type = Object.prototype.toString.call(found);
    let isArrayLike = ((type == '[object Array]') || (type == '[object HTMLCollection]') || (type == '[object NodeList]'));
    if (found == null || (isArrayLike && found.length <= 0)) {
      if (!searchTimeout || new Date().getTime() - startTime > searchTimeout) {
        if (all) {
          on_success([], command_id); 
        } else {
          
          let message = "Unable to locate element: " + values.value;
          if (values.using == ANON) {
            message = "Unable to locate anonymous children";
          } else if (values.using == ANON_ATTRIBUTE) {
            message = "Unable to locate anonymous element: " + JSON.stringify(values.value);
          }
          on_error(new NoSuchElementError(message), command_id);
        }
      } else {
        values.time = startTime;
        this.timer.initWithCallback(this.find.bind(this, win, values,
                                                   searchTimeout, all,
                                                   on_success, on_error,
                                                   command_id),
                                    100,
                                    Components.interfaces.nsITimer.TYPE_ONE_SHOT);
      }
    } else {
      if (isArrayLike) {
        let ids = []
        for (let i = 0 ; i < found.length ; i++) {
          ids.push({"ELEMENT": this.addToKnownElements(found[i])});
        }
        on_success(ids, command_id);
      } else {
        let id = this.addToKnownElements(found);
        on_success({"ELEMENT": id}, command_id);
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
      case ANON:
        element = rootNode.getAnonymousNodes(startNode);
        if (element != null) {
          element = element[0];
        }
        break;
      case ANON_ATTRIBUTE:
        let attr = Object.keys(value)[0];
        element = rootNode.getAnonymousElementByAttribute(startNode, attr, value[attr]);
        break;
      default:
        throw new WebDriverError("No such strategy");
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
      case ANON:
        elements = rootNode.getAnonymousNodes(startNode) || [];
        break;
      case ANON_ATTRIBUTE:
        let attr = Object.keys(value)[0];
        let el = rootNode.getAnonymousElementByAttribute(startNode, attr, value[attr]);
        if (el != null) {
          elements = [el];
        }
        break;
      default:
        throw new WebDriverError("No such strategy");
    }
    return elements;
  },
}
