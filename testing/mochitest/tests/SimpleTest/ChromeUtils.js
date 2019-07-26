








const EventUtils = {};
const scriptLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].
                   getService(Components.interfaces.mozIJSSubScriptLoader);
scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/EventUtils.js", EventUtils);












function synthesizeQueryTextContent(aOffset, aLength, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_TEXT_CONTENT,
                                     aOffset, aLength, 0, 0);
}










function synthesizeQueryCaretRect(aOffset, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_CARET_RECT,
                                     aOffset, 0, 0, 0);
}












function synthesizeQueryTextRect(aOffset, aLength, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_TEXT_RECT,
                                     aOffset, aLength, 0, 0);
}








function synthesizeQueryEditorRect(aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_EDITOR_RECT, 0, 0, 0, 0);
}









function synthesizeCharAtPoint(aX, aY, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_CHARACTER_AT_POINT,
                                     0, 0, aX, aY);
}













function synthesizeSelectionSet(aOffset, aLength, aReverse, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return false;
  }
  return utils.sendSelectionSetEvent(aOffset, aLength, aReverse);
}





















function synthesizeDragStart(element, expectedDragData, aWindow, x, y)
{
  if (!aWindow)
    aWindow = window;
  x = x || 2;
  y = y || 2;
  const step = 9;

  var result = "trapDrag was not called";
  var trapDrag = function(event) {
    try {
      var dataTransfer = event.dataTransfer;
      result = null;
      if (!dataTransfer)
        throw "no dataTransfer";
      if (expectedDragData == null ||
          dataTransfer.mozItemCount != expectedDragData.length)
        throw dataTransfer;
      for (var i = 0; i < dataTransfer.mozItemCount; i++) {
        var dtTypes = dataTransfer.mozTypesAt(i);
        if (dtTypes.length != expectedDragData[i].length)
          throw dataTransfer;
        for (var j = 0; j < dtTypes.length; j++) {
          if (dtTypes[j] != expectedDragData[i][j].type)
            throw dataTransfer;
          var dtData = dataTransfer.mozGetDataAt(dtTypes[j],i);
          if (expectedDragData[i][j].eqTest) {
            if (!expectedDragData[i][j].eqTest(dtData, expectedDragData[i][j].data))
              throw dataTransfer;
          }
          else if (expectedDragData[i][j].data != dtData)
            throw dataTransfer;
        }
      }
    } catch(ex) {
      result = ex;
    }
    event.preventDefault();
    event.stopPropagation();
  }
  aWindow.addEventListener("dragstart", trapDrag, false);
  EventUtils.synthesizeMouse(element, x, y, { type: "mousedown" }, aWindow);
  x += step; y += step;
  EventUtils.synthesizeMouse(element, x, y, { type: "mousemove" }, aWindow);
  x += step; y += step;
  EventUtils.synthesizeMouse(element, x, y, { type: "mousemove" }, aWindow);
  aWindow.removeEventListener("dragstart", trapDrag, false);
  EventUtils.synthesizeMouse(element, x, y, { type: "mouseup" }, aWindow);
  return result;
}

















function synthesizeDrop(srcElement, destElement, dragData, dropEffect, aWindow, aDestWindow)
{
  if (!aWindow)
    aWindow = window;
  if (!aDestWindow)
    aDestWindow = aWindow;

  var gWindowUtils = aDestWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                                 getInterface(Components.interfaces.nsIDOMWindowUtils);
  var ds = Components.classes["@mozilla.org/widget/dragservice;1"].
           getService(Components.interfaces.nsIDragService);

  var dataTransfer;
  var trapDrag = function(event) {
    dataTransfer = event.dataTransfer;
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

  ds.startDragSession();

  try {
    
    aWindow.addEventListener("dragstart", trapDrag, true);
    EventUtils.synthesizeMouseAtCenter(srcElement, { type: "mousedown" }, aWindow);

    var rect = srcElement.getBoundingClientRect();
    var x = rect.width / 2;
    var y = rect.height / 2;
    EventUtils.synthesizeMouse(srcElement, x, y, { type: "mousemove" }, aWindow);
    EventUtils.synthesizeMouse(srcElement, x+10, y+10, { type: "mousemove" }, aWindow);
    aWindow.removeEventListener("dragstart", trapDrag, true);

    event = aDestWindow.document.createEvent("DragEvents");
    event.initDragEvent("dragenter", true, true, aDestWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
    gWindowUtils.dispatchDOMEventViaPresShell(destElement, event, true);
    var event = aDestWindow.document.createEvent("DragEvents");
    event.initDragEvent("dragover", true, true, aDestWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
    if (gWindowUtils.dispatchDOMEventViaPresShell(destElement, event, true)) {
      EventUtils.synthesizeMouseAtCenter(destElement, { type: "mouseup" }, aDestWindow);
      return "none";
    }

    if (dataTransfer.dropEffect != "none") {
      event = aDestWindow.document.createEvent("DragEvents");
      event.initDragEvent("drop", true, true, aDestWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
      gWindowUtils.dispatchDOMEventViaPresShell(destElement, event, true);
    }

    EventUtils.synthesizeMouseAtCenter(destElement, { type: "mouseup" }, aDestWindow);

    return dataTransfer.dropEffect;
  } finally {
    ds.endDragSession(true);
  }
};

var PluginUtils =
{
  withTestPlugin : function(callback)
  {
    if (typeof Components == "undefined")
    {
      todo(false, "Not a Mozilla-based browser");
      return false;
    }

    var ph = Components.classes["@mozilla.org/plugin/host;1"]
                       .getService(Components.interfaces.nsIPluginHost);
    var tags = ph.getPluginTags();

    
    for (var i = 0; i < tags.length; i++)
    {
      if (tags[i].name == "Test Plug-in")
      {
        callback(tags[i]);
        return true;
      }
    }
    todo(false, "Need a test plugin on this platform");
    return false;
  }
};
