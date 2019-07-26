



var EXPORTED_SYMBOLS = ["Elem", "Selector", "ID", "Link", "XPath", "Name", "Lookup",
                        "MozMillElement", "MozMillCheckBox", "MozMillRadio", "MozMillDropList",
                        "MozMillTextBox", "subclasses"
                       ];

const NAMESPACE_XUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var EventUtils = {};  Cu.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);

var assertions = {};  Cu.import('resource://mozmill/modules/assertions.js', assertions);
var broker = {};      Cu.import('resource://mozmill/driver/msgbroker.js', broker);
var elementslib = {}; Cu.import('resource://mozmill/driver/elementslib.js', elementslib);
var utils = {};       Cu.import('resource://mozmill/stdlib/utils.js', utils);

var assert = new assertions.Assert();


var subclasses = [MozMillCheckBox, MozMillRadio, MozMillDropList, MozMillTextBox];







function createInstance(locatorType, locator, elem, document) {
  var args = { "document": document, "element": elem };

  
  if (elem) {
    for (var i = 0; i < subclasses.length; ++i) {
      if (subclasses[i].isType(elem)) {
        return new subclasses[i](locatorType, locator, args);
      }
    }
  }

  
  if (MozMillElement.isType(elem)) {
    return new MozMillElement(locatorType, locator, args);
  }

  throw new Error("Unsupported element type " + locatorType + ": " + locator);
}

var Elem = function (node) {
  return createInstance("Elem", node, node);
};

var Selector = function (document, selector, index) {
  return createInstance("Selector", selector, elementslib.Selector(document, selector, index), document);
};

var ID = function (document, nodeID) {
  return createInstance("ID", nodeID, elementslib.ID(document, nodeID), document);
};

var Link = function (document, linkName) {
  return createInstance("Link", linkName, elementslib.Link(document, linkName), document);
};

var XPath = function (document, expr) {
  return createInstance("XPath", expr, elementslib.XPath(document, expr), document);
};

var Name = function (document, nName) {
  return createInstance("Name", nName, elementslib.Name(document, nName), document);
};

var Lookup = function (document, expression) {
  var elem = createInstance("Lookup", expression, elementslib.Lookup(document, expression), document);

  
  elem.expression = elem._locator;

  return elem;
};





function MozMillElement(locatorType, locator, args) {
  args = args || {};
  this._locatorType = locatorType;
  this._locator = locator;
  this._element = args["element"];
  this._owner = args["owner"];

  this._document = this._element ? this._element.ownerDocument : args["document"];
  this._defaultView = this._document ? this._document.defaultView : null;

  
  this.isElement = true;
}


MozMillElement.isType = function (node) {
  return true;
};


