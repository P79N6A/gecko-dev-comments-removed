









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











function getChildAtPoint(aIdentifier, aX, aY, aFindDeepestChild)
{
  var nodeObj = { value: null };
  var acc = getAccessible(aIdentifier, null, nodeObj);
  var node = nodeObj.value;

  if (!acc || !node)
    return;

  var [deltaX, deltaY] = getScreenCoords(node);

  var x = deltaX + aX;
  var y = deltaY + aY;

  try {
    if (aFindDeepestChild)
      return acc.getDeepestChildAtPoint(x, y);
    return acc.getChildAtPoint(x, y);
  } catch (e) {  }

  return null;
}




function getScreenCoords(aNode)
{
  if (aNode instanceof nsIDOMXULElement)
    return [node.boxObject.screenX, node.boxObject.screenY];

  
  const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  var descr = document.createElementNS(XUL_NS, "description");
  descr.setAttribute("value", "helper description");
  aNode.parentNode.appendChild(descr);
  var descrBoxObject = descr.boxObject;
  var descrRect = descr.getBoundingClientRect();
  var deltaX = descrBoxObject.screenX - descrRect.left;
  var deltaY = descrBoxObject.screenY - descrRect.top;
  aNode.parentNode.removeChild(descr);

  var rect = aNode.getBoundingClientRect();
  return [rect.left + deltaX, rect.top + deltaY];
}
