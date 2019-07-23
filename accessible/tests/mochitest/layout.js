









function testChildAtPoint(aIdentifier, aX, aY, aFindDeepestChild,
                          aChildIdentifier)
{
  var childAcc = getAccessible(aChildIdentifier);
  if (!childAcc)
    return;

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

  var deltaX = node.boxObject.screenX;
  var deltaY = node.boxObject.screenY;

  var x = deltaX + aX;
  var y = deltaY + aY;

  try {
    if (aFindDeepestChild)
      return acc.getDeepestChildAtPoint(x, y);
    return acc.getChildAtPoint(x, y);
  } catch (e) {  }

  return null;
}
