






































var EXPORTED_SYMBOLS = ["MozMillController", "waitForEval", "MozMillAsyncTest",
                        "globalEventRegistry", "sleep"];

var EventUtils = {}; Components.utils.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);

var events = {}; Components.utils.import('resource://mozmill/modules/events.js', events);
var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);
var elementslib = {}; Components.utils.import('resource://mozmill/modules/elementslib.js', elementslib);
var frame = {}; Components.utils.import('resource://mozmill/modules/frame.js', frame);

var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
                .getService(Components.interfaces.nsIAppShellService)
                .hiddenDOMWindow;
var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
     getService(Components.interfaces.nsIConsoleService);


var sleep = utils.sleep;
var assert = utils.assert;
var waitFor = utils.waitFor;
var waitForEval = utils.waitForEval;


waitForEvents = function() {}

waitForEvents.prototype = {
  


  init : function waitForEvents_init(node, events) {
    if (node.getNode != undefined)
      node = node.getNode();
  
    this.events = events;
    this.node = node;
    node.firedEvents = {};
    this.registry = {};
  
    for each(e in events) {
      var listener = function(event) {
        this.firedEvents[event.type] = true;
      }
      this.registry[e] = listener;
      this.registry[e].result = false;
      this.node.addEventListener(e, this.registry[e], true);
    }
  },

  


  wait : function waitForEvents_wait(timeout, interval)
  {
    for (var e in this.registry) {
      utils.waitFor(function() {
        return this.node.firedEvents[e] == true;
      }, "Timeout happened before event '" + ex +"' was fired.", timeout, interval);
  
      this.node.removeEventListener(e, this.registry[e], true);
    }
  }
}














var Menu = function(controller, menuSelector, document) {
  this._controller = controller;
  this._menu = null;

  document = document || controller.window.document;
  var node = document.querySelector(menuSelector);
  if (node) {
    
    node = node.wrappedJSObject || node;
    this._menu = new elementslib.Elem(node);
  }
  else {
    throw new Error("Menu element '" + menuSelector + "' not found.");
  }
}

Menu.prototype = {

  






  open : function(contextElement) {
    
    var menu = this._menu.getNode();
    if (menu.localName == "menupopup" &&
        contextElement && contextElement.exists()) {
      this._controller.rightClick(contextElement);
      this._controller.waitFor(function() {
        return menu.state == "open";
      }, "Context menu has been opened.");
    }

    
    this._buildMenu(this._controller, menu);

    return this;
  },

  




  close : function() {
    var menu = this._menu.getNode();

    this._controller.keypress(this._menu, "VK_ESCAPE", {});
    this._controller.waitFor(function() {
      return menu.state == "closed";
    }, "Context menu has been closed.");

    return this;
  },

  







  getItem : function(itemSelector) {
    var node = this._menu.getNode().querySelector(itemSelector);

    if (!node) {
      throw new Error("Menu entry '" + itemSelector + "' not found.");
    }

    return new elementslib.Elem(node);
  },

  







  click : function(itemSelector) {
    this._controller.click(this.getItem(itemSelector));

    return this;
  },

  










  keypress : function(key, modifier) {
    this._controller.keypress(this._menu, key, modifier);

    return this;
  },

  










  select : function(itemSelector, contextElement) {
    this.open(contextElement);
    this.click(itemSelector);
    this.close();
  },

  






  _buildMenu : function(menu) {
    var items = menu ? menu.childNodes : null;

    Array.forEach(items, function(item) {
      
      
      if (item.tagName == "menu") {
        var popup = item.querySelector("menupopup");
        if (popup) {
          if (popup.allowevents) {
            events.fakeOpenPopup(this._controller.window, popup);
          }
          this._buildMenu(popup);
        }
      }
    }, this);
  }
};




