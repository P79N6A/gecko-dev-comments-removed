







































var EXPORTED_SYMBOLS = ["MozMillController", "globalEventRegistry", "sleep"];

var EventUtils = {}; Components.utils.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);

var utils = {}; Components.utils.import('resource://mozmill/stdlib/utils.js', utils);
var elementslib = {}; Components.utils.import('resource://mozmill/driver/elementslib.js', elementslib);
var mozelement = {}; Components.utils.import('resource://mozmill/driver/mozelement.js', mozelement);
var broker = {}; Components.utils.import('resource://mozmill/driver/msgbroker.js', broker);

var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
                .getService(Components.interfaces.nsIAppShellService)
                .hiddenDOMWindow;
var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
     getService(Components.interfaces.nsIConsoleService);


var sleep = utils.sleep;
var assert = utils.assert;
var waitFor = utils.waitFor;

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
    this._menu = new mozelement.Elem(node);
  }
  else {
    throw new Error("Menu element '" + menuSelector + "' not found.");
  }
}

Menu.prototype = {

  






  open : function(contextElement) {
    
    var menu = this._menu.getNode();
    if ((menu.localName == "popup" || menu.localName == "menupopup") &&
        contextElement && contextElement.exists()) {
      this._controller.rightClick(contextElement);
      this._controller.waitFor(function() {
        return menu.state == "open";
      }, "Context menu has been opened.");
    }

    
    this._buildMenu(menu);

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

    return new mozelement.Elem(node);
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
            var popupEvent = this._controller.window.document.createEvent("MouseEvent");
            popupEvent.initMouseEvent("popupshowing", true, true, this._controller.window, 0,
                                             0, 0, 0, 0, false, false, false, false, 0, null);
            popup.dispatchEvent(popupEvent);
          }
          this._buildMenu(popup);
        }
      }
    }, this);
  }
};

var MozMillController = function (window) {
  this.window = window;

  this.mozmillModule = {};
  Components.utils.import('resource://mozmill/driver/mozmill.js', this.mozmillModule);

  utils.waitFor(function() {
    return window != null && this.isLoaded();
  }, "controller(): Window could not be initialized.", undefined, undefined, this);

  if ( controllerAdditions[window.document.documentElement.getAttribute('windowtype')] != undefined ) {
    this.prototype = new utils.Copy(this.prototype);
    controllerAdditions[window.document.documentElement.getAttribute('windowtype')](this);
    this.windowtype = window.document.documentElement.getAttribute('windowtype');
  }
}


MozMillController.prototype.__defineGetter__("windowElement", function() {
  if (this._windowElement == undefined) 
    this._windowElement = new mozelement.MozMillElement(undefined, undefined, {'element': this.window});
  return this._windowElement;
});

MozMillController.prototype.sleep = utils.sleep;


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

  broker.pass({'function':'Controller.open()'});
}













MozMillController.prototype.screenShot = function _screenShot(node, name, save, highlights) {
  if (!node) {
    throw new Error("node is undefined");
  }
  
  
  if ("getNode" in node) node = node.getNode();
  if (highlights) {
    for (var i = 0; i < highlights.length; ++i) {
      if ("getNode" in highlights[i]) {
        highlights[i] = highlights[i].getNode();
      }
    }
  }
  
  
  
  var filepath, dataURL;
  try {
    if (save) {
      filepath = utils.takeScreenshot(node, name, highlights);
    } else {
      dataURL = utils.takeScreenshot(node, undefined, highlights);
    }
  } catch (e) {
    throw new Error("controller.screenShot() failed: " + e);
  }

  
  var d = new Date();
  
  var obj = { "filepath": filepath,
              "dataURL": dataURL,
              "name": name,
              "timestamp": d.toLocaleString(),
            }
  
  broker.sendMessage("screenShot", obj);
  broker.pass({'function':'controller.screenShot()'});
}






MozMillController.prototype.isLoaded = function(window) {
  var win = window || this.window;

  return ("mozmillDocumentLoaded" in win) && win.mozmillDocumentLoaded;
};

MozMillController.prototype.waitFor = function(callback, message, timeout,
                                               interval, thisObject) {
  utils.waitFor(callback, message, timeout, interval, thisObject);

  broker.pass({'function':'controller.waitFor()'});
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
        throw('controller.menus - DEPRECATED Use controller.mainMenu instead.');

});

MozMillController.prototype.waitForImage = function (elem, timeout, interval) {
  this.waitFor(function() {
    return elem.getNode().complete == true;
  }, "timeout exceeded for waitForImage " + elem.getInfo(), timeout, interval);

  broker.pass({'function':'Controller.waitForImage()'});
}

