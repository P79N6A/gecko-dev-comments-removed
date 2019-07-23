











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























function testTextAttrs(aID, aOffset, aAttrs, aDefAttrs,
                       aStartOffset, aEndOffset, aSkipUnexpectedAttrs)
{
  var accessible = getAccessible(aID, [nsIAccessibleText]);
  if (!accessible)
    return;

  var startOffset = { value: -1 };
  var endOffset = { value: -1 };

  
  var attrs = getTextAttributes(aID, accessible, false, aOffset,
                                startOffset, endOffset);

  if (!attrs)
    return;

  var errorMsg = " for " + aID + " at offset " + aOffset;

  is(startOffset.value, aStartOffset, "Wrong start offset" + errorMsg);
  is(endOffset.value, aEndOffset, "Wrong end offset" + errorMsg);

  compareAttrs(errorMsg, attrs, aAttrs, aSkipUnexpectedAttrs);

  
  var expectedAttrs = {};
  for (var name in aAttrs)
    expectedAttrs[name] = aAttrs[name];

  for (var name in aDefAttrs) {
    if (!(name in expectedAttrs))
      expectedAttrs[name] = aDefAttrs[name];
  }

  attrs = getTextAttributes(aID, accessible, true, aOffset,
                            startOffset, endOffset);
  
  if (!attrs)
    return;

  compareAttrs(errorMsg, attrs, expectedAttrs, aSkipUnexpectedAttrs);
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




function getTextAttributes(aID, aAccessible, aIncludeDefAttrs, aOffset,
                           aStartOffset, aEndOffset)
{
  
  
  var attrs = null;
  try {
    attrs = aAccessible.getTextAttributes(aIncludeDefAttrs, aOffset,
                                          aStartOffset, aEndOffset);
  } catch (e) {
  }

  if (attrs)
    return attrs;

  ok(false, "Can't get text attributes for " + aID);
  return null;
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