var MenuTree = function(aWindow, aMenu) {
  var items = aMenu ? aMenu.childNodes : null;

  for each (var node in items) {
    var entry = null;

    switch (node.tagName) {
      case "menu":
        
        var popup = node.querySelector("menupopup");
        if (popup) {
          if (popup.allowevents) {
            events.fakeOpenPopup(aWindow, popup);
          }
          entry = new MenuTree(aWindow, popup);
        }
        break;
      case "menuitem":
        entry = node;
        break;
      default:
        continue;
    }

    if (entry) {
      var label = node.getAttribute("label");
      this[label] = entry;

      if (node.id)
        this[node.id] = this[label];
    }
  }
};

var MozMillController = function (window) {
  this.window = window;

  this.mozmillModule = {};
  Components.utils.import('resource://mozmill/modules/mozmill.js', this.mozmillModule);

  utils.waitFor(function() {
    return window != null && (window.documentLoaded != undefined);
  }, "controller(): Window could not be initialized.");

  if ( controllerAdditions[window.document.documentElement.getAttribute('windowtype')] != undefined ) {
    this.prototype = new utils.Copy(this.prototype);
    controllerAdditions[window.document.documentElement.getAttribute('windowtype')](this);
    this.windowtype = window.document.documentElement.getAttribute('windowtype');
  }
}

MozMillController.prototype.sleep = utils.sleep;



























MozMillController.prototype.keypress = function(aTarget, aKey, aModifiers, aExpectedEvent) {
  var element = (aTarget == null) ? this.window : aTarget.getNode();
  if (!element) {
    throw new Error("Could not find element " + aTarget.getInfo());
    return false;
  }

  events.triggerKeyEvent(element, 'keypress', aKey, aModifiers, aExpectedEvent);

  frame.events.pass({'function':'Controller.keypress()'});
  return true;
}














MozMillController.prototype.type = function (aTarget, aText, aExpectedEvent) {
  var element = (aTarget == null) ? this.window : aTarget.getNode();
  if (!element) {
    throw new Error("could not find element " + aTarget.getInfo());
    return false;
  }

  Array.forEach(aText, function(letter) {
    events.triggerKeyEvent(element, 'keypress', letter, {}, aExpectedEvent);
  });

  frame.events.pass({'function':'Controller.type()'});
  return true;
}


MozMillController.prototype.open = function(url)
{
  switch(this.mozmillModule.Application) {
    case "Firefox":
      this.window.gBrowser.loadURI(url);
      break;
    case "SeaMonkey":
      this.window.getBrowser().loadURI(url);
      break;
    default:
      throw new Error("MozMillController.open not supported.");
  }

  frame.events.pass({'function':'Controller.open()'});
}



































MozMillController.prototype.mouseEvent = function(aTarget, aOffsetX, aOffsetY,
                                                  aEvent, aExpectedEvent) {

  var element = aTarget.getNode();
  if (!element) {
    throw new Error(arguments.callee.name + ": could not find element " +
                    aTarget.getInfo());
  }

  
  var rect = element.getBoundingClientRect();
  if (isNaN(aOffsetX))
    aOffsetX = rect.width / 2;
  if (isNaN(aOffsetY))
    aOffsetY = rect.height / 2;

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  if (aExpectedEvent) {
    
    if (!aExpectedEvent.type)
      throw new Error(arguments.callee.name + ": Expected event type not specified");

    
    var target = aExpectedEvent.target ? aExpectedEvent.target.getNode() : element;
    if (!target) {
      throw new Error(arguments.callee.name + ": could not find element " +
                      aExpectedEvent.target.getInfo());
    }

    EventUtils.synthesizeMouseExpectEvent(element, aOffsetX, aOffsetY, aEvent,
                                          target, aExpectedEvent.event,
                                          "controller.mouseEvent()",
                                          element.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(element, aOffsetX, aOffsetY, aEvent,
                               element.ownerDocument.defaultView);
  }

  sleep(0);
}




MozMillController.prototype.click = function(elem, left, top, expectedEvent) {
  var element = elem.getNode()

  
  if (element && element.tagName == "menuitem") {
    element.click();
  } else {
    this.mouseEvent(elem, left, top, {}, expectedEvent);
  }

  frame.events.pass({'function':'controller.click()'});
}




MozMillController.prototype.doubleClick = function(elem, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {clickCount: 2}, expectedEvent);

  frame.events.pass({'function':'controller.doubleClick()'});
  return true;
}