MozMillElement.prototype.__defineGetter__("element", function () {
  
  
  if (this._defaultView && this._defaultView.document !== this._document) {
    this._document = this._defaultView.document;
    this._element = undefined;
  }

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


























MozMillElement.prototype.dragToElement = function(aElement, aOffsetX, aOffsetY,
                                                  aSourceWindow, aDestWindow,
                                                  aDropEffect, aDragData) {
  if (!this.element) {
    throw new Error("Could not find element " + this.getInfo());
  }
  if (!aElement) {
    throw new Error("Missing destination element");
  }

  var srcNode = this.element;
  var destNode = aElement.getNode();
  var srcWindow = aSourceWindow ||
                  (srcNode.ownerDocument ? srcNode.ownerDocument.defaultView
                                         : srcNode);
  var destWindow = aDestWindow ||
                  (destNode.ownerDocument ? destNode.ownerDocument.defaultView
                                          : destNode);

  var srcRect = srcNode.getBoundingClientRect();
  var srcCoords = {
    x: srcRect.width / 2,
    y: srcRect.height / 2
  };
  var destRect = destNode.getBoundingClientRect();
  var destCoords = {
    x: (!aOffsetX || isNaN(aOffsetX)) ? (destRect.width / 2) : aOffsetX,
    y: (!aOffsetY || isNaN(aOffsetY)) ? (destRect.height / 2) : aOffsetY
  };

  var windowUtils = destWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIDOMWindowUtils);
  var ds = Cc["@mozilla.org/widget/dragservice;1"].getService(Ci.nsIDragService);

  var dataTransfer;
  var trapDrag = function (event) {
    srcWindow.removeEventListener("dragstart", trapDrag, true);
    dataTransfer = event.dataTransfer;

    if (!aDragData) {
      return;
    }

    for (var i = 0; i < aDragData.length; i++) {
      var item = aDragData[i];
      for (var j = 0; j < item.length; j++) {
        dataTransfer.mozSetDataAt(item[j].type, item[j].data, i);
      }
    }

    dataTransfer.dropEffect = aDropEffect || "move";
    event.preventDefault();
    event.stopPropagation();
  }

  ds.startDragSession();

  try {
    srcWindow.addEventListener("dragstart", trapDrag, true);
    EventUtils.synthesizeMouse(srcNode, srcCoords.x, srcCoords.y,
                               { type: "mousedown" }, srcWindow);
    EventUtils.synthesizeMouse(destNode, destCoords.x, destCoords.y,
                               { type: "mousemove" }, destWindow);

    var event = destWindow.document.createEvent("DragEvents");
    event.initDragEvent("dragenter", true, true, destWindow, 0, 0, 0, 0, 0,
                        false, false, false, false, 0, null, dataTransfer);
    event.initDragEvent("dragover", true, true, destWindow, 0, 0, 0, 0, 0,
                        false, false, false, false, 0, null, dataTransfer);
    event.initDragEvent("drop", true, true, destWindow, 0, 0, 0, 0, 0,
                        false, false, false, false, 0, null, dataTransfer);
    windowUtils.dispatchDOMEventViaPresShell(destNode, event, true);

    EventUtils.synthesizeMouse(destNode, destCoords.x, destCoords.y,
                               { type: "mouseup" }, destWindow);

    return dataTransfer.dropEffect;
  } finally {
    ds.endDragSession(true);
  }

};


MozMillElement.prototype.getNode = function () {
  return this.element;
};

MozMillElement.prototype.getInfo = function () {
  return this._locatorType + ": " + this._locator;
};





MozMillElement.prototype.exists = function () {
  this._element = undefined;
  if (this.element) {
    return true;
  }

  return false;
};

























MozMillElement.prototype.keypress = function (aKey, aModifiers, aExpectedEvent) {
  if (!this.element) {
    throw new Error("Could not find element " + this.getInfo());
  }

  var win = this.element.ownerDocument ? this.element.ownerDocument.defaultView
                                       : this.element;
  this.element.focus();

  if (aExpectedEvent) {
    if (!aExpectedEvent.type) {
      throw new Error(arguments.callee.name + ": Expected event type not specified");
    }

    var target = aExpectedEvent.target ? aExpectedEvent.target.getNode()
                                       : this.element;
    EventUtils.synthesizeKeyExpectEvent(aKey, aModifiers || {}, target, aExpectedEvent.type,
                                        "MozMillElement.keypress()", win);
  } else {
    EventUtils.synthesizeKey(aKey, aModifiers || {}, win);
  }

  broker.pass({'function':'MozMillElement.keypress()'});

  return true;
};


































MozMillElement.prototype.mouseEvent = function (aOffsetX, aOffsetY, aEvent, aExpectedEvent) {
  if (!this.element) {
    throw new Error(arguments.callee.name + ": could not find element " + this.getInfo());
  }

  if ("document" in this.element) {
    throw new Error("A window cannot be a target for mouse events.");
  }

  var rect = this.element.getBoundingClientRect();

  if (!aOffsetX || isNaN(aOffsetX)) {
    aOffsetX = rect.width / 2;
  }

  if (!aOffsetY || isNaN(aOffsetY)) {
    aOffsetY = rect.height / 2;
  }

  
  if ("scrollIntoView" in this.element)
    this.element.scrollIntoView();

  if (aExpectedEvent) {
    
    if (!aExpectedEvent.type) {
      throw new Error(arguments.callee.name + ": Expected event type not specified");
    }

    
    var target = aExpectedEvent.target ? aExpectedEvent.target.getNode()
                                       : this.element;
    if (!target) {
      throw new Error(arguments.callee.name + ": could not find element " +
                      aExpectedEvent.target.getInfo());
    }

    EventUtils.synthesizeMouseExpectEvent(this.element, aOffsetX, aOffsetY, aEvent,
                                          target, aExpectedEvent.type,
                                          "MozMillElement.mouseEvent()",
                                          this.element.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(this.element, aOffsetX, aOffsetY, aEvent,
                               this.element.ownerDocument.defaultView);
  }

  
  
  
  utils.sleep(0);

  return true;
};




MozMillElement.prototype.click = function (aOffsetX, aOffsetY, aExpectedEvent) {
  
  if (this.element && this.element.tagName == "menuitem") {
    this.element.click();
  } else {
    this.mouseEvent(aOffsetX, aOffsetY, {}, aExpectedEvent);
  }

  broker.pass({'function':'MozMillElement.click()'});

  return true;
};




MozMillElement.prototype.doubleClick = function (aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {clickCount: 2}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.doubleClick()'});

  return true;
};




MozMillElement.prototype.mouseDown = function (aButton, aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {button: aButton, type: "mousedown"}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.mouseDown()'});

  return true;
};




MozMillElement.prototype.mouseOut = function (aButton, aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {button: aButton, type: "mouseout"}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.mouseOut()'});

  return true;
};




MozMillElement.prototype.mouseOver = function (aButton, aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {button: aButton, type: "mouseover"}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.mouseOver()'});

  return true;
};




MozMillElement.prototype.mouseUp = function (aButton, aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {button: aButton, type: "mouseup"}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.mouseUp()'});

  return true;
};




MozMillElement.prototype.middleClick = function (aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {button: 1}, aExpectedEvent);

  broker.pass({'function':'MozMillElement.middleClick()'});

  return true;
};




MozMillElement.prototype.rightClick = function (aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {type : "contextmenu", button: 2 }, aExpectedEvent);

  broker.pass({'function':'MozMillElement.rightClick()'});

  return true;
};




























































