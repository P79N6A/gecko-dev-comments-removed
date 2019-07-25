






































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
      }, timeout, interval, "Timeout happened before event '" + ex +"' was fired.");
  
      this.node.removeEventListener(e, this.registry[e], true);
    }
  }
}









var Menu = function (aWindow, aElements) {
  for each (var node in aElements) {
    var entry = null;

    switch (node.tagName) {
      case "menu":
        var popup = node.getElementsByTagName("menupopup")[0];

        
        if (popup) {
          if (popup.allowevents) {
            events.fakeOpenPopup(aWindow, popup);
          }
          entry = new Menu(aWindow, popup.childNodes);
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
  }, 5000, 100, "controller(): Window could not be initialized.");

  if ( controllerAdditions[window.document.documentElement.getAttribute('windowtype')] != undefined ) {
    this.prototype = new utils.Copy(this.prototype);
    controllerAdditions[window.document.documentElement.getAttribute('windowtype')](this);
    this.windowtype = window.document.documentElement.getAttribute('windowtype');
  }
}

MozMillController.prototype.sleep = utils.sleep;

MozMillController.prototype.keypress = function(el, aKey, modifiers) {
  var element = (el == null) ? this.window : el.getNode();
  if (!element) {
    throw new Error("could not find element " + el.getInfo());
    return false;
  }

  events.triggerKeyEvent(element, 'keypress', aKey, modifiers);

  frame.events.pass({'function':'Controller.keypress()'});
  return true;
}

MozMillController.prototype.type = function (el, text) {
  var element = (el == null) ? this.window : el.getNode();
  if (!element) {
    throw new Error("could not find element " + el.getInfo());
    return false;
  }

  for (var indx = 0; indx < text.length; indx++) {
    events.triggerKeyEvent(element, 'keypress', text.charAt(indx), {});
  }

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

MozMillController.prototype.click = function(elem, left, top)
{
  var element = elem.getNode();
  if (!element){
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (element.tagName == "menuitem") {
    element.click();
    return true;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {}, element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.click()'});
  return true;
}

MozMillController.prototype.doubleClick = function(elem, left, top)
{
  var element = elem.getNode();
  if (!element) {
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {clickCount: 2},
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.doubleClick()'});
  return true;
}

MozMillController.prototype.mouseDown = function (elem, button, left, top)
{
  var element = elem.getNode();
  if (!element) {
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {button: button, type: "mousedown"},
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.mouseDown()'});
  return true;
};

MozMillController.prototype.mouseOut = function (elem, button, left, top)
{
  var element = elem.getNode();
  if (!element) {
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {button: button, type: "mouseout"},
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.mouseOut()'});
  return true;
};

MozMillController.prototype.mouseOver = function (elem, button, left, top)
{
  var element = elem.getNode();
  if (!element) {
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {button: button, type: "mouseover"},
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.mouseOver()'});
  return true;
};

MozMillController.prototype.mouseUp = function (elem, button, left, top)
{
  var element = elem.getNode();
  if (!element) {
    throw new Error("could not find element " + elem.getInfo());
    return false;
  }

  if (isNaN(left)) { left = 1; }
  if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top, {button: button, type: "mouseup"},
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.mouseUp()'});
  return true;
};

MozMillController.prototype.middleClick = function(elem, left, top)
{
  var element = elem.getNode();
    if (!element){
      throw new Error("could not find element " + el.getInfo());
      return false;
    }

    if (isNaN(left)) { left = 1; }
    if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top,
                             { button: 1 },
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.middleClick()'});
    return true;
}

MozMillController.prototype.rightClick = function(elem, left, top)
{
  var element = elem.getNode();
    if (!element){
      throw new Error("could not find element " + el.getInfo());
      return false;
    }

    if (isNaN(left)) { left = 1; }
    if (isNaN(top)) { top = 1; }

  
  if (element.scrollIntoView)
    element.scrollIntoView();

  EventUtils.synthesizeMouse(element, left, top,
                             { type : "contextmenu", button: 2 },
                             element.ownerDocument.defaultView);
  this.sleep(0);

  frame.events.pass({'function':'Controller.rightClick()'});
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
    }, 500, 100, "Checkbox " + el.getInfo() + " could not be checked/unchecked");

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
  }, 500, 100, "Radio button " + el.getInfo() + " could not be selected");

  frame.events.pass({'function':'Controller.radio(' + el.getInfo() + ')'});
  return true;
}

MozMillController.prototype.waitFor = function(callback, timeout, interval, message) {
  utils.waitFor(callback, timeout, interval, message);

  frame.events.pass({'function':'controller.waitFor()'});
}

MozMillController.prototype.waitForEval = function(expression, timeout, interval, subject) {
  waitFor(function() {
    return eval(expression);
  }, timeout, interval, "controller.waitForEval: Timeout exceeded for '" + expression + "'");

  frame.events.pass({'function':'controller.waitForEval()'});
}

MozMillController.prototype.waitForElement = function(elem, timeout, interval) {
  this.waitFor(function() {
    return elem.exists();
  }, timeout, interval, "Timeout exceeded for waitForElement " + elem.getInfo());

  frame.events.pass({'function':'Controller.waitForElement()'});
}

MozMillController.prototype.__defineGetter__("waitForEvents", function() {
  if (this._waitForEvents == undefined)
    this._waitForEvents = new waitForEvents();
  return this._waitForEvents;
});

MozMillController.prototype.__defineGetter__("menus", function() {
  var menu = null;

  var menubar = this.window.document.getElementsByTagName('menubar');
  if(menubar && menubar.length > 0) 
    menu = new Menu(this.window, menubar[0].childNodes);

  return menu;
});

MozMillController.prototype.waitForImage = function (elem, timeout, interval) {
  this.waitFor(function() {
    return elem.getNode().complete == true;
  }, timeout, interval, "timeout exceeded for waitForImage " + elem.getInfo());

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




MozMillController.prototype.assert = function(callback, message)
{
  utils.assert(callback, message);

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
    var self = {loaded: false};
    var tab = null;

    
    
    if (typeof(aTabDocument) == "number"){
      aTimeout = aTabDocument;
    }

    
    if (typeof(aTabDocument) == "object") {
      for each (var browser in this.window.gBrowser) {
        if (browser && browser.contentDocument === aTabDocument) {
          tab = browser;
          break;
        }
      }
    }

    
    tab = tab || this.window.gBrowser.selectedBrowser;

    
    
    
    if (tab.contentDocument.defaultView.documentLoaded) {
      frame.events.pass({'function':'controller.waitForPageLoad()'});
      return;
    }

    
    
    
    function pageLoaded() {
      tab.removeEventListener("DOMContentLoaded", pageLoaded, true);
      self.loaded = true;
    }
    tab.addEventListener("DOMContentLoaded", pageLoaded, true);

    try {
      
      this.waitFor(function() { return self.loaded; }, aTimeout, aInterval);
      this.sleep(1000);

      frame.events.pass({'function':'controller.waitForPageLoad()'});
    } catch (ex) {
      
      tab.removeEventListener("DOMContentLoaded", pageLoaded, true);

      throw new Error("controller.waitForPageLoad(): Timeout waiting for page loaded.");
    }
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
  }, 500, 100, "MozMillAsyncTest timed out. Done is " + this._done); 

  return true;
}

MozMillAsyncTest.prototype.finish = function () {
  this._done = true;
}
