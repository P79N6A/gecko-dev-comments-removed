









function testChildAtPoint(aID, aX, aY, aChildID, aGrandChildID)
{
  var child = getChildAtPoint(aID, aX, aY, false);
  var expectedChild = getAccessible(aChildID);

  var msg = "Wrong direct child accessible at the point (" + aX + ", " + aY +
    ") of " + prettyName(aID);
  isObject(child, expectedChild, msg);

  var grandChild = getChildAtPoint(aID, aX, aY, true);
  var expectedGrandChild = getAccessible(aGrandChildID);

  msg = "Wrong deepest child accessible at the point (" + aX + ", " + aY +
    ") of " + prettyName(aID);
  isObject(grandChild, expectedGrandChild, msg);
}





function hitTest(aContainerID, aChildID, aGrandChildID)
{
  var container = getAccessible(aContainerID);
  var child = getAccessible(aChildID);
  var grandChild = getAccessible(aGrandChildID);

  var [x, y] = getBoundsForDOMElm(child);

  var actualChild = container.getChildAtPoint(x + 1, y + 1);
  isObject(actualChild, child,
           "Wrong direct child of " + prettyName(aContainerID));

  var actualGrandChild = container.getDeepestChildAtPoint(x + 1, y + 1);
  isObject(actualGrandChild, grandChild,
           "Wrong deepest child of " + prettyName(aContainerID));
}




function testOffsetAtPoint(aHyperTextID, aX, aY, aCoordType, aExpectedOffset)
{
  var hyperText = getAccessible(aHyperTextID, [nsIAccessibleText]);
  var offset = hyperText.getOffsetAtPoint(aX, aY, aCoordType);
  is(offset, aExpectedOffset,
     "Wrong offset at given point (" + aX + ", " + aY + ") for " +
     prettyName(aHyperTextID));
}




function zoomDocument(aDocument, aZoom)
{
  var docShell = aDocument.defaultView.
    QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIWebNavigation).
    QueryInterface(Components.interfaces.nsIDocShell);
  var docViewer = docShell.contentViewer.
    QueryInterface(Components.interfaces.nsIMarkupDocumentViewer);

  docViewer.fullZoom = aZoom;
}











function getChildAtPoint(aIdentifier, aX, aY, aFindDeepestChild)
{
  var acc = getAccessible(aIdentifier);
  if (!acc)
    return;

  var [screenX, screenY] = getBoundsForDOMElm(acc.DOMNode);

  var x = screenX + aX;
  var y = screenY + aY;

  try {
    if (aFindDeepestChild)
      return acc.getDeepestChildAtPoint(x, y);
    return acc.getChildAtPoint(x, y);
  } catch (e) {  }

  return null;
}




function testPos(aID, aPoint)
{
  var [expectedX, expectedY] =
    (aPoint != undefined) ? aPoint : getBoundsForDOMElm(aID);

  var [x, y] = getBounds(aID);
  is(x, expectedX, "Wrong x coordinate of " + prettyName(aID));
  is(y, expectedY, "Wrong y coordinate of " + prettyName(aID));
}




function testBounds(aID, aRect)
{
  var [expectedX, expectedY, expectedWidth, expectedHeight] =
    (aRect != undefined) ? aRect : getBoundsForDOMElm(aID);

  var [x, y, width, height] = getBounds(aID);
  is(x, expectedX, "Wrong x coordinate of " + prettyName(aID));
  is(y, expectedY, "Wrong y coordinate of " + prettyName(aID));
  is(width, expectedWidth, "Wrong width of " + prettyName(aID));
  is(height, expectedHeight, "Wrong height of " + prettyName(aID));
}




function testTextPos(aID, aOffset, aPoint, aCoordOrigin)
{
  var [expectedX, expectedY] = aPoint;

  var xObj = {}, yObj = {};
  var hyperText = getAccessible(aID, [nsIAccessibleText]);
  hyperText.getCharacterExtents(aOffset, xObj, yObj, {}, {}, aCoordOrigin);
  is(xObj.value, expectedX,
     "Wrong x coordinate at offset " + aOffset + " for " + prettyName(aID));
  ok(yObj.value - expectedY < 2 && expectedY - yObj.value < 2,
     "Wrong y coordinate at offset " + aOffset + " for " + prettyName(aID) +
     " - got " + yObj.value + ", expected " + expectedY +
     "The difference doesn't exceed 1.");
}




function testTextBounds(aID, aStartOffset, aEndOffset, aRect, aCoordOrigin)
{
  var [expectedX, expectedY, expectedWidth, expectedHeight] = aRect;

  var xObj = {}, yObj = {}, widthObj = {}, heightObj = {};
  var hyperText = getAccessible(aID, [nsIAccessibleText]);
  hyperText.getRangeExtents(aStartOffset, aEndOffset,
                            xObj, yObj, widthObj, heightObj, aCoordOrigin);
  is(xObj.value, expectedX,
     "Wrong x coordinate of text between offsets (" + aStartOffset + ", " +
     aEndOffset + ") for " + prettyName(aID));
  is(yObj.value, expectedY,
     "Wrong y coordinate of text between offsets (" + aStartOffset + ", " +
     aEndOffset + ") for " + prettyName(aID));

  var msg = "Wrong width of text between offsets (" + aStartOffset + ", " +
    aEndOffset + ") for " + prettyName(aID);
  if (widthObj.value == expectedWidth)
    ok(true, msg);
  else
    todo(false, msg); 

  is(heightObj.value, expectedHeight,
     "Wrong height of text between offsets (" + aStartOffset + ", " +
     aEndOffset + ") for " + prettyName(aID));
}




function getPos(aID)
{
  var accessible = getAccessible(aID);
  var x = {}, y = {};
  accessible.getBounds(x, y, {}, {});
  return [x.value, y.value];
}





function getBounds(aID)
{
  var accessible = getAccessible(aID);
  var x = {}, y = {}, width = {}, height = {};
  accessible.getBounds(x, y, width, height);
  return [x.value, y.value, width.value, height.value];
}





function getBoundsForDOMElm(aID)
{
  var x = 0, y = 0, width = 0, height = 0;

  var elm = getNode(aID);
  if (elm.localName == "area") {
    var mapName = elm.parentNode.getAttribute("name");
    var selector = "[usemap='#" + mapName + "']";
    var img = elm.ownerDocument.querySelector(selector);

    var areaCoords = elm.coords.split(",");
    var areaX = parseInt(areaCoords[0]);
    var areaY = parseInt(areaCoords[1]);
    var areaWidth = parseInt(areaCoords[2]) - areaX;
    var areaHeight = parseInt(areaCoords[3]) - areaY;

    var rect = img.getBoundingClientRect();
    x = rect.left + areaX;
    y = rect.top + areaY;
    width = areaWidth;
    height = areaHeight;
  }
  else {
    var rect = elm.getBoundingClientRect();
    x = rect.left;
    y = rect.top;
    width = rect.width;
    height = rect.height;
  }

  var elmWindow = elm.ownerDocument.defaultView;
  return CSSToDevicePixels(elmWindow,
                           x + elmWindow.mozInnerScreenX,
                           y + elmWindow.mozInnerScreenY,
                           width,
                           height);
}

function CSSToDevicePixels(aWindow, aX, aY, aWidth, aHeight)
{
  var winUtil = aWindow.
    QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIDOMWindowUtils);

  var ratio = winUtil.screenPixelsPerCSSPixel;

  
  
  return [ Math.round(aX * ratio), Math.round(aY * ratio),
           Math.round(aWidth * ratio), Math.round(aHeight * ratio) ];
}
