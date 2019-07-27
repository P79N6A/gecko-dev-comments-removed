








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
                                     aOffset, aLength, 0, 0,
                                     QUERY_CONTENT_FLAG_USE_NATIVE_LINE_BREAK);
}












function synthesizeQueryTextRect(aOffset, aLength, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_TEXT_RECT,
                                     aOffset, aLength, 0, 0,
                                     QUERY_CONTENT_FLAG_USE_NATIVE_LINE_BREAK);
}








function synthesizeQueryEditorRect(aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_EDITOR_RECT, 0, 0, 0, 0,
                                     QUERY_CONTENT_FLAG_USE_NATIVE_LINE_BREAK);
}









function synthesizeCharAtPoint(aX, aY, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nullptr;
  }
  return utils.sendQueryContentEvent(utils.QUERY_CHARACTER_AT_POINT,
                                     0, 0, aX, aY,
                                     QUERY_CONTENT_FLAG_USE_NATIVE_LINE_BREAK);
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














function createDragEventObject(aType, aDestElement, aDestWindow, aDataTransfer,
                               aDragEvent)
{
  var destRect = aDestElement.getBoundingClientRect();
  var destClientX = destRect.left + destRect.width / 2;
  var destClientY = destRect.top + destRect.height / 2;
  var destScreenX = aDestWindow.mozInnerScreenX + destClientX;
  var destScreenY = aDestWindow.mozInnerScreenY + destClientY;
  if ("clientX" in aDragEvent && !("screenX" in aDragEvent)) {
    aDragEvent.screenX = aDestWindow.mozInnerScreenX + aDragEvent.clientX;
  }
  if ("clientY" in aDragEvent && !("screenY" in aDragEvent)) {
    aDragEvent.screenY = aDestWindow.mozInnerScreenY + aDragEvent.clientY;
  }
  return Object.assign({ type: aType,
                         screenX: destScreenX, screenY: destScreenY,
                         clientX: destClientX, clientY: destClientY,
                         dataTransfer: aDataTransfer }, aDragEvent);
}























function synthesizeDragOver(aSrcElement, aDestElement, aDragData, aDropEffect, aWindow, aDestWindow, aDragEvent={})
{
  if (!aWindow)
    aWindow = window;
  if (!aDestWindow)
    aDestWindow = aWindow;

  var dataTransfer;
  var trapDrag = function(event) {
    dataTransfer = event.dataTransfer;
    if (aDragData) {
      for (var i = 0; i < aDragData.length; i++) {
        var item = aDragData[i];
        for (var j = 0; j < item.length; j++) {
          dataTransfer.mozSetDataAt(item[j].type, item[j].data, i);
        }
      }
    }
    dataTransfer.dropEffect = aDropEffect || "move";
    event.preventDefault();
  };

  
  aWindow.addEventListener("dragstart", trapDrag, true);
  EventUtils.synthesizeMouseAtCenter(aSrcElement, { type: "mousedown" }, aWindow);

  var rect = aSrcElement.getBoundingClientRect();
  var x = rect.width / 2;
  var y = rect.height / 2;
  EventUtils.synthesizeMouse(aSrcElement, x, y, { type: "mousemove" }, aWindow);
  EventUtils.synthesizeMouse(aSrcElement, x+10, y+10, { type: "mousemove" }, aWindow);
  aWindow.removeEventListener("dragstart", trapDrag, true);

  var event = createDragEventObject("dragenter", aDestElement, aDestWindow,
                                    dataTransfer, aDragEvent);
  EventUtils.sendDragEvent(event, aDestElement, aDestWindow);

  event = createDragEventObject("dragover", aDestElement, aDestWindow,
                                dataTransfer, aDragEvent);
  var result = EventUtils.sendDragEvent(event, aDestElement, aDestWindow);

  return [result, dataTransfer];
}
















function synthesizeDropAfterDragOver(aResult, aDataTransfer, aDestElement, aDestWindow, aDragEvent={})
{
  if (!aDestWindow)
    aDestWindow = window;

  var effect = aDataTransfer.dropEffect;
  var event;

  if (aResult) {
    effect = "none";
  } else if (effect != "none") {
    event = createDragEventObject("drop", aDestElement, aDestWindow,
                                  aDataTransfer, aDragEvent);
    EventUtils.sendDragEvent(event, aDestElement, aDestWindow);
  }

  EventUtils.synthesizeMouseAtCenter(aDestElement, { type: "mouseup" }, aDestWindow);

  return effect;
}





















function synthesizeDrop(aSrcElement, aDestElement, aDragData, aDropEffect, aWindow, aDestWindow, aDragEvent={})
{
  if (!aWindow)
    aWindow = window;
  if (!aDestWindow)
    aDestWindow = aWindow;

  var ds = Components.classes["@mozilla.org/widget/dragservice;1"].
           getService(Components.interfaces.nsIDragService);

  ds.startDragSession();

  try {
    var [result, dataTransfer] = synthesizeDragOver(aSrcElement, aDestElement,
                                                    aDragData, aDropEffect,
                                                    aWindow, aDestWindow,
                                                    aDragEvent);
    return synthesizeDropAfterDragOver(result, dataTransfer, aDestElement,
                                       aDestWindow, aDragEvent);
  } finally {
    ds.endDragSession(true);
  }
}

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
