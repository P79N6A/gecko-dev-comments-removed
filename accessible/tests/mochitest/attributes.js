











function testAttrs(aAccOrElmOrID, aAttrs, aSkipUnexpectedAttrs)
{
  var accessible = getAccessible(aAccOrElmOrID);
  if (!accessible)
    return;

  var attrs = null;
  try {
    attrs = accessible.attributes;
  } catch (e) { }
  
  if (!attrs) {
    ok(false, "Can't get object attributes for " + aAccOrElmOrID);
    return;
  }
  
  var errorMsg = " for " + aAccOrElmOrID;
  compareAttrs(errorMsg, attrs, aAttrs, aSkipUnexpectedAttrs);
}









function testGroupAttrs(aAccOrElmOrID, aPosInSet, aSetSize, aLevel)
{
  var attrs = {
    "posinset": String(aPosInSet),
    "setsize": String(aSetSize)
  };

  if (aLevel)
    attrs["level"] = String(aLevel);

  testAttrs(aAccOrElmOrID, attrs, true);
}




















function testTextAttrs(aID, aOffset, aAttrs, aStartOffset, aEndOffset,
                       aSkipUnexpectedAttrs)
{
  var accessible = getAccessible(aID, [nsIAccessibleText]);
  if (!accessible)
    return;

  var startOffset = { value: -1 };
  var endOffset = { value: -1 };
  var attrs = null;
  try {
    attrs = accessible.getTextAttributes(false, aOffset,
                                         startOffset, endOffset);
  } catch (e) {
  }

  if (!attrs) {
    ok(false, "Can't get text attributes for " + aID);
    return;
  }

  var errorMsg = " for " + aID + " at offset " + aOffset;

  is(startOffset.value, aStartOffset, "Wrong start offset" + errorMsg);
  is(endOffset.value, aEndOffset, "Wrong end offset" + errorMsg);

  compareAttrs(errorMsg, attrs, aAttrs, aSkipUnexpectedAttrs);
}











function testDefaultTextAttrs(aID, aDefAttrs, aSkipUnexpectedAttrs)
{
  var accessible = getAccessible(aID, [nsIAccessibleText]);
  if (!accessible)
    return;
  
  var defAttrs = null;
  try{
    defAttrs = accessible.defaultTextAttributes;
  } catch (e) {
  }
  
  if (!defAttrs) {
    ok(false, "Can't get default text attributes for " + aID);
    return;
  }
  
  var errorMsg = ". Getting default text attributes for " + aID;
  compareAttrs(errorMsg, defAttrs, aDefAttrs, aSkipUnexpectedAttrs);
}




function compareAttrs(aErrorMsg, aAttrs, aExpectedAttrs, aSkipUnexpectedAttrs)
{
  var enumerate = aAttrs.enumerate();
  while (enumerate.hasMoreElements()) {
    var prop = enumerate.getNext().QueryInterface(nsIPropertyElement);

    if (!(prop.key in aExpectedAttrs)) {
      if (!aSkipUnexpectedAttrs)
        ok(false, "Unexpected attribute '" + prop.key + "' having '" +
           prop.value + "'" + aErrorMsg);
    } else {
      is(prop.value, aExpectedAttrs[prop.key],
         "Attribute '" + prop.key + " 'has wrong value" + aErrorMsg);
    }
  }

  for (var name in aExpectedAttrs) {
    var value = "";
    try {
      value = aAttrs.getStringProperty(name);
    } catch(e) { }

    if (!value)
      ok(false,
         "There is no expected attribute '" + name + "' " + aErrorMsg);
  }
}