MozMillController.prototype.mouseDown = function (elem, button, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {button: button, type: "mousedown"}, expectedEvent);

  frame.events.pass({'function':'controller.mouseDown()'});
  return true;
};




MozMillController.prototype.mouseOut = function (elem, button, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {button: button, type: "mouseout"}, expectedEvent);

  frame.events.pass({'function':'controller.mouseOut()'});
  return true;
};




MozMillController.prototype.mouseOver = function (elem, button, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {button: button, type: "mouseover"}, expectedEvent);

  frame.events.pass({'function':'controller.mouseOver()'});
  return true;
};




MozMillController.prototype.mouseUp = function (elem, button, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {button: button, type: "mouseup"}, expectedEvent);

  frame.events.pass({'function':'controller.mouseUp()'});
  return true;
};




MozMillController.prototype.middleClick = function(elem, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {button: 1}, expectedEvent);

  frame.events.pass({'function':'controller.middleClick()'});
  return true;
}




MozMillController.prototype.rightClick = function(elem, left, top, expectedEvent) {
  this.mouseEvent(elem, left, top, {type : "contextmenu", button: 2 }, expectedEvent);

  frame.events.pass({'function':'controller.rightClick()'});
  return true;
}




MozMillController.prototype.rightclick = function(){
  frame.log({function:'rightclick - Deprecation Warning', message:'Controller.rightclick should be renamed to Controller.rightClick'});
  this.rightClick.apply(this, arguments);
}




MozMillController.prototype.check = function(el, state) {
  var result = false;
  var element = el.getNode();

  if (!element) {
    throw new Error("could not find element " + el.getInfo());
    return false;
  }

  state = (typeof(state) == "boolean") ? state : false;
  if (state != element.checked) {
    this.click(el);
    this.waitFor(function() {
      return element.checked == state;
    }, "Checkbox " + el.getInfo() + " could not be checked/unchecked", 500);

    result = true;
  }

  frame.events.pass({'function':'Controller.check(' + el.getInfo() + ', state: ' + state + ')'});
  return result;
}




MozMillController.prototype.radio = function(el)
{
  var element = el.getNode();
  if (!element) {
    throw new Error("could not find element " + el.getInfo());
    return false;
  }

  this.click(el);
  this.waitFor(function() {
    return element.selected == true;
  }, "Radio button " + el.getInfo() + " could not be selected", 500);

  frame.events.pass({'function':'Controller.radio(' + el.getInfo() + ')'});
  return true;
}

MozMillController.prototype.waitFor = function(callback, message, timeout,
                                               interval, thisObject) {
  utils.waitFor(callback, message, timeout, interval, thisObject);

  frame.events.pass({'function':'controller.waitFor()'});
}

MozMillController.prototype.waitForEval = function(expression, timeout, interval, subject) {
  waitFor(function() {
    return eval(expression);
  }, "controller.waitForEval: Timeout exceeded for '" + expression + "'", timeout, interval);

  frame.events.pass({'function':'controller.waitForEval()'});
}

MozMillController.prototype.waitForElement = function(elem, timeout, interval) {
  this.waitFor(function() {
    return elem.exists();
  }, "Timeout exceeded for waitForElement " + elem.getInfo(), timeout, interval);

  frame.events.pass({'function':'Controller.waitForElement()'});
}

MozMillController.prototype.__defineGetter__("waitForEvents", function() {
  if (this._waitForEvents == undefined)
    this._waitForEvents = new waitForEvents();
  return this._waitForEvents;
});





MozMillController.prototype.getMenu = function (menuSelector, document) {
  return new Menu(this, menuSelector, document);
};

MozMillController.prototype.__defineGetter__("mainMenu", function() {
  return this.getMenu("menubar");
});

