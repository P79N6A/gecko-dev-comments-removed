









function testChildAtPoint(aIdentifier, aX, aY, aFindDeepestChild,
                          aChildIdentifier)
{
  var childAcc = getAccessible(aChildIdentifier);
  var actualChildAcc = getChildAtPoint(aIdentifier, aX, aY, aFindDeepestChild);

  var msg = "Wrong " + (aFindDeepestChild ? "deepest" : "direct");
  msg += " child accessible [" + prettyName(actualChildAcc);
  msg += "] at the point (" + aX + ", " + aY + ") of accessible [";
  msg += prettyName(aIdentifier) + "]";

  is(childAcc, actualChildAcc, msg);
}





function hitTest(aContainerID, aChildID, aGrandChildID)
{
  var container = getAccessible(aContainerID);
  var child = getAccessible(aChildID);
  var grandChild = getAccessible(aGrandChildID);

  var [x, y] = getBoundsForDOMElm(child);

  var actualChild = container.getChildAtPoint(x + 1, y + 1);
  is(actualChild, child,
     "Wrong child, expected: " + prettyName(child) +
     ", got: " + prettyName(actualChild));

  var actualGrandChild = container.getDeepestChildAtPoint(x + 1, y + 1);
  is(actualGrandChild, grandChild,
     "Wrong deepest child, expected: " + prettyName(grandChild) +
     ", got: " + prettyName(actualGrandChild));
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




function testBounds(aID)
{
  var [expectedX, expectedY, expectedWidth, expectedHeight] =
    getBoundsForDOMElm(aID);

  var [x, y, width, height] = getBounds(aID);
  is(x, expectedX, "Wrong x coordinate of " + prettyName(aID));
  is(y, expectedY, "Wrong y coordinate of " + prettyName(aID));
  is(width, expectedWidth, "Wrong width of " + prettyName(aID));
  is(height, expectedHeight, "Wrong height of " + prettyName(aID));
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
  var elm = getNode(aID);
  var elmWindow = elm.ownerDocument.defaultView;
  var winUtil = elmWindow.
    QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIDOMWindowUtils);

  var ratio = winUtil.screenPixelsPerCSSPixel;
  var rect = elm.getBoundingClientRect();
  return [ (rect.left + elmWindow.mozInnerScreenX) * ratio,
           (rect.top + elmWindow.mozInnerScreenY) * ratio,
           rect.width * ratio,
           rect.height * ratio ];
}