MozMillController.prototype.startUserShutdown = function (timeout, restart, next, resetProfile) {
  if (restart && resetProfile) {
      throw new Error("You can't have a user-restart and reset the profile; there is a race condition");
  }
  broker.sendMessage('userShutdown', {'user': true,
                                  'restart': Boolean(restart),
                                  'next': next,
                                  'resetProfile': Boolean(resetProfile)});
  this.window.setTimeout(broker.sendMessage, timeout, 'userShutdown', 0);
}

MozMillController.prototype.restartApplication = function (next, resetProfile) 
{
  
  
  
  broker.sendMessage('userShutdown', {'user': false,
                                  'restart': true,
                                  'next': next,
                                  'resetProfile': Boolean(resetProfile)});
  broker.sendMessage('endTest');
  broker.sendMessage('persist');
  utils.getMethodInWindows('goQuitApplication')();
}

MozMillController.prototype.stopApplication = function (resetProfile) 
{
  
  
  broker.sendMessage('userShutdown', {'user': false,
                                  'restart': false,
                                  'resetProfile': Boolean(resetProfile)});
  broker.sendMessage('endTest');
  broker.sendMessage('persist');
  utils.getMethodInWindows('goQuitApplication')();
}


MozMillController.prototype.goBack = function(){
  this.window.content.history.back();
  broker.pass({'function':'Controller.goBack()'});
  return true;
}
MozMillController.prototype.goForward = function(){
  this.window.content.history.forward();
  broker.pass({'function':'Controller.goForward()'});
  return true;
}
MozMillController.prototype.refresh = function(){
  this.window.content.location.reload(true);
  broker.pass({'function':'Controller.refresh()'});
  return true;
}

function logDeprecated(funcName, message) {
   broker.log({'function': funcName + '() - DEPRECATED', 'message': funcName + '() is deprecated' + message});
}

function logDeprecatedAssert(funcName) {
   logDeprecated('controller.' + funcName, '. use the generic `assert` module instead');
}

MozMillController.prototype.assertText = function (el, text) {
  logDeprecatedAssert("assertText");
  
  var n = el.getNode();

  if (n && n.innerHTML == text){
    broker.pass({'function':'Controller.assertText()'});
    return true;
   }

  throw new Error("could not validate element " + el.getInfo()+" with text "+ text);
  return false;

};


MozMillController.prototype.assertNode = function (el) {
  logDeprecatedAssert("assertNode");
  
  
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  broker.pass({'function':'Controller.assertNode()'});
  return true;
};


MozMillController.prototype.assertNodeNotExist = function (el) {
  logDeprecatedAssert("assertNodeNotExist");
  
  
  try {
    var element = el.getNode();
  } catch(err){
    broker.pass({'function':'Controller.assertNodeNotExist()'});
    return true;
  }

  if (element) {
    throw new Error("Unexpectedly found element " + el.getInfo());
    return false;
  } else {
    broker.pass({'function':'Controller.assertNodeNotExist()'});
    return true;
  }
};


MozMillController.prototype.assertValue = function (el, value) {
  logDeprecatedAssert("assertValue");
  
  
  var n = el.getNode();

  if (n && n.value == value){
    broker.pass({'function':'Controller.assertValue()'});
    return true;
  }
  throw new Error("could not validate element " + el.getInfo()+" with value "+ value);
  return false;
};




MozMillController.prototype.assert = function(callback, message, thisObject)
{
  logDeprecatedAssert("assert");
  utils.assert(callback, message, thisObject);

  broker.pass({'function': ": controller.assert('" + callback + "')"});
  return true;
}


MozMillController.prototype.assertSelected = function (el, value) {
  logDeprecatedAssert("assertSelected");
  
  
  var n = el.getNode();
  var validator = value;

  if (n && n.options[n.selectedIndex].value == validator){
    broker.pass({'function':'Controller.assertSelected()'});
    return true;
    }
  throw new Error("could not assert value for element " + el.getInfo()+" with value "+ value);
  return false;
};


MozMillController.prototype.assertChecked = function (el) {
  logDeprecatedAssert("assertChecked");
  
  
  var element = el.getNode();

  if (element && element.checked == true){
    broker.pass({'function':'Controller.assertChecked()'});
    return true;
    }
  throw new Error("assert failed for checked element " + el.getInfo());
  return false;
};


MozMillController.prototype.assertNotChecked = function (el) {
  logDeprecatedAssert("assertNotChecked");
  
  var element = el.getNode();

  if (!element) {
    throw new Error("Could not find element" + el.getInfo());
  }

  if (!element.hasAttribute("checked") || element.checked != true){
    broker.pass({'function':'Controller.assertNotChecked()'});
    return true;
    }
  throw new Error("assert failed for not checked element " + el.getInfo());
  return false;
};