MozMillController.prototype.__defineGetter__("menus", function() {
  frame.log({'property': 'controller.menus - DEPRECATED',
             'message': 'Use controller.mainMenu instead.'});

  var menubar = this.window.document.querySelector("menubar");
  return new MenuTree(this.window, menubar);
});

MozMillController.prototype.waitForImage = function (elem, timeout, interval) {
  this.waitFor(function() {
    return elem.getNode().complete == true;
  }, "timeout exceeded for waitForImage " + elem.getInfo(), timeout, interval);

  frame.events.pass({'function':'Controller.waitForImage()'});
}

MozMillController.prototype.waitThenClick = function (elem, timeout, interval) {
  this.waitForElement(elem, timeout, interval);
  this.click(elem);
}

MozMillController.prototype.fireEvent = function (name, obj) {
  if (name == "userShutdown") {
    frame.events.toggleUserShutdown();
  }
  frame.events.fireEvent(name, obj);
}

MozMillController.prototype.startUserShutdown = function (timeout, restart) {
  
  this.fireEvent('userShutdown', restart ? 2 : 1);
  this.window.setTimeout(this.fireEvent, timeout, 'userShutdown', 0);
}


MozMillController.prototype.select = function (el, indx, option, value) {
  element = el.getNode();
  if (!element){
    throw new Error("Could not find element " + el.getInfo());
    return false;
  }

  
  if (element.localName.toLowerCase() == "select"){
    var item = null;

    
    if (indx != undefined) {
      
      if (indx == -1) {
        events.triggerEvent(element, 'focus', false);
        element.selectedIndex = indx;
        events.triggerEvent(element, 'change', true);

     frame.events.pass({'function':'Controller.select()'});
     return true;
      } else {
        item = element.options.item(indx);
    }
    } else {
      for (var i = 0; i < element.options.length; i++) {
        var entry = element.options.item(i);
        if (option != undefined && entry.innerHTML == option ||
            value != undefined && entry.value == value) {
          item = entry;
         break;
       }
     }
           }

    
    try {
      
      events.triggerEvent(element, 'focus', false);
      item.selected = true;
           events.triggerEvent(element, 'change', true);

      frame.events.pass({'function':'Controller.select()'});
      return true;
    } catch (ex) {
      throw new Error("No item selected for element " + el.getInfo());
     return false;
   }
  }
  
  else if (element.localName.toLowerCase() == "menulist"){
    var item = null;

    if (indx != undefined) {
      if (indx == -1) {
        events.triggerEvent(element, 'focus', false);
      element.selectedIndex = indx;
        events.triggerEvent(element, 'change', true);

        frame.events.pass({'function':'Controller.select()'});
        return true;
      } else {
        item = element.getItemAtIndex(indx);
      }
    } else {
      for (var i = 0; i < element.itemCount; i++) {
        var entry = element.getItemAtIndex(i);
        if (option != undefined && entry.label == option ||
            value != undefined && entry.value == value) {
          item = entry;
          break;
    }
    }
  }

    
    try {
      EventUtils.synthesizeMouse(element, 1, 1, {}, item.ownerDocument.defaultView);
      this.sleep(0);

      
      for (var i = s = element.selectedIndex; i <= element.itemCount + s; ++i) {
        var entry = element.getItemAtIndex((i + 1) % element.itemCount);
        EventUtils.synthesizeKey("VK_DOWN", {}, element.ownerDocument.defaultView);
        if (entry.label == item.label) {
          break;
        }
        else if (entry.label == "") i += 1;
      }

      EventUtils.synthesizeMouse(item, 1, 1, {}, item.ownerDocument.defaultView);
      this.sleep(0);

   frame.events.pass({'function':'Controller.select()'});
   return true;
    } catch (ex) {
      throw new Error('No item selected for element ' + el.getInfo());
      return false;
    }
  }
};


MozMillController.prototype.goBack = function(){
  
  this.window.content.history.back();
  frame.events.pass({'function':'Controller.goBack()'});
  return true;
}
MozMillController.prototype.goForward = function(){
  
  this.window.content.history.forward();
  frame.events.pass({'function':'Controller.goForward()'});
  return true;
}
MozMillController.prototype.refresh = function(){
  
  this.window.content.location.reload(true);
  frame.events.pass({'function':'Controller.refresh()'});
  return true;
}

