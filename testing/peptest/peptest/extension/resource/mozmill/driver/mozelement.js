





































var EXPORTED_SYMBOLS = ["Elem", "Selector", "ID", "Link", "XPath", "Name", "Lookup", 
                        "MozMillElement", "MozMillCheckBox", "MozMillRadio", "MozMillDropList",
                        "MozMillTextBox", "subclasses",
                       ];

var EventUtils = {};  Components.utils.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);
var utils = {};       Components.utils.import('resource://mozmill/stdlib/utils.js', utils);
var elementslib = {}; Components.utils.import('resource://mozmill/driver/elementslib.js', elementslib);
var broker = {};      Components.utils.import('resource://mozmill/driver/msgbroker.js', broker);


var subclasses = [MozMillCheckBox, MozMillRadio, MozMillDropList, MozMillTextBox];







function createInstance(locatorType, locator, elem) {
  if (elem) {
    var args = {"element":elem};
    for (var i = 0; i < subclasses.length; ++i) {
      if (subclasses[i].isType(elem)) {
        return new subclasses[i](locatorType, locator, args);
      }
    }
    if (MozMillElement.isType(elem)) return new MozMillElement(locatorType, locator, args);
  }
  throw new Error("could not find element " + locatorType + ": " + locator);
};

var Elem = function(node) {
  return createInstance("Elem", node, node);
};

var Selector = function(_document, selector, index) {
  return createInstance("Selector", selector, elementslib.Selector(_document, selector, index));
};

var ID = function(_document, nodeID) {
  return createInstance("ID", nodeID, elementslib.ID(_document, nodeID));
};

var Link = function(_document, linkName) {
  return createInstance("Link", linkName, elementslib.Link(_document, linkName));
};

var XPath = function(_document, expr) {
  return createInstance("XPath", expr, elementslib.XPath(_document, expr));
};

var Name = function(_document, nName) {
  return createInstance("Name", nName, elementslib.Name(_document, nName));
};

var Lookup = function(_document, expression) {
  return createInstance("Lookup", expression, elementslib.Lookup(_document, expression));
};






function MozMillElement(locatorType, locator, args) {
  args = args || {};
  this._locatorType = locatorType;
  this._locator = locator;
  this._element = args["element"];
  this._document = args["document"];
  this._owner = args["owner"];
  
  this.isElement = true;
}


MozMillElement.isType = function(node) {
  return true;
};


MozMillElement.prototype.__defineGetter__("element", function() {
  if (this._element == undefined) {
    if (elementslib[this._locatorType]) {
      this._element = elementslib[this._locatorType](this._document, this._locator); 
    } else if (this._locatorType == "Elem") {
      this._element = this._locator;
    } else {
      throw new Error("Unknown locator type: " + this._locatorType);
    }
  }
  return this._element;
});


MozMillElement.prototype.getNode = function() {
  return this.element;
};

MozMillElement.prototype.getInfo = function() {
  return this._locatorType + ": " + this._locator;
};





MozMillElement.prototype.exists = function() {
  this._element = undefined;
  if (this.element) return true;
  return false;
};

























MozMillElement.prototype.keypress = function(aKey, aModifiers, aExpectedEvent) {
  if (!this.element) {
    throw new Error("Could not find element " + this.getInfo());
  }

  var win = this.element.ownerDocument? this.element.ownerDocument.defaultView : this.element;
  this.element.focus();

  if (aExpectedEvent) {
    var target = aExpectedEvent.target? aExpectedEvent.target.getNode() : this.element;
    EventUtils.synthesizeKeyExpectEvent(aKey, aModifiers || {}, target, aExpectedEvent.type,
                                                            "MozMillElement.keypress()", win);
  } else {
    EventUtils.synthesizeKey(aKey, aModifiers || {}, win);
  }

  broker.pass({'function':'MozMillElement.keypress()'});
  return true;
};




































