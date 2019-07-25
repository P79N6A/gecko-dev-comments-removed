











function testAttrs(aAccOrElmOrID, aAttrs, aSkipUnexpectedAttrs)
{
  testAttrsInternal(aAccOrElmOrID, aAttrs, aSkipUnexpectedAttrs);
}








function testAbsentAttrs(aAccOrElmOrID, aAbsentAttrs)
{
  testAttrsInternal(aAccOrElmOrID, {}, true, aAbsentAttrs);
}










function testGroupAttrs(aAccOrElmOrID, aPosInSet, aSetSize, aLevel)
{
  var acc = getAccessible(aAccOrElmOrID);
  var levelObj = {}, posInSetObj = {}, setSizeObj = {};
  acc.groupPosition(levelObj, setSizeObj, posInSetObj);

  if (aPosInSet && aSetSize) {
    is(posInSetObj.value, aPosInSet,
       "Wrong group position (posinset) for " + prettyName(aAccOrElmOrID));
    is(setSizeObj.value, aSetSize,
       "Wrong size of the group (setsize) for " + prettyName(aAccOrElmOrID));

    var attrs = {
      "posinset": String(aPosInSet),
      "setsize": String(aSetSize)
    };
    testAttrs(aAccOrElmOrID, attrs, true);
  }

  if (aLevel) {
    is(levelObj.value, aLevel,
       "Wrong group level for " + prettyName(aAccOrElmOrID));

    var attrs = { "level" : String(aLevel) };
    testAttrs(aAccOrElmOrID, attrs, true);
  }
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




function testTextAttrsWrongOffset(aID, aOffset)
{
  var res = false;
  try {
  var s = {}, e = {};
  var acc = getAccessible(ID, [nsIAccessibleText]);
    acc.getTextAttributes(false, 157, s, e);
  } catch (e) {
    res = true;
  }

  ok(res,
     "text attributes are calculated successfully at wrong offset " + aOffset + " for " + prettyName(aID));
}

const kNormalFontWeight =
  function equalsToNormal(aWeight) { return aWeight <= 400 ; }

const kBoldFontWeight =
  function equalsToBold(aWeight) { return aWeight > 400; }


const kInputFontSize = WIN ?
  "10pt" : (MAC ? "8pt" : function() { return true; });









function buildDefaultTextAttrs(aID, aFontSize, aFontWeight)
{
  var elm = getNode(aID);
  var computedStyle = document.defaultView.getComputedStyle(elm, "");
  var bgColor = computedStyle.backgroundColor == "transparent" ?
    "rgb(255, 255, 255)" : computedStyle.backgroundColor;

  var defAttrs = {
    "font-style": computedStyle.fontStyle,
    "font-size": aFontSize,
    "background-color": bgColor,
    "font-weight": aFontWeight ? aFontWeight : kNormalFontWeight,
    "color": computedStyle.color,
    "font-family": computedStyle.fontFamily,
    "text-position": computedStyle.verticalAlign
  };

  return defAttrs;
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

function testAttrsInternal(aAccOrElmOrID, aAttrs, aSkipUnexpectedAttrs,
                   aAbsentAttrs)
{
  var accessible = getAccessible(aAccOrElmOrID);
  if (!accessible)
    return;

  var attrs = null;
  try {
    attrs = accessible.attributes;
  } catch (e) { }
  
  if (!attrs) {
    ok(false, "Can't get object attributes for " + prettyName(aAccOrElmOrID));
    return;
  }
  
  var errorMsg = " for " + prettyName(aAccOrElmOrID);
  compareAttrs(errorMsg, attrs, aAttrs, aSkipUnexpectedAttrs, aAbsentAttrs);
}

function compareAttrs(aErrorMsg, aAttrs, aExpectedAttrs, aSkipUnexpectedAttrs,
                      aAbsentAttrs)
{
  
  var enumerate = aAttrs.enumerate();
  while (enumerate.hasMoreElements()) {
    var prop = enumerate.getNext().QueryInterface(nsIPropertyElement);

    if (!(prop.key in aExpectedAttrs)) {
      if (!aSkipUnexpectedAttrs)
        ok(false, "Unexpected attribute '" + prop.key + "' having '" +
           prop.value + "'" + aErrorMsg);
    } else {
      var msg = "Attribute '" + prop.key + "' has wrong value" + aErrorMsg;
      var expectedValue = aExpectedAttrs[prop.key];

      if (typeof expectedValue == "function")
        ok(expectedValue(prop.value), msg);
      else
        is(prop.value, expectedValue, msg);
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

  
  if (aAbsentAttrs) {
    for (var name in aAbsentAttrs) {
      var wasFound = false;

      var enumerate = aAttrs.enumerate();
      while (enumerate.hasMoreElements()) {
        var prop = enumerate.getNext().QueryInterface(nsIPropertyElement);
        if (prop.key == name)
          wasFound = true;
      }
    }

    ok(!wasFound,
       "There is an unexpected attribute '" + name + "' " + aErrorMsg);
  }
}