MozMillController.prototype.assertText = function (el, text) {
  
  var n = el.getNode();

  if (n && n.innerHTML == text){
    frame.events.pass({'function':'Controller.assertText()'});
    return true;
   }

  throw new Error("could not validate element " + el.getInfo()+" with text "+ text);
  return false;

};


MozMillController.prototype.assertNode = function (el) {
  
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  frame.events.pass({'function':'Controller.assertNode()'});
  return true;
};


MozMillController.prototype.assertNodeNotExist = function (el) {
  
  try {
    var element = el.getNode();
  } catch(err){
    frame.events.pass({'function':'Controller.assertNodeNotExist()'});
    return true;
  }

  if (element) {
    throw new Error("Unexpectedly found element " + el.getInfo());
    return false;
  } else {
    frame.events.pass({'function':'Controller.assertNodeNotExist()'});
    return true;
  }
};


MozMillController.prototype.assertValue = function (el, value) {
  
  var n = el.getNode();

  if (n && n.value == value){
    frame.events.pass({'function':'Controller.assertValue()'});
    return true;
  }
  throw new Error("could not validate element " + el.getInfo()+" with value "+ value);
  return false;
};




MozMillController.prototype.assert = function(callback, message, thisObject)
{
  utils.assert(callback, message, thisObject);

  frame.events.pass({'function': ": controller.assert('" + callback + "')"});
  return true;
}


MozMillController.prototype.assertJS = function(expression, subject) {
  assert(function() {
    return eval(expression)
  }, "controller.assertJS: Failed for '" + expression + "'");

  frame.events.pass({'function': "controller.assertJS('" + expression + "')"});
  return true;
}


MozMillController.prototype.assertSelected = function (el, value) {
  
  var n = el.getNode();
  var validator = value;

  if (n && n.options[n.selectedIndex].value == validator){
    frame.events.pass({'function':'Controller.assertSelected()'});
    return true;
    }
  throw new Error("could not assert value for element " + el.getInfo()+" with value "+ value);
  return false;
};


MozMillController.prototype.assertChecked = function (el) {
  
  var element = el.getNode();

  if (element && element.checked == true){
    frame.events.pass({'function':'Controller.assertChecked()'});
    return true;
    }
  throw new Error("assert failed for checked element " + el.getInfo());
  return false;
};


MozMillController.prototype.assertNotChecked = function (el) {
  var element = el.getNode();

  if (!element) {
    throw new Error("Could not find element" + el.getInfo());
  }

  if (!element.hasAttribute("checked") || element.checked != true){
    frame.events.pass({'function':'Controller.assertNotChecked()'});
    return true;
    }
  throw new Error("assert failed for not checked element " + el.getInfo());
  return false;
};







