













function testValue(aAccOrElmOrId, aValue, aCurrValue,
                   aMinValue, aMaxValue, aMinIncr)
{
  var acc = getAccessible(aAccOrElmOrId, [nsIAccessibleValue]);
  if (!acc)
    return;

  is(acc.value, aValue, "Wrong value of " + prettyName(aAccOrElmOrId));

  is(acc.currentValue, aCurrValue,
     "Wrong current value of " + prettyName(aAccOrElmOrId));
  is(acc.minimumValue, aMinValue,
     "Wrong minimum value of " + prettyName(aAccOrElmOrId));
  is(acc.maximumValue, aMaxValue,
     "Wrong maximum value of " + prettyName(aAccOrElmOrId));
  is(acc.minimumIncrement, aMinIncr,
     "Wrong minimum increment value of " + prettyName(aAccOrElmOrId));
}
