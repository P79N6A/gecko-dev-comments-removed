var gMenuPopup = null;
var gTrigger = null;
var gIsMenu = false;
var gScreenX = -1, gScreenY = -1;
var gCachedEvent = null;

function runTests()
{
  if (screen.height < 768) {
    ok(false, "popup tests are likely to fail for screen heights less than 768 pixels");
  }

  gMenuPopup = document.getElementById("thepopup");
  gTrigger = document.getElementById("trigger");

  gIsMenu = gTrigger.boxObject instanceof Components.interfaces.nsIMenuBoxObject;

  var mouseFn = function(event) {
    gScreenX = event.screenX;
    gScreenY = event.screenY;
    
    gCachedEvent = event;
  }

  
  window.addEventListener("mousedown", mouseFn, false);
  synthesizeMouse(document.documentElement, 0, 0, { });
  window.removeEventListener("mousedown", mouseFn, false);
  startPopupTests(popupTests);
}

var popupTests = [
{
  testname: "mouse click on trigger",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function() {
    
    
    gExpectedTriggerNode = gIsMenu ? "notset" : gTrigger;
    synthesizeMouse(gTrigger, 4, 4, { });
  },
  result: function (testname) {
    gExpectedTriggerNode = null;
    is(gMenuPopup.triggerNode, gIsMenu ? null : gTrigger, testname + " triggerNode");
    is(document.popupNode, gIsMenu ? null : gTrigger, testname + " document.popupNode");
    is(document.tooltipNode, null, testname + " document.tooltipNode");
    
    if (window.opener)
      is(window.opener.document.popupNode, null, testname + " opener.document.popupNode");

    checkActive(gMenuPopup, "", testname);
    checkOpen("trigger", testname);
    
    
    if (gIsMenu)
      compareEdge(gTrigger, gMenuPopup, "after_start", 0, 0, testname);
  }
},
{
  
  
  testname: "cursor down no selection",
  events: [ "DOMMenuItemActive item1" ],
  test: function() { synthesizeKey("VK_DOWN", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item1", testname); }
},
{
  
  testname: "cursor up wrap",
  events: [ "DOMMenuItemInactive item1", "DOMMenuItemActive last" ],
  test: function() { synthesizeKey("VK_UP", { }); },
  result: function(testname) {
    checkActive(gMenuPopup, "last", testname);
  }
},
{
  
  testname: "cursor down wrap",
  events: [ "DOMMenuItemInactive last", "DOMMenuItemActive item1" ],
  test: function() { synthesizeKey("VK_DOWN", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item1", testname); }
},
{
  
  testname: "cursor down",
  events: [ "DOMMenuItemInactive item1", "DOMMenuItemActive item2" ],
  test: function() { synthesizeKey("VK_DOWN", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item2", testname); }
},
{
  
  testname: "cursor up",
  events: [ "DOMMenuItemInactive item2", "DOMMenuItemActive item1" ],
  test: function() { synthesizeKey("VK_UP", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item1", testname); }
},
{
  
  testname: "cursor left",
  test: function() { synthesizeKey("VK_LEFT", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item1", testname); }
},
{
  
  testname: "cursor right",
  test: function() { synthesizeKey("VK_RIGHT", { }); },
  result: function(testname) { checkActive(gMenuPopup, "item1", testname); }
},
{
  
  testname: "cursor down disabled",
  events: function() {
    
    
    if (navigator.platform.indexOf("Win") == 0)
      return [ "DOMMenuItemInactive item1", "DOMMenuItemActive item2" ];
    else
      return [ "DOMMenuItemInactive item1", "DOMMenuItemActive amenu" ];
  },
  test: function() {
    document.getElementById("item2").disabled = true;
    synthesizeKey("VK_DOWN", { });
  }
},
{
  
  testname: "cursor up disabled",
  events: function() {
    if (navigator.platform.indexOf("Win") == 0)
      return [ "DOMMenuItemInactive item2", "DOMMenuItemActive amenu",
               "DOMMenuItemInactive amenu", "DOMMenuItemActive item2",
               "DOMMenuItemInactive item2", "DOMMenuItemActive item1" ];
    else
      return [ "DOMMenuItemInactive amenu", "DOMMenuItemActive item1" ];
  },
  test: function() {
    if (navigator.platform.indexOf("Win") == 0)
      synthesizeKey("VK_DOWN", { });
    synthesizeKey("VK_UP", { });
    if (navigator.platform.indexOf("Win") == 0)
      synthesizeKey("VK_UP", { });
  }
},
{
  testname: "mouse click outside",
  events: [ "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuItemInactive item1", "DOMMenuInactive thepopup" ],
  test: function() {
    gMenuPopup.hidePopup();
    
    
    
    
  },
  result: function(testname, step) {
    is(gMenuPopup.triggerNode, null, testname + " triggerNode");
    is(document.popupNode, null, testname + " document.popupNode");
    checkClosed("trigger", testname);
  }
},
{
  
  
  testname: "open popup anchored",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  steps: ["before_start", "before_end", "after_start", "after_end",
          "start_before", "start_after", "end_before", "end_after", "after_pointer", "overlap"],
  test: function(testname, step) {
    gExpectedTriggerNode = "notset";
    gMenuPopup.openPopup(gTrigger, step, 0, 0, false, false);
  },
  result: function(testname, step) {
    
    gExpectedTriggerNode = null;
    is(gMenuPopup.triggerNode, null, testname + " triggerNode");
    is(document.popupNode, null, testname + " document.popupNode");
    compareEdge(gTrigger, gMenuPopup, step, 0, 0, testname);
  }
},
{
  
  testname: "open popup anchored with margin",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  steps: ["before_start", "before_end", "after_start", "after_end",
          "start_before", "start_after", "end_before", "end_after", "after_pointer", "overlap"],
  test: function(testname, step) {
    gMenuPopup.setAttribute("style", "margin: 10px;");
    gMenuPopup.openPopup(gTrigger, step, 0, 0, false, false);
  },
  result: function(testname, step) {
    var rightmod = step == "before_end" || step == "after_end" ||
                   step == "start_before" || step == "start_after";
    var bottommod = step == "before_start" || step == "before_end" ||
                    step == "start_after" || step == "end_after";
    compareEdge(gTrigger, gMenuPopup, step, rightmod ? -10 : 10, bottommod ? -10 : 10, testname);
    gMenuPopup.removeAttribute("style");
  }
},
{
  
  testname: "open popup anchored with negative margin",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  steps: ["before_start", "before_end", "after_start", "after_end",
          "start_before", "start_after", "end_before", "end_after", "after_pointer", "overlap"],
  test: function(testname, step) {
    gMenuPopup.setAttribute("style", "margin: -8px;");
    gMenuPopup.openPopup(gTrigger, step, 0, 0, false, false);
  },
  result: function(testname, step) {
    var rightmod = step == "before_end" || step == "after_end" ||
                   step == "start_before" || step == "start_after";
    var bottommod = step == "before_start" || step == "before_end" ||
                    step == "start_after" || step == "end_after";
    compareEdge(gTrigger, gMenuPopup, step, rightmod ? 8 : -8, bottommod ? 8 : -8, testname);
    gMenuPopup.removeAttribute("style");
  }
},
{
  
  
  testname: "open popup anchored with attribute",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  steps: ["before_start", "before_end", "after_start", "after_end",
          "start_before", "start_after", "end_before", "end_after", "after_pointer", "overlap"],
  test: function(testname, step) {
    gMenuPopup.setAttribute("position", step);
    gMenuPopup.openPopup(gTrigger, "", 0, 0, false, false);
  },
  result: function(testname, step) { compareEdge(gTrigger, gMenuPopup, step, 0, 0, testname); }
},
{
  
  
  
  testname: "open popup anchored with override",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) {
    
    gMenuPopup.setAttribute("position", "end_after");
    gExpectedTriggerNode = gCachedEvent.target;
    gMenuPopup.openPopup(gTrigger, "before_start", 0, 0, false, true, gCachedEvent);
  },
  result: function(testname, step) {
    gExpectedTriggerNode = null;
    is(gMenuPopup.triggerNode, gCachedEvent.target, testname + " triggerNode");
    is(document.popupNode, gCachedEvent.target, testname + " document.popupNode");
    compareEdge(gTrigger, gMenuPopup, "end_after", 0, 0, testname);
  }
},
{
  testname: "close popup with escape",
  events: [ "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuInactive thepopup", ],
  test: function(testname, step) {
    synthesizeKey("VK_ESCAPE", { });
    checkClosed("trigger", testname);
  }
},
{
  
  testname: "open popup anchored with offsets",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    
    gMenuPopup.setAttribute("position", "");
    gMenuPopup.openPopup(gTrigger, "before_start", 5, 10, true, true);
  },
  result: function(testname, step) { compareEdge(gTrigger, gMenuPopup, "before_start", 5, 10, testname); }
},
{
  
  
  testname: "show popup anchored",
  condition: function() {
    
    
    
    return !gIsMenu;
  },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  steps: [["topleft", "topleft"],
          ["topleft", "topright"], ["topleft", "bottomleft"],
          ["topright", "topleft"], ["topright", "bottomright"],
          ["bottomleft", "bottomright"], ["bottomleft", "topleft"],
          ["bottomright", "bottomleft"], ["bottomright", "topright"]],
  test: function(testname, step) {
    
    gMenuPopup.setAttribute("popupanchor", "topright");
    gMenuPopup.setAttribute("popupalign", "bottomright");
    gMenuPopup.setAttribute("position", "end_after");
    gMenuPopup.showPopup(gTrigger, -1, -1, "popup", step[0], step[1]);
  },
  result: function(testname, step) {
    var pos = convertPosition(step[0], step[1]);
    compareEdge(gTrigger, gMenuPopup, pos, 0, 0, testname);
    gMenuPopup.removeAttribute("popupanchor");
    gMenuPopup.removeAttribute("popupalign");
    gMenuPopup.removeAttribute("position");
  }
},
{
  testname: "show popup with position",
  condition: function() { return !gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    gMenuPopup.showPopup(gTrigger, gScreenX + 60, gScreenY + 15,
                         "context", "topleft", "bottomright");
  },
  result: function(testname, step) {
    var rect = gMenuPopup.getBoundingClientRect();
    ok(true, gScreenX + "," + gScreenY);
    is(rect.left, 60, testname + " left");
    is(rect.top, 15, testname + " top");
    ok(rect.right, testname + " right is " + rect.right);
    ok(rect.bottom, testname + " bottom is " + rect.bottom);
  }
},
{
  
  
  testname: "open popup unanchored",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) { gMenuPopup.openPopup(null, "after_start", 6, 8, false); },
  result: function(testname, step) {
    var rect = gMenuPopup.getBoundingClientRect();
    ok(rect.left == 6 && rect.top == 8 && rect.right && rect.bottom, testname);
  }
},
{
  testname: "activate menuitem with mouse",
  events: [ "DOMMenuInactive thepopup", "command item3",
            "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuItemInactive item3" ],
  test: function(testname, step) {
    var item3 = document.getElementById("item3");
    synthesizeMouse(item3, 4, 4, { });
  },
  result: function(testname, step) { checkClosed("trigger", testname); }
},
{
  testname: "close popup",
  condition: function() { return false; },
  events: [ "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuInactive thepopup" ],
  test: function(testname, step) { gMenuPopup.hidePopup(); }
},
{
  testname: "open popup at screen",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) {
    gExpectedTriggerNode = "notset";
    gMenuPopup.openPopupAtScreen(gScreenX + 24, gScreenY + 20, false);
  },
  result: function(testname, step) {
    gExpectedTriggerNode = null;
    is(gMenuPopup.triggerNode, null, testname + " triggerNode");
    is(document.popupNode, null, testname + " document.popupNode");
    var rect = gMenuPopup.getBoundingClientRect();
    is(rect.left, 24, testname + " left");
    is(rect.top, 20, testname + " top");
    ok(rect.right, testname + " right is " + rect.right);
    ok(rect.bottom, testname + " bottom is " + rect.bottom);
  }
},
{
  
  
  
  testname: "menuitem accelerator",
  events: [ "DOMMenuItemActive amenu", "DOMMenuItemInactive amenu",
            "DOMMenuInactive thepopup",
            "command amenu", "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuItemInactive amenu"
           ],
  test: function() { synthesizeKey("M", { }); },
  result: function(testname) { checkClosed("trigger", testname); }
},
{
  testname: "open context popup at screen",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) {
    gExpectedTriggerNode = gCachedEvent.target;
    gMenuPopup.openPopupAtScreen(gScreenX + 8, gScreenY + 16, true, gCachedEvent);
  },
  result: function(testname, step) {
    gExpectedTriggerNode = null;
    is(gMenuPopup.triggerNode, gCachedEvent.target, testname + " triggerNode");
    is(document.popupNode, gCachedEvent.target, testname + " document.popupNode");

    var childframe = document.getElementById("childframe");
    if (childframe) {
      for (var t = 0; t < 2; t++) {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        var child = childframe.contentDocument; 
        var evt = child.createEvent("Event");
        evt.initEvent("click", true, true);
        child.documentElement.dispatchEvent(evt);
        is(child.documentElement.getAttribute("data"), "xnull",
           "cannot get popupNode from other document");
        child.documentElement.setAttribute("data", "none");
        
        document.popupNode = gCachedEvent.target;
      }
    }

    var rect = gMenuPopup.getBoundingClientRect();
    is(rect.left, 10, testname + " left");
    is(rect.top, 18, testname + " top");
    ok(rect.right, testname + " right is " + rect.right);
    ok(rect.bottom, testname + " bottom is " + rect.bottom);
  }
},
{
  
  
  
  testname: "menuitem with non accelerator",
  events: [ "DOMMenuItemActive one" ],
  test: function() { synthesizeKey("O", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkActive(gMenuPopup, "one", testname);
  }
},
{
  
  
  testname: "menuitem with non accelerator again",
  events: [ "DOMMenuItemInactive one", "DOMMenuItemActive submenu" ],
  test: function() { synthesizeKey("O", { }); },
  result: function(testname) {
    
    checkOpen("trigger", testname);
    checkClosed("submenu", testname);
    checkActive(gMenuPopup, "submenu", testname);
  }
},
{
  
  testname: "open submenu with cursor right",
  events: [ "popupshowing submenupopup", "DOMMenuItemActive submenuitem",
            "popupshown submenupopup" ],
  test: function() { synthesizeKey("VK_RIGHT", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkOpen("submenu", testname);
    checkActive(gMenuPopup, "submenu", testname);
    checkActive(document.getElementById("submenupopup"), "submenuitem", testname);
  }
},
{
  
  testname: "close submenu with cursor left",
  events: [ "popuphiding submenupopup", "popuphidden submenupopup",
            "DOMMenuItemInactive submenuitem", "DOMMenuInactive submenupopup",
            "DOMMenuItemActive submenu" ],
  test: function() { synthesizeKey("VK_LEFT", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkClosed("submenu", testname);
    checkActive(gMenuPopup, "submenu", testname);
    checkActive(document.getElementById("submenupopup"), "", testname);
  }
},
{
  
  testname: "open submenu with enter",
  events: [ "popupshowing submenupopup", "DOMMenuItemActive submenuitem",
            "popupshown submenupopup" ],
  test: function() { synthesizeKey("VK_ENTER", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkOpen("submenu", testname);
    checkActive(gMenuPopup, "submenu", testname);
    checkActive(document.getElementById("submenupopup"), "submenuitem", testname);
  }
},
{
  
  testname: "close submenu with escape",
  events: [ "popuphiding submenupopup", "popuphidden submenupopup",
            "DOMMenuItemInactive submenuitem", "DOMMenuInactive submenupopup",
            "DOMMenuItemActive submenu" ],
  test: function() { synthesizeKey("VK_ESCAPE", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkClosed("submenu", testname);
    checkActive(gMenuPopup, "submenu", testname);
    checkActive(document.getElementById("submenupopup"), "", testname);
  }
},
{
  
  
  
  testname: "menuitem with non accelerator disabled",
  events: function() {
    if (navigator.platform.indexOf("Win") == 0)
      return [ "DOMMenuItemInactive submenu", "DOMMenuItemActive other",
               "DOMMenuItemInactive other", "DOMMenuItemActive item1" ];
    else
      return [ "DOMMenuItemInactive submenu", "DOMMenuItemActive last",
               "DOMMenuItemInactive last", "DOMMenuItemActive item1" ];
  },
  test: function() { synthesizeKey("O", { }); synthesizeKey("F", { }); },
  result: function(testname) {
    checkActive(gMenuPopup, "item1", testname);
  }
},
{
  
  
  testname: "menuitem with keypress no accelerator found",
  test: function() { synthesizeKey("G", { }); },
  result: function(testname) {
    checkOpen("trigger", testname);
    checkActive(gMenuPopup, "item1", testname);
  }
},
{
  
  
  testname: "menuitem with non accelerator single",
  events: [ "DOMMenuItemInactive item1", "DOMMenuItemActive amenu",
            "DOMMenuItemInactive amenu", "DOMMenuInactive thepopup",
            "command amenu", "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuItemInactive amenu",
           ],
  test: function() { synthesizeKey("M", { }); },
  result: function(testname) {
    checkClosed("trigger", testname);
    checkActive(gMenuPopup, "", testname);
  }
},
{
  testname: "open popup with open property",
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) { openMenu(gTrigger); },
  result: function(testname, step) {
    checkOpen("trigger", testname);
    if (gIsMenu)
      compareEdge(gTrigger, gMenuPopup, "after_start", 0, 0, testname);
  }
},
{
  testname: "open submenu with open property",
  events: [ "popupshowing submenupopup", "DOMMenuItemActive submenu",
            "popupshown submenupopup" ],
  test: function(testname, step) { openMenu(document.getElementById("submenu")); },
  result: function(testname, step) {
    checkOpen("trigger", testname);
    checkOpen("submenu", testname);
    
    
    
    
    
  }
},
{
  testname: "hidePopup hides entire chain",
  events: [ "popuphiding submenupopup", "popuphidden submenupopup",
            "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuInactive submenupopup",
            "DOMMenuItemInactive submenu", "DOMMenuItemInactive submenu",
            "DOMMenuInactive thepopup", ],
  test: function() { gMenuPopup.hidePopup(); },
  result: function(testname, step) {
    checkClosed("trigger", testname);
    checkClosed("submenu", testname);
  }
},
{
  testname: "open submenu with open property without parent open",
  test: function(testname, step) { openMenu(document.getElementById("submenu")); },
  result: function(testname, step) {
    checkClosed("trigger", testname);
    checkClosed("submenu", testname);
  }
},
{
  testname: "open popup with open property and position",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) {
    gMenuPopup.setAttribute("position", "before_start");
    openMenu(gTrigger);
  },
  result: function(testname, step) {
    compareEdge(gTrigger, gMenuPopup, "before_start", 0, 0, testname);
  }
},
{
  testname: "close popup with open property",
  condition: function() { return gIsMenu; },
  events: [ "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuInactive thepopup" ],
  test: function(testname, step) { closeMenu(gTrigger, gMenuPopup); },
  result: function(testname, step) { checkClosed("trigger", testname); }
},
{
  testname: "open popup with open property, position, anchor and alignment",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    gMenuPopup.setAttribute("position", "start_after");
    gMenuPopup.setAttribute("popupanchor", "topright");
    gMenuPopup.setAttribute("popupalign", "bottomright");
    openMenu(gTrigger);
  },
  result: function(testname, step) {
    compareEdge(gTrigger, gMenuPopup, "start_after", 0, 0, testname);
  }
},
{
  testname: "open popup with open property, anchor and alignment",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    gMenuPopup.removeAttribute("position");
    gMenuPopup.setAttribute("popupanchor", "bottomright");
    gMenuPopup.setAttribute("popupalign", "topright");
    openMenu(gTrigger);
  },
  result: function(testname, step) {
    compareEdge(gTrigger, gMenuPopup, "after_end", 0, 0, testname);
    gMenuPopup.removeAttribute("popupanchor");
    gMenuPopup.removeAttribute("popupalign");
  }
},
{
  testname: "focus and cursor down on trigger",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    gTrigger.focus();
    synthesizeKey("VK_DOWN", { altKey: (navigator.platform.indexOf("Mac") == -1) });
  },
  result: function(testname, step) {
    checkOpen("trigger", testname);
    checkActive(gMenuPopup, "", testname);
  }
},
{
  testname: "focus and cursor up on trigger",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  test: function(testname, step) {
    gTrigger.focus();
    synthesizeKey("VK_UP", { altKey: (navigator.platform.indexOf("Mac") == -1) });
  },
  result: function(testname, step) {
    checkOpen("trigger", testname);
    checkActive(gMenuPopup, "", testname);
  }
},
{
  testname: "select and enter on menuitem",
  condition: function() { return gIsMenu; },
  events: [ "DOMMenuItemActive item1", "DOMMenuItemInactive item1",
            "DOMMenuInactive thepopup", "command item1",
            "popuphiding thepopup", "popuphidden thepopup",
            "DOMMenuItemInactive item1" ],
  test: function(testname, step) {
    synthesizeKey("VK_DOWN", { });
    synthesizeKey("VK_ENTER", { });
  },
  result: function(testname, step) { checkClosed("trigger", testname); }
},
{
  testname: "focus trigger and key to open",
  condition: function() { return gIsMenu; },
  events: [ "popupshowing thepopup", "popupshown thepopup" ],
  autohide: "thepopup",
  test: function(testname, step) {
    gTrigger.focus();
    synthesizeKey((navigator.platform.indexOf("Mac") == -1) ? "VK_F4" : " ", { });
  },
  result: function(testname, step) {
    checkOpen("trigger", testname);
    checkActive(gMenuPopup, "", testname);
  }
},
{
  
  testname: "focus trigger and key wrong modifier",
  condition: function() { return gIsMenu; },
  test: function(testname, step) {
    gTrigger.focus();
    if (navigator.platform.indexOf("Mac") == -1)
      synthesizeKey("", { metaKey: true });
    else
      synthesizeKey("VK_F4", { altKey: true });
  },
  result: function(testname, step) {
    checkClosed("trigger", testname);
  }
},
{
  testname: "mouse click on disabled menu",
  condition: function() { return gIsMenu; },
  test: function(testname, step) {
    gTrigger.setAttribute("disabled", "true");
    synthesizeMouse(gTrigger, 4, 4, { });
  },
  result: function(testname, step) {
    checkClosed("trigger", testname);
    gTrigger.removeAttribute("disabled");
  }
},
{
  
  
  testname: "openPopup synchronous",
  events: [ "popupshowing thepopup", "popupshowing submenupopup",
            "popupshown thepopup", "DOMMenuItemActive submenu",
            "popupshown submenupopup" ],
  test: function(testname, step) {
    gMenuPopup.openPopup(gTrigger, "after_start", 0, 0, false, true);
    document.getElementById("submenupopup").
      openPopup(gTrigger, "end_before", 0, 0, false, true);
    checkOpen("trigger", testname);
    checkOpen("submenu", testname);
  }
},
{
  
  testname: "remove content",
  test: function(testname, step) {
    var submenupopup = document.getElementById("submenupopup");
    submenupopup.parentNode.removeChild(submenupopup);
    var popup = document.getElementById("thepopup");
    popup.parentNode.removeChild(popup);
  }
}

];