MozMillElement.prototype.mouseEvent = function(aOffsetX, aOffsetY, aEvent, aExpectedEvent) {
  if (!this.element) {
    throw new Error(arguments.callee.name + ": could not find element " + this.getInfo());
  }

  
  var rect = this.element.getBoundingClientRect();
  if (isNaN(aOffsetX)) {
    aOffsetX = rect.width / 2;
  }
  if (isNaN(aOffsetY)) {
    aOffsetY = rect.height / 2;
  }

  
  if (this.element.scrollIntoView) {
    this.element.scrollIntoView();
  }

  if (aExpectedEvent) {
    
    if (!aExpectedEvent.type)
      throw new Error(arguments.callee.name + ": Expected event type not specified");

    
    var target = aExpectedEvent.target ? aExpectedEvent.target.getNode() : this.element;
    if (!target) {
      throw new Error(arguments.callee.name + ": could not find element " + aExpectedEvent.target.getInfo());
    }

    EventUtils.synthesizeMouseExpectEvent(this.element, aOffsetX, aOffsetY, aEvent,
                                          target, aExpectedEvent.event,
                                          "MozMillElement.mouseEvent()",
                                          this.element.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(this.element, aOffsetX, aOffsetY, aEvent,
                               this.element.ownerDocument.defaultView);
  }
};




MozMillElement.prototype.click = function(left, top, expectedEvent) {
  
  if (this.element && this.element.tagName == "menuitem") {
    this.element.click();
  } else {
    this.mouseEvent(left, top, {}, expectedEvent);
  }

  broker.pass({'function':'MozMillElement.click()'});
};




MozMillElement.prototype.doubleClick = function(left, top, expectedEvent) {
  this.mouseEvent(left, top, {clickCount: 2}, expectedEvent);

  broker.pass({'function':'MozMillElement.doubleClick()'});
  return true;
};




MozMillElement.prototype.mouseDown = function (button, left, top, expectedEvent) {
  this.mouseEvent(left, top, {button: button, type: "mousedown"}, expectedEvent);

  broker.pass({'function':'MozMillElement.mouseDown()'});
  return true;
};




MozMillElement.prototype.mouseOut = function (button, left, top, expectedEvent) {
  this.mouseEvent(left, top, {button: button, type: "mouseout"}, expectedEvent);

  broker.pass({'function':'MozMillElement.mouseOut()'});
  return true;
};




MozMillElement.prototype.mouseOver = function (button, left, top, expectedEvent) {
  this.mouseEvent(left, top, {button: button, type: "mouseover"}, expectedEvent);

  broker.pass({'function':'MozMillElement.mouseOver()'});
  return true;
};




MozMillElement.prototype.mouseUp = function (button, left, top, expectedEvent) {
  this.mouseEvent(left, top, {button: button, type: "mouseup"}, expectedEvent);

  broker.pass({'function':'MozMillElement.mouseUp()'});
  return true;
};




MozMillElement.prototype.middleClick = function(left, top, expectedEvent) {
  this.mouseEvent(left, top, {button: 1}, expectedEvent);

  broker.pass({'function':'MozMillElement.middleClick()'});
  return true;
};




MozMillElement.prototype.rightClick = function(left, top, expectedEvent) {
  this.mouseEvent(left, top, {type : "contextmenu", button: 2 }, expectedEvent);

  broker.pass({'function':'MozMillElement.rightClick()'});
  return true;
};

MozMillElement.prototype.waitForElement = function(timeout, interval) {
  var elem = this;
  utils.waitFor(function() {
    return elem.exists();
  }, "Timeout exceeded for waitForElement " + this.getInfo(), timeout, interval);

  broker.pass({'function':'MozMillElement.waitForElement()'});
};

MozMillElement.prototype.waitForElementNotPresent = function(timeout, interval) {
  var elem = this;
  utils.waitFor(function() {
    return !elem.exists();
  }, "Timeout exceeded for waitForElementNotPresent " + this.getInfo(), timeout, interval);

  broker.pass({'function':'MozMillElement.waitForElementNotPresent()'});
};

MozMillElement.prototype.waitThenClick = function (timeout, interval, left, top, expectedEvent) {
  this.waitForElement(timeout, interval);
  this.click(left, top, expectedEvent);
};


MozMillElement.prototype.dispatchEvent = function (eventType, canBubble, modifiers) {
  canBubble = canBubble || true;
  var evt = this.element.ownerDocument.createEvent('HTMLEvents');
  evt.shiftKey = modifiers["shift"];
  evt.metaKey = modifiers["meta"];
  evt.altKey = modifiers["alt"];
  evt.ctrlKey = modifiers["ctrl"];
  evt.initEvent(eventType, canBubble, true);
  this.element.dispatchEvent(evt);
};









MozMillCheckBox.prototype = new MozMillElement();
MozMillCheckBox.prototype.parent = MozMillElement.prototype;
MozMillCheckBox.prototype.constructor = MozMillCheckBox;
function MozMillCheckBox(locatorType, locator, args) {
  this.parent.constructor.call(this, locatorType, locator, args);
}


MozMillCheckBox.isType = function(node) {
  if ((node.localName.toLowerCase() == "input" && node.getAttribute("type") == "checkbox") ||
      (node.localName.toLowerCase() == 'toolbarbutton' && node.getAttribute('type') == 'checkbox') ||
      (node.localName.toLowerCase() == 'checkbox')) {
    return true;
  }
  return false;
};




MozMillCheckBox.prototype.check = function(state) {
  var result = false;

  if (!this.element) {
    throw new Error("could not find element " + this.getInfo());
    return false;
  }

  
  if (this.element.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul") {
    this.element = utils.unwrapNode(this.element);
  }

  state = (typeof(state) == "boolean") ? state : false;
  if (state != this.element.checked) {
    this.click();
    var element = this.element;
    utils.waitFor(function() {
      return element.checked == state;
    }, "Checkbox " + this.getInfo() + " could not be checked/unchecked", 500);

    result = true;
  }

  broker.pass({'function':'MozMillCheckBox.check(' + this.getInfo() + ', state: ' + state + ')'});
  return result;
};








MozMillRadio.prototype = new MozMillElement();
MozMillRadio.prototype.parent = MozMillElement.prototype;
MozMillRadio.prototype.constructor = MozMillRadio;
function MozMillRadio(locatorType, locator, args) {
  this.parent.constructor.call(this, locatorType, locator, args);
}


MozMillRadio.isType = function(node) {
  if ((node.localName.toLowerCase() == 'input' && node.getAttribute('type') == 'radio') ||
      (node.localName.toLowerCase() == 'toolbarbutton' && node.getAttribute('type') == 'radio') ||
      (node.localName.toLowerCase() == 'radio') ||
      (node.localName.toLowerCase() == 'radiogroup')) {
    return true;
  }
  return false;
};







MozMillRadio.prototype.select = function(index) {
  if (!this.element) {
    throw new Error("could not find element " + this.getInfo());
  }
  
  if (this.element.localName.toLowerCase() == "radiogroup") {
    var element = this.element.getElementsByTagName("radio")[index || 0];
    new MozMillRadio("Elem", element).click();
  } else {
    var element = this.element;
    this.click();
  }
  
  utils.waitFor(function() {
    
    if (element.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul") {
      element = utils.unwrapNode(element);
      return element.selected == true;
    }
    return element.checked == true;
  }, "Radio button " + this.getInfo() + " could not be selected", 500);

  broker.pass({'function':'MozMillRadio.select(' + this.getInfo() + ')'});
  return true;
};








MozMillDropList.prototype = new MozMillElement();
MozMillDropList.prototype.parent = MozMillElement.prototype;
MozMillDropList.prototype.constructor = MozMillDropList;
function MozMillDropList(locatorType, locator, args) {
  this.parent.constructor.call(this, locatorType, locator, args);
};


MozMillDropList.isType = function(node) {
  if ((node.localName.toLowerCase() == 'toolbarbutton' && (node.getAttribute('type') == 'menu' || node.getAttribute('type') == 'menu-button')) ||
      (node.localName.toLowerCase() == 'menu') ||
      (node.localName.toLowerCase() == 'menulist') ||
      (node.localName.toLowerCase() == 'select' )) {
    return true;
  }
  return false;
};


MozMillDropList.prototype.select = function (indx, option, value) {
  if (!this.element){
    throw new Error("Could not find element " + this.getInfo());
  }

  
  if (this.element.localName.toLowerCase() == "select"){
    var item = null;

    
    if (indx != undefined) {
      
      if (indx == -1) {
        this.dispatchEvent('focus', false);
        this.element.selectedIndex = indx;
        this.dispatchEvent('change', true);

        broker.pass({'function':'MozMillDropList.select()'});
        return true;
      } else {
        item = this.element.options.item(indx);
      }
    } else {
      for (var i = 0; i < this.element.options.length; i++) {
        var entry = this.element.options.item(i);
        if (option != undefined && entry.innerHTML == option ||
            value != undefined && entry.value == value) {
          item = entry;
          break;
        }
      }
    }

    
    try {
      
      this.dispatchEvent('focus', false);
      item.selected = true;
      this.dispatchEvent('change', true);

      broker.pass({'function':'MozMillDropList.select()'});
      return true;
    } catch (ex) {
      throw new Error("No item selected for element " + this.getInfo());
      return false;
    }
  }
  
  else if (this.element.namespaceURI.toLowerCase() == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul") {
    var ownerDoc = this.element.ownerDocument;
    
    this.element = utils.unwrapNode(this.element);
    
    menuitems = this.element.getElementsByTagName("menupopup")[0].getElementsByTagName("menuitem");
    
    var item = null;

    if (indx != undefined) {
      if (indx == -1) {
        this.dispatchEvent('focus', false);
        this.element.boxObject.QueryInterface(Components.interfaces.nsIMenuBoxObject).activeChild = null;
        this.dispatchEvent('change', true);

        broker.pass({'function':'MozMillDropList.select()'});
        return true;
      } else {
        item = menuitems[indx];
      }
    } else {
      for (var i = 0; i < menuitems.length; i++) {
        var entry = menuitems[i];
        if (option != undefined && entry.label == option ||
            value != undefined && entry.value == value) {
          item = entry;
          break;
        }
      }
    }

    
    try {
      EventUtils.synthesizeMouse(this.element, 1, 1, {}, ownerDoc.defaultView);

      
      for (var i = 0; i <= menuitems.length; ++i) {
        var selected = this.element.boxObject.QueryInterface(Components.interfaces.nsIMenuBoxObject).activeChild;
        if (item == selected) {
          break;
        }
        EventUtils.synthesizeKey("VK_DOWN", {}, ownerDoc.defaultView);
      }

      EventUtils.synthesizeMouse(item, 1, 1, {}, ownerDoc.defaultView);

      broker.pass({'function':'MozMillDropList.select()'});
      return true;
    } catch (ex) {
      throw new Error('No item selected for element ' + this.getInfo());
      return false;
    }
  }
};









MozMillTextBox.prototype = new MozMillElement();
MozMillTextBox.prototype.parent = MozMillElement.prototype;
MozMillTextBox.prototype.constructor = MozMillTextBox;
function MozMillTextBox(locatorType, locator, args) {
  this.parent.constructor.call(this, locatorType, locator, args);
};


MozMillTextBox.isType = function(node) {
  if ((node.localName.toLowerCase() == 'input' && (node.getAttribute('type') == 'text' || node.getAttribute('type') == 'search')) ||
      (node.localName.toLowerCase() == 'textarea') ||
      (node.localName.toLowerCase() == 'textbox')) {
    return true;
  }
  return false;
};
























MozMillTextBox.prototype.sendKeys = function (aText, aModifiers, aExpectedEvent) {
  if (!this.element) {
    throw new Error("could not find element " + this.getInfo());
  }

  var element = this.element;
  Array.forEach(aText, function(letter) {
    var win = element.ownerDocument? element.ownerDocument.defaultView : element;
    element.focus();

    if (aExpectedEvent) {
      var target = aExpectedEvent.target ? aExpectedEvent.target.getNode() : element;
      EventUtils.synthesizeKeyExpectEvent(letter, aModifiers || {}, target, aExpectedEvent.type,
                                                              "MozMillTextBox.sendKeys()", win);
    } else {
      EventUtils.synthesizeKey(letter, aModifiers || {}, win);
    }
  });

  broker.pass({'function':'MozMillTextBox.type()'});
  return true;
};
