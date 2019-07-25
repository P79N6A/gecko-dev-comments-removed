





function testSelectableSelection(aIdentifier, aSelectedChildren, aMsg)
{
  var acc = getAccessible(aIdentifier, [nsIAccessibleSelectable]);
  if (!acc)
    return;

  var msg = aMsg ? aMsg : "";
  var len = aSelectedChildren.length;

  
  var selectedChildren = acc.GetSelectedChildren();
  is(selectedChildren ? selectedChildren.length : 0, len,
     msg + "getSelectedChildren: wrong selected children count for " +
     prettyName(aIdentifier));

  for (var idx = 0; idx < len; idx++) {
    var expectedAcc = getAccessible(aSelectedChildren[idx]);
    var actualAcc = selectedChildren.queryElementAt(idx, nsIAccessible);
    is(actualAcc, expectedAcc,
       msg + "getSelectedChildren: wrong selected child at index " + idx +
       " for " + prettyName(aIdentifier) + " { actual : " +
       prettyName(actualAcc) + ", expected: " + prettyName(expectedAcc) + "}");
  }

  
  
  
  
  

  
  for (var idx = 0; idx < len; idx++) {
    var expectedAcc = getAccessible(aSelectedChildren[idx]);
    is(acc.refSelection(idx), expectedAcc,
       msg + "refSelection: wrong selected child at index " + idx + " for " +
       prettyName(aIdentifier));
  }

  
  testIsChildSelected(acc, acc, { value: 0 }, aSelectedChildren, msg);
}




function testIsChildSelected(aSelectAcc, aTraversedAcc, aIndexObj, aSelectedChildren, aMsg)
{
  var childCount = aTraversedAcc.childCount;
  for (var idx = 0; idx < childCount; idx++) {
    var child = aTraversedAcc.getChildAt(idx);
    var [state, extraState] = getStates(child);
    if (state & STATE_SELECTABLE) {
      var isSelected = false;
      var len = aSelectedChildren.length;
      for (var jdx = 0; jdx < len; jdx++) {
        if (child == getAccessible(aSelectedChildren[jdx])) {
          isSelected = true;
          break;
        }
      }

      
      is(aSelectAcc.isChildSelected(aIndexObj.value++), isSelected,
         aMsg + "isChildSelected: wrong selected child " + prettyName(child) +
         " for " + prettyName(aSelectAcc));

      
      testStates(child, isSelected ? STATE_SELECTED : 0, 0,
                 !isSelected ? STATE_SELECTED : 0 , 0);

      continue;
    }

    testIsChildSelected(aSelectAcc, child, aIndexObj, aSelectedChildren);
  }
}