MozMillController.prototype.assertJSProperty = function(el, attrib, val) {
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value = element[attrib];
  var res = (value !== undefined && (val === undefined ? true : String(value) == String(val)));
  if (res) {
    frame.events.pass({'function':'Controller.assertJSProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertJSProperty(" + el.getInfo() + ") : " + 
                     (val === undefined ? "property '" + attrib + "' doesn't exist" : val + " == " + value));
  }
  return res;
};







MozMillController.prototype.assertNotJSProperty = function(el, attrib, val) {
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value = element[attrib];
  var res = (val === undefined ? value === undefined : String(value) != String(val));
  if (res) {
    frame.events.pass({'function':'Controller.assertNotProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertNotJSProperty(" + el.getInfo() + ") : " +
                     (val === undefined ? "property '" + attrib + "' exists" : val + " != " + value));
  }
  return res;
};







MozMillController.prototype.assertDOMProperty = function(el, attrib, val) {
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value, res = element.hasAttribute(attrib);
  if (res && val !== undefined) {
    value = element.getAttribute(attrib);
    res = (String(value) == String(val));
  }   
 
  if (res) {
    frame.events.pass({'function':'Controller.assertDOMProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertDOMProperty(" + el.getInfo() + ") : " + 
                     (val === undefined ? "property '" + attrib + "' doesn't exist" : val + " == " + value));
  }
  return res;
};







MozMillController.prototype.assertNotDOMProperty = function(el, attrib, val) {
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value, res = element.hasAttribute(attrib);
  if (res && val !== undefined) {
    value = element.getAttribute(attrib);
    res = (String(value) == String(val));
  }   
  if (!res) {
    frame.events.pass({'function':'Controller.assertNotDOMProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertNotDOMProperty(" + el.getInfo() + ") : " + 
                     (val == undefined ? "property '" + attrib + "' exists" : val + " == " + value));
  }
  return !res;
};


MozMillController.prototype.assertProperty = function(el, attrib, val) {
  frame.log({'function':'controller.assertProperty() - DEPRECATED', 
                      'message':'assertProperty(el, attrib, val) is deprecated. Use assertJSProperty(el, attrib, val) or assertDOMProperty(el, attrib, val) instead'});
  return this.assertJSProperty(el, attrib, val);
};


MozMillController.prototype.assertPropertyNotExist = function(el, attrib) {
  frame.log({'function':'controller.assertPropertyNotExist() - DEPRECATED',
                   'message':'assertPropertyNotExist(el, attrib) is deprecated. Use assertNotJSProperty(el, attrib) or assertNotDOMProperty(el, attrib) instead'});
  return this.assertNotJSProperty(el, attrib);
};




MozMillController.prototype.assertImageLoaded = function (el) {
  
  var img = el.getNode();
  if (!img || img.tagName != 'IMG') {
    throw new Error('Controller.assertImageLoaded() failed.')
    return false;
  }
  var comp = img.complete;
  var ret = null; 

  
  
  if (typeof comp == 'undefined') {
    test = new Image();
    
    
    test.src = img.src;
    comp = test.complete;
  }

  
  
  
  
  
  if (comp === false) {
    ret = false;
  }
  
  
  else if (comp === true && img.naturalWidth == 0) {
    ret = false;
  }
  
  
  else {
    ret = true;
  }
  if (ret) {
    frame.events.pass({'function':'Controller.assertImageLoaded'});
  } else {
    throw new Error('Controller.assertImageLoaded() failed.')
  }

  return ret;
};


MozMillController.prototype.mouseMove = function (doc, start, dest) {
  
  if (typeof start != 'object'){
    throw new Error("received bad coordinates");
    return false;
  }
  if (typeof dest != 'object'){
    throw new Error("received bad coordinates");
    return false;
  }

  
  events.triggerMouseEvent(doc.body, 'mousemove', true, start[0], start[1]);
  events.triggerMouseEvent(doc.body, 'mousemove', true, dest[0], dest[1]);
  frame.events.pass({'function':'Controller.mouseMove()'});
  return true;
}


MozMillController.prototype.dragDropElemToElem = function (dstart, ddest) {
  
  var drag = dstart.getNode();
  var dest = ddest.getNode();

  
  if (!drag){
    throw new Error("could not find element " + drag.getInfo());
    return false;
  }
  if (!dest){
    throw new Error("could not find element " + dest.getInfo());
    return false;
  }

  var dragCoords = null;
  var destCoords = null;

  dragCoords = drag.getBoundingClientRect();
  destCoords = dest.getBoundingClientRect();

  
  events.triggerMouseEvent(drag.ownerDocument.body, 'mousemove', true, dragCoords.left, dragCoords.top);
  events.triggerMouseEvent(drag, 'mousedown', true, dragCoords.left, dragCoords.top); 
  events.triggerMouseEvent(drag.ownerDocument.body, 'mousemove', true, destCoords.left, destCoords.top);
  events.triggerMouseEvent(dest, 'mouseup', true, destCoords.left, destCoords.top);
  events.triggerMouseEvent(dest, 'click', true, destCoords.left, destCoords.top);
  frame.events.pass({'function':'Controller.dragDropElemToElem()'});
  return true;
}

function preferencesAdditions(controller) {
  var mainTabs = controller.window.document.getAnonymousElementByAttribute(controller.window.document.documentElement, 'anonid', 'selector');
  controller.tabs = {};
  for (var i = 0; i < mainTabs.childNodes.length; i++) {
    var node  = mainTabs.childNodes[i];
    var obj = {'button':node}
    controller.tabs[i] = obj;
    var label = node.attributes.item('label').value.replace('pane', '');
    controller.tabs[label] = obj;
  }
  controller.prototype.__defineGetter__("activeTabButton",
    function () {return mainTabs.getElementsByAttribute('selected', true)[0];
  })
}

function Tabs (controller) {
  this.controller = controller;
}
Tabs.prototype.getTab = function(index) {
  return this.controller.window.gBrowser.browsers[index].contentDocument;
}
Tabs.prototype.__defineGetter__("activeTab", function() {
  return this.controller.window.gBrowser.selectedBrowser.contentDocument;
})
Tabs.prototype.selectTab = function(index) {
  
}
Tabs.prototype.findWindow = function (doc) {
  for (var i = 0; i <= (this.controller.window.frames.length - 1); i++) {
    if (this.controller.window.frames[i].document == doc) {
      return this.controller.window.frames[i];
    }
  }
  throw new Error("Cannot find window for document. Doc title == " + doc.title);
}
Tabs.prototype.getTabWindow = function(index) {
  return this.findWindow(this.getTab(index));
}
Tabs.prototype.__defineGetter__("activeTabWindow", function () {
  return this.findWindow(this.activeTab);
})
Tabs.prototype.__defineGetter__("length", function () {
  return this.controller.window.gBrowser.browsers.length;
})
Tabs.prototype.__defineGetter__("activeTabIndex", function() {
  return this.controller.window.gBrowser.tabContainer.selectedIndex;
})
Tabs.prototype.selectTabIndex = function(i) {
  this.controller.window.gBrowser.selectTabAtIndex(i);
}

function browserAdditions (controller) {
  controller.tabs = new Tabs(controller);

  controller.waitForPageLoad = function(aTabDocument, aTimeout, aInterval) {
    var timeout = aTimeout || 30000;
    var tab = null;

    
    
    if (typeof(aTabDocument) == "number"){
      timeout = aTabDocument;
    }

    
    if (aTabDocument && typeof(aTabDocument) == "object") {
      for each (var browser in this.window.gBrowser.browsers) {
        if (browser.contentDocument == aTabDocument) {
          tab = browser;
          break;
        }
      }

      if (!tab) {
        throw new Error("controller.waitForPageLoad(): Specified tab hasn't been found.");
      }
    }

    
    tab = this.window.gBrowser.selectedBrowser;

    
    this.waitFor(function() {
      return tab.contentDocument.documentLoaded;
    }, "controller.waitForPageLoad(): Timeout waiting for page loaded.", timeout, aInterval);

    frame.events.pass({'function':'controller.waitForPageLoad()'});
  }
}

controllerAdditions = {
  'Browser:Preferences':preferencesAdditions,
  'navigator:browser'  :browserAdditions,
}

var withs = {}; Components.utils.import('resource://mozmill/stdlib/withs.js', withs);

MozMillAsyncTest = function (timeout) {
  if (timeout == undefined) {
    this.timeout = 6000;
  } else {
    this.timeout = timeout;
  }
  this._done = false;
  this._mozmillasynctest = true;
}

MozMillAsyncTest.prototype.run = function () {
  for (var i in this) {
    if (withs.startsWith(i, 'test') && typeof(this[i]) == 'function') {
      this[i]();
    }
  }

  utils.waitFor(function() {
    return this._done == true;
  }, "MozMillAsyncTest timed out. Done is " + this._done, 500, 100); 

  return true;
}

MozMillAsyncTest.prototype.finish = function () {
  this._done = true;
}