MozMillElement.prototype.touchEvent = function (aOffsetX, aOffsetY, aEvent) {
  if (!this.element) {
    throw new Error(arguments.callee.name + ": could not find element " + this.getInfo());
  }

  if ("document" in this.element) {
    throw new Error("A window cannot be a target for touch events.");
  }

  var rect = this.element.getBoundingClientRect();

  if (!aOffsetX || isNaN(aOffsetX)) {
    aOffsetX = rect.width / 2;
  }

  if (!aOffsetY || isNaN(aOffsetY)) {
    aOffsetY = rect.height / 2;
  }

  
  if ("scrollIntoView" in this.element) {
    this.element.scrollIntoView();
  }

  EventUtils.synthesizeTouch(this.element, aOffsetX, aOffsetY, aEvent,
                             this.element.ownerDocument.defaultView);

  return true;
};















MozMillElement.prototype.tap = function (aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {
    clickCount: 1,
    inputSource: Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH
  }, aExpectedEvent);

  broker.pass({'function':'MozMillElement.tap()'});

  return true;
};















MozMillElement.prototype.doubleTap = function (aOffsetX, aOffsetY, aExpectedEvent) {
  this.mouseEvent(aOffsetX, aOffsetY, {
    clickCount: 2,
    inputSource: Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH
  }, aExpectedEvent);

  broker.pass({'function':'MozMillElement.doubleTap()'});

  return true;
};











MozMillElement.prototype.longPress = function (aOffsetX, aOffsetY, aTime) {
  var time = aTime || 1000;

  this.touchStart(aOffsetX, aOffsetY);
  utils.sleep(time);
  this.touchEnd(aOffsetX, aOffsetY);

  broker.pass({'function':'MozMillElement.longPress()'});

  return true;
};













MozMillElement.prototype.touchDrag = function (aOffsetX1, aOffsetY1, aOffsetX2, aOffsetY2) {
  this.touchStart(aOffsetX1, aOffsetY1);
  this.touchMove(aOffsetX2, aOffsetY2);
  this.touchEnd(aOffsetX2, aOffsetY2);

  broker.pass({'function':'MozMillElement.move()'});

  return true;
};









MozMillElement.prototype.touchStart = function (aOffsetX, aOffsetY) {
  this.touchEvent(aOffsetX, aOffsetY, { type: "touchstart" });

  broker.pass({'function':'MozMillElement.touchStart()'});

  return true;
};









MozMillElement.prototype.touchEnd = function (aOffsetX, aOffsetY) {
  this.touchEvent(aOffsetX, aOffsetY, { type: "touchend" });

  broker.pass({'function':'MozMillElement.touchEnd()'});

  return true;
};









MozMillElement.prototype.touchMove = function (aOffsetX, aOffsetY) {
  this.touchEvent(aOffsetX, aOffsetY, { type: "touchmove" });

  broker.pass({'function':'MozMillElement.touchMove()'});

  return true;
};

