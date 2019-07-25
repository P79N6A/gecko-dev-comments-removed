function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();

  let target;
  let doc;
  let win;
  let callbackOnPopupShown;
  let callbackOnPopupHidden;
  let dragService = Components.classes["@mozilla.org/widget/dragservice;1"].
                      getService(Components.interfaces.nsIDragService);

  let onPopupHidden = function (aEvent)
  {
    if (aEvent.originalTarget.localName != "tooltip") {
      return;
    }
    if (callbackOnPopupHidden) {
      setTimeout(callbackOnPopupHidden, 0);
    }
  }

  let onPopupShown = function (aEvent)
  {
    if (aEvent.originalTarget.localName != "tooltip") {
      return;
    }
    if (callbackOnPopupShown) {
      setTimeout(callbackOnPopupShown, 0);
    }
  }

  let finishTest = function ()
  {
    document.removeEventListener("popupshown", onPopupShown, true);
    document.removeEventListener("popuphidden", onPopupHidden, true);

    gBrowser.removeCurrentTab();
    finish();
  }

  let testHideTooltipByMouseDown = function ()
  {
    callbackOnPopupShown = function () {
      callbackOnPopupShown = null;
      ok(true, "tooltip is shown before testing mousedown");

      
      callbackOnPopupHidden = function () {
        callbackOnPopupHidden = null;
        ok(true, "The tooltip is hidden by mousedown");

        EventUtils.synthesizeMouse(target, 5, 15, { type: "mouseup" }, win);
        EventUtils.synthesizeMouse(target, -5, 15, { type: "mousemove" }, win);

        setTimeout(finishTest, 0);
      }
      EventUtils.synthesizeMouse(target, 5, 15, { type: "mousedown" }, win);
    }
    EventUtils.synthesizeMouse(target, 5, 15, { type: "mousemove" }, win);
  }

  let testShowTooltipAgain = function ()
  {
    
    
    callbackOnPopupShown = function () {
      callbackOnPopupShown = null;
      ok(true, "tooltip is shown after drag");

      
      callbackOnPopupHidden = function () {
        callbackOnPopupHidden = null;
        ok(true, "The tooltip is hidden again");

        setTimeout(testHideTooltipByMouseDown, 0);
      }
      EventUtils.synthesizeMouse(target, -5, 15, { type: "mousemove" }, win);
    }
    EventUtils.synthesizeMouse(target, 5, 15, { type: "mousemove" }, win);
  }

  let testDuringDnD = function ()
  {
    
    
    
    
    callbackOnPopupShown = function () {
      callbackOnPopupShown = null;
      ok(false, "tooltip is shown during drag");
    }
    dragService.startDragSession();
    
    
    EventUtils.synthesizeMouse(target, 5, 15, { type: "mousemove" }, win);
    setTimeout(function () {
      callbackOnPopupShown = null;
      ok(true, "tooltip isn't shown during drag");
      dragService.endDragSession(true);
      EventUtils.synthesizeMouse(target, -5, -5, { type: "mousemove" }, win);

      setTimeout(testShowTooltipAgain, 0);
    }, 600);
  }

  let onLoad = function (aEvent)
  {
    doc = gBrowser.contentDocument;
    win = gBrowser.contentWindow;
    target = doc.getElementById("target");

    document.addEventListener("popupshown", onPopupShown, true);
    document.addEventListener("popuphidden", onPopupHidden, true);

    EventUtils.synthesizeMouse(target, -5, -5, { type: "mousemove" }, win);

    
    callbackOnPopupShown = function ()
    {
      callbackOnPopupShown = null;
      ok(true, "The tooltip is shown");

      
      callbackOnPopupHidden = function () {
        callbackOnPopupHidden = null;
        ok(true, "The tooltip is hidden");

        setTimeout(testDuringDnD, 0);
      }
      EventUtils.synthesizeMouse(target, -5, 15, { type: "mousemove" }, win);
    }
    EventUtils.synthesizeMouse(target, 5, 15, { type: "mousemove" }, win);
  }

  gBrowser.selectedBrowser.addEventListener("load",
    function () {
      gBrowser.selectedBrowser.
        removeEventListener("load", arguments.callee, true);
      setTimeout(onLoad, 0);
   }, true);

  content.location = "data:text/html,<html><head></head><body>" +
    "<a id=\"target\" href=\"about:blank\" title=\"This is tooltip text\" " +
       "style=\"display:block;height:20px;margin:10px;\" " +
       "onclick=\"return false;\">here is an anchor element</a></body></html>";
}