MozMillController.prototype.assertJSProperty = function(el, attrib, val) {
  logDeprecatedAssert("assertJSProperty");
  
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value = element[attrib];
  var res = (value !== undefined && (val === undefined ? true : String(value) == String(val)));
  if (res) {
    broker.pass({'function':'Controller.assertJSProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertJSProperty(" + el.getInfo() + ") : " + 
                     (val === undefined ? "property '" + attrib + "' doesn't exist" : val + " == " + value));
  }
  return res;
};







MozMillController.prototype.assertNotJSProperty = function(el, attrib, val) {
  logDeprecatedAssert("assertNotJSProperty");
  
  var element = el.getNode();
  if (!element){
    throw new Error("could not find element " + el.getInfo());
    return false;
  }
  var value = element[attrib];
  var res = (val === undefined ? value === undefined : String(value) != String(val));
  if (res) {
    broker.pass({'function':'Controller.assertNotProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertNotJSProperty(" + el.getInfo() + ") : " +
                     (val === undefined ? "property '" + attrib + "' exists" : val + " != " + value));
  }
  return res;
};







MozMillController.prototype.assertDOMProperty = function(el, attrib, val) {
  logDeprecatedAssert("assertDOMProperty");
  
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
    broker.pass({'function':'Controller.assertDOMProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertDOMProperty(" + el.getInfo() + ") : " + 
                     (val === undefined ? "property '" + attrib + "' doesn't exist" : val + " == " + value));
  }
  return res;
};







MozMillController.prototype.assertNotDOMProperty = function(el, attrib, val) {
  logDeprecatedAssert("assertNotDOMProperty");
  
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
    broker.pass({'function':'Controller.assertNotDOMProperty("' + el.getInfo() + '") : ' + val});
  } else {
    throw new Error("Controller.assertNotDOMProperty(" + el.getInfo() + ") : " + 
                     (val == undefined ? "property '" + attrib + "' exists" : val + " == " + value));
  }
  return !res;
};


MozMillController.prototype.assertProperty = function(el, attrib, val) {
  logDeprecatedAssert("assertProperty");
  return this.assertJSProperty(el, attrib, val);
};


MozMillController.prototype.assertPropertyNotExist = function(el, attrib) {
  logDeprecatedAssert("assertPropertyNotExist");
  return this.assertNotJSProperty(el, attrib);
};




MozMillController.prototype.assertImageLoaded = function (el) {
  logDeprecatedAssert("assertImageLoaded");
  
  
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
    broker.pass({'function':'Controller.assertImageLoaded'});
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

  var triggerMouseEvent = function(element, clientX, clientY) {
    clientX = clientX ? clientX: 0;
    clientY = clientY ? clientY: 0;

    
    var screenX = element.boxObject.screenX ? element.boxObject.screenX : 0;
    var screenY = element.boxObject.screenY ? element.boxObject.screenY : 0;

    var evt = element.ownerDocument.createEvent('MouseEvents');
    if (evt.initMouseEvent) {
      evt.initMouseEvent('mousemove', true, true, element.ownerDocument.defaultView, 1, screenX, screenY, clientX, clientY)
    }
    else {
      
      evt.initEvent('mousemove', true, true);
    }
    element.dispatchEvent(evt);
  };

  
  triggerMouseEvent(doc.body, start[0], start[1]);
  triggerMouseEvent(doc.body, dest[0], dest[1]);
  broker.pass({'function':'Controller.mouseMove()'});
  return true;
}



MozMillController.prototype.dragToElement = function(src, dest, offsetX,
    offsetY, aWindow, dropEffect, dragData) {
  srcElement = src.getNode();
  destElement = dest.getNode();
  aWindow = aWindow || srcElement.ownerDocument.defaultView;
  offsetX = offsetX || 20;
  offsetY = offsetY || 20;

  var dataTransfer;

  var trapDrag = function(event) {
    dataTransfer = event.dataTransfer;
    if(!dragData)
      return;

    for (var i = 0; i < dragData.length; i++) {
      var item = dragData[i];
      for (var j = 0; j < item.length; j++) {
        dataTransfer.mozSetDataAt(item[j].type, item[j].data, i);
      }
    }
    dataTransfer.dropEffect = dropEffect || "move";
    event.preventDefault();
    event.stopPropagation();
  }

  aWindow.addEventListener("dragstart", trapDrag, true);
  EventUtils.synthesizeMouse(srcElement, 2, 2, { type: "mousedown" }, aWindow); 
  EventUtils.synthesizeMouse(srcElement, 11, 11, { type: "mousemove" }, aWindow);
  EventUtils.synthesizeMouse(srcElement, offsetX, offsetY, { type: "mousemove" }, aWindow);
  aWindow.removeEventListener("dragstart", trapDrag, true);

  var event = aWindow.document.createEvent("DragEvents");
  event.initDragEvent("dragenter", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  destElement.dispatchEvent(event);

  var event = aWindow.document.createEvent("DragEvents");
  event.initDragEvent("dragover", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  if (destElement.dispatchEvent(event)) {
    EventUtils.synthesizeMouse(destElement, offsetX, offsetY, { type: "mouseup" }, aWindow);
    return "none";
  }

  event = aWindow.document.createEvent("DragEvents");
  event.initDragEvent("drop", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  destElement.dispatchEvent(event);
  EventUtils.synthesizeMouse(destElement, offsetX, offsetY, { type: "mouseup" }, aWindow);

  return dataTransfer.dropEffect;
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

  controller.waitForPageLoad = function(aDocument, aTimeout, aInterval) {
    var timeout = aTimeout || 30000;
    var win = null;

    
    
    if (typeof(aDocument) == "number"){
      timeout = aDocument;
    }

    
    if (aDocument && (typeof(aDocument) === "object") &&
        "defaultView" in aDocument)
      win = aDocument.defaultView;

    
    
    win = win || this.window.gBrowser.selectedBrowser.contentWindow;

    
    this.waitFor(function () {
        var loaded = this.isLoaded(win);
        var firstRun = !('mozmillWaitForPageLoad' in win);
        var ret = firstRun && loaded;
        if (ret) {
          win.mozmillWaitForPageLoad = true;
        }
        return ret;
    }, "controller.waitForPageLoad(): Timeout waiting for page loaded.",
        timeout, aInterval, this);

    broker.pass({'function':'controller.waitForPageLoad()'});
  }
}

controllerAdditions = {
  'Browser:Preferences':preferencesAdditions,
  'navigator:browser'  :browserAdditions,
}







MozMillController.prototype.select = function (elem, index, option, value) {
  return elem.select(index, option, value); 
};

MozMillController.prototype.keypress = function(aTarget, aKey, aModifiers, aExpectedEvent) {
  if (aTarget == null) { aTarget = this.windowElement; }
  return aTarget.keypress(aKey, aModifiers, aExpectedEvent);
}

MozMillController.prototype.type = function (aTarget, aText, aExpectedEvent) {
  if (aTarget == null) { aTarget = this.windowElement; }

  var that = this;
  var retval = true;
  Array.forEach(aText, function(letter) {
    if (!that.keypress(aTarget, letter, {}, aExpectedEvent)) {
      retval = false; }
  });

  return retval;
}

MozMillController.prototype.mouseEvent = function(aTarget, aOffsetX, aOffsetY, aEvent, aExpectedEvent) {
  return aTarget.mouseEvent(aOffsetX, aOffsetY, aEvent, aExpectedEvent);
}

MozMillController.prototype.click = function(elem, left, top, expectedEvent) {
  return elem.click(left, top, expectedEvent);
}

MozMillController.prototype.doubleClick = function(elem, left, top, expectedEvent) {
  return elem.doubleClick(left, top, expectedEvent);
}

MozMillController.prototype.mouseDown = function (elem, button, left, top, expectedEvent) {
  return elem.mouseDown(button, left, top, expectedEvent);
};

MozMillController.prototype.mouseOut = function (elem, button, left, top, expectedEvent) {
  return elem.mouseOut(button, left, top, expectedEvent);
};

MozMillController.prototype.mouseOver = function (elem, button, left, top, expectedEvent) {
  return elem.mouseOver(button, left, top, expectedEvent);
};

MozMillController.prototype.mouseUp = function (elem, button, left, top, expectedEvent) {
  return elem.mouseUp(button, left, top, expectedEvent);
};

MozMillController.prototype.middleClick = function(elem, left, top, expectedEvent) {
  return elem.middleClick(elem, left, top, expectedEvent);
}

MozMillController.prototype.rightClick = function(elem, left, top, expectedEvent) {
  return elem.rightClick(left, top, expectedEvent);
}

MozMillController.prototype.check = function(elem, state) {
  return elem.check(state);
}

MozMillController.prototype.radio = function(elem) {
  return elem.select();
}

MozMillController.prototype.waitThenClick = function (elem, timeout, interval) {
  return elem.waitThenClick(timeout, interval);
}

MozMillController.prototype.waitForElement = function(elem, timeout, interval) {
  return elem.waitForElement(timeout, interval);
}

MozMillController.prototype.waitForElementNotPresent = function(elem, timeout, interval) {
  return elem.waitForElementNotPresent(timeout, interval);
}