MozMillElement.prototype.waitForElement = function (timeout, interval) {
  var elem = this;

  assert.waitFor(function () {
    return elem.exists();
  }, "Element.waitForElement(): Element '" + this.getInfo() +
     "' has been found", timeout, interval);

  broker.pass({'function':'MozMillElement.waitForElement()'});
};

MozMillElement.prototype.waitForElementNotPresent = function (timeout, interval) {
  var elem = this;

  assert.waitFor(function () {
    return !elem.exists();
  }, "Element.waitForElementNotPresent(): Element '" + this.getInfo() +
     "' has not been found", timeout, interval);

  broker.pass({'function':'MozMillElement.waitForElementNotPresent()'});
};

MozMillElement.prototype.waitThenClick = function (timeout, interval,
                                                   aOffsetX, aOffsetY, aExpectedEvent) {
  this.waitForElement(timeout, interval);
  this.click(aOffsetX, aOffsetY, aExpectedEvent);
};



















MozMillElement.prototype.waitThenTap = function (aTimeout, aInterval,
                                                 aOffsetX, aOffsetY, aExpectedEvent) {
  this.waitForElement(aTimeout, aInterval);
  this.tap(aOffsetX, aOffsetY, aExpectedEvent);
};


MozMillElement.prototype.dispatchEvent = function (eventType, canBubble, modifiers) {
  canBubble = canBubble || true;
  modifiers = modifiers || { };

  let document = 'ownerDocument' in this.element ? this.element.ownerDocument
                                                 : this.element.document;

  let evt = document.createEvent('HTMLEvents');
  evt.shiftKey = modifiers["shift"];
  evt.metaKey = modifiers["meta"];
  evt.altKey = modifiers["alt"];
  evt.ctrlKey = modifiers["ctrl"];
  evt.initEvent(eventType, canBubble, true);

  this.element.dispatchEvent(evt);
};





function MozMillCheckBox(locatorType, locator, args) {
  MozMillElement.call(this, locatorType, locator, args);
}


MozMillCheckBox.prototype = Object.create(MozMillElement.prototype, {
  check : {
    





    value : function MMCB_check(state) {
      var result = false;

      if (!this.element) {
        throw new Error("could not find element " + this.getInfo());
      }

      
      if (this.element.namespaceURI == NAMESPACE_XUL) {
        this.element = utils.unwrapNode(this.element);
      }

      state = (typeof(state) == "boolean") ? state : false;
      if (state != this.element.checked) {
        this.click();
        var element = this.element;

        assert.waitFor(function () {
          return element.checked == state;
        }, "CheckBox.check(): Checkbox " + this.getInfo() + " could not be checked/unchecked", 500);

        result = true;
      }

      broker.pass({'function':'MozMillCheckBox.check(' + this.getInfo() +
                   ', state: ' + state + ')'});

      return result;
    }
  }
});









MozMillCheckBox.isType = function MMCB_isType(node) {
  return ((node.localName.toLowerCase() == "input" && node.getAttribute("type") == "checkbox") ||
    (node.localName.toLowerCase() == 'toolbarbutton' && node.getAttribute('type') == 'checkbox') ||
    (node.localName.toLowerCase() == 'checkbox'));
};





function MozMillRadio(locatorType, locator, args) {
  MozMillElement.call(this, locatorType, locator, args);
}


MozMillRadio.prototype = Object.create(MozMillElement.prototype, {
  select : {
    







    value : function MMR_select(index) {
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

      assert.waitFor(function () {
        
        if (element.namespaceURI == NAMESPACE_XUL) {
          element = utils.unwrapNode(element);
          return element.selected == true;
        }

        return element.checked == true;
      }, "Radio.select(): Radio button " + this.getInfo() + " has been selected", 500);

      broker.pass({'function':'MozMillRadio.select(' + this.getInfo() + ')'});

      return true;
    }
  }
});









MozMillRadio.isType = function MMR_isType(node) {
  return ((node.localName.toLowerCase() == 'input' && node.getAttribute('type') == 'radio') ||
    (node.localName.toLowerCase() == 'toolbarbutton' && node.getAttribute('type') == 'radio') ||
    (node.localName.toLowerCase() == 'radio') ||
    (node.localName.toLowerCase() == 'radiogroup'));
};





function MozMillDropList(locatorType, locator, args) {
  MozMillElement.call(this, locatorType, locator, args);
}


MozMillDropList.prototype = Object.create(MozMillElement.prototype, {
  select : {
    



    value : function MMDL_select(index, option, value) {
      if (!this.element){
        throw new Error("Could not find element " + this.getInfo());
      }

      
      if (this.element.localName.toLowerCase() == "select"){
        var item = null;

        
        if (index != undefined) {
          
          if (index == -1) {
            this.dispatchEvent('focus', false);
            this.element.selectedIndex = index;
            this.dispatchEvent('change', true);

            broker.pass({'function':'MozMillDropList.select()'});

            return true;
          } else {
            item = this.element.options.item(index);
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

          var self = this;
          var selected = index || option || value;
          assert.waitFor(function () {
            switch (selected) {
              case index:
                return selected === self.element.selectedIndex;
                break;
              case option:
                return selected === item.label;
                break;
              case value:
                return selected === item.value;
                break;
            }
          }, "DropList.select(): The correct item has been selected");

          broker.pass({'function':'MozMillDropList.select()'});

          return true;
        } catch (e) {
          throw new Error("No item selected for element " + this.getInfo());
        }
      }
      
      else if (this.element.namespaceURI.toLowerCase() == NAMESPACE_XUL) {
        var ownerDoc = this.element.ownerDocument;
        
        this.element = utils.unwrapNode(this.element);
        
        var menuitems = this.element.
                        getElementsByTagNameNS(NAMESPACE_XUL, "menupopup")[0].
                        getElementsByTagNameNS(NAMESPACE_XUL, "menuitem");

        var item = null;

        if (index != undefined) {
          if (index == -1) {
            this.dispatchEvent('focus', false);
            this.element.boxObject.QueryInterface(Ci.nsIMenuBoxObject).activeChild = null;
            this.dispatchEvent('change', true);

            broker.pass({'function':'MozMillDropList.select()'});

            return true;
          } else {
            item = menuitems[index];
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
          item.click();

          var self = this;
          var selected = index || option || value;
          assert.waitFor(function () {
            switch (selected) {
              case index:
                return selected === self.element.selectedIndex;
                break;
              case option:
                return selected === self.element.label;
                break;
              case value:
                return selected === self.element.value;
                break;
            }
          }, "DropList.select(): The correct item has been selected");

          broker.pass({'function':'MozMillDropList.select()'});

          return true;
        } catch (e) {
          throw new Error('No item selected for element ' + this.getInfo());
        }
      }
    }
  }
});









MozMillDropList.isType = function MMR_isType(node) {
  return ((node.localName.toLowerCase() == 'toolbarbutton' &&
    (node.getAttribute('type') == 'menu' || node.getAttribute('type') == 'menu-button')) ||
    (node.localName.toLowerCase() == 'menu') ||
    (node.localName.toLowerCase() == 'menulist') ||
    (node.localName.toLowerCase() == 'select' ));
};





function MozMillTextBox(locatorType, locator, args) {
  MozMillElement.call(this, locatorType, locator, args);
}


MozMillTextBox.prototype = Object.create(MozMillElement.prototype, {
  sendKeys : {
    























    value : function MMTB_sendKeys(aText, aModifiers, aExpectedEvent) {
      if (!this.element) {
        throw new Error("could not find element " + this.getInfo());
      }

      var element = this.element;
      Array.forEach(aText, function (letter) {
        var win = element.ownerDocument ? element.ownerDocument.defaultView
          : element;
        element.focus();

        if (aExpectedEvent) {
          if (!aExpectedEvent.type) {
            throw new Error(arguments.callee.name + ": Expected event type not specified");
          }

          var target = aExpectedEvent.target ? aExpectedEvent.target.getNode()
            : element;
          EventUtils.synthesizeKeyExpectEvent(letter, aModifiers || {}, target,
            aExpectedEvent.type,
            "MozMillTextBox.sendKeys()", win);
        } else {
          EventUtils.synthesizeKey(letter, aModifiers || {}, win);
        }
      });

      broker.pass({'function':'MozMillTextBox.type()'});

      return true;
    }
  }
});









MozMillTextBox.isType = function MMR_isType(node) {
  return ((node.localName.toLowerCase() == 'input' &&
    (node.getAttribute('type') == 'text' || node.getAttribute('type') == 'search')) ||
    (node.localName.toLowerCase() == 'textarea') ||
    (node.localName.toLowerCase() == 'textbox'));
};
