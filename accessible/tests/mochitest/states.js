



















function testStates(aAccOrElmOrID, aState, aExtraState, aAbsentState,
                    aAbsentExtraState)
{
  var [state, extraState] = getStates(aAccOrElmOrID);

  var id = prettyName(aAccOrElmOrID);

  
  isState(state & aState, aState, false,
          "wrong state bits for " + id + "!");

  if (aExtraState)
    isState(extraState & aExtraState, aExtraState, true,
            "wrong extra state bits for " + id + "!");

  if (aAbsentState)
    isState(state & aAbsentState, 0, false,
            "state bits should not be present in ID " + id + "!");

  if (aAbsentExtraState)
    isState(extraState & aAbsentExtraState, 0, true,
            "extraState bits should not be present in ID " + id + "!");

  

  
  if (state & STATE_READONLY)
    isState(extraState & EXT_STATE_EDITABLE, 0, true,
            "Read-only " + id + " cannot be editable!");

  if (extraState & EXT_STATE_EDITABLE)
    isState(state & STATE_READONLY, 0, true,
            "Editable " + id + " cannot be readonly!");

  
  if (extraState & EXT_STATE_MULTI_LINE)
    isState(extraState & EXT_STATE_SINGLE_LINE, 0, true,
            "Multiline " + id + " cannot be singleline!");

  if (extraState & EXT_STATE_SINGLE_LINE)
    isState(extraState & EXT_STATE_MULTI_LINE, 0, true,
            "Singleline " + id + " cannot be multiline!");

  
  if (state & STATE_COLLAPSED || state & STATE_EXPANDED)
    isState(extraState & EXT_STATE_EXPANDABLE, EXT_STATE_EXPANDABLE, true,
            "Collapsed or expanded " + id + " should be expandable!");

  if (state & STATE_COLLAPSED)
    isState(state & STATE_EXPANDED, 0, false,
            "Collapsed " + id + " cannot be expanded!");

  if (state & STATE_EXPANDED)
    isState(state & STATE_COLLAPSED, 0, false,
            "Expanded " + id + " cannot be collapsed!");

  
  if (state & STATE_CHECKED || state & STATE_MIXED)
    isState(state & STATE_CHECKABLE, STATE_CHECKABLE, false,
            "Checked or mixed element must be checkable!");

  if (state & STATE_CHECKED)
    isState(state & STATE_MIXED, 0, false,
            "Checked element cannot be state mixed!");

  if (state & STATE_MIXED)
    isState(state & STATE_CHECKED, 0, false,
            "Mixed element cannot be state checked!");

  
  if (state & STATE_SELECTED) {
    isState(state & STATE_SELECTABLE, STATE_SELECTABLE, false,
            "Selected element should be selectable!");
  }

  
  if (state & STATE_UNAVAILABLE) {
    var role = getRole(aAccOrElmOrID);
    if (role != ROLE_GROUPING && role != ROLE_EMBEDDED_OBJECT)
      isState(state & STATE_FOCUSABLE, STATE_FOCUSABLE, false,
              "Disabled " + id + " must be focusable!");
  }
}











function testStatesInSubtree(aAccOrElmOrID, aState, aExtraState, aAbsentState)
{
  
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return;

  if (getRole(acc) != ROLE_TEXT_LEAF)
    
    
    testStates(acc, aState, aExtraState, aAbsentState);

  
  var children = null;
  try {
    children = acc.children;
  } catch(e) {}
  ok(children, "Could not get children for " + aAccOrElmOrID +"!");

  if (children) {
    for (var i = 0; i < children.length; i++) {
      var childAcc = children.queryElementAt(i, nsIAccessible);
      testStatesInSubtree(childAcc, aState, aExtraState, aAbsentState);
    }
  }
}

function getStringStates(aAccOrElmOrID)
{
  var [state, extraState] = getStates(aAccOrElmOrID);
  return statesToString(state, extraState);
}

function getStates(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return [0, 0];
  
  var state = {}, extraState = {};
  acc.getState(state, extraState);

  return [state.value, extraState.value];
}







function isState(aState1, aState2, aIsExtraStates, aMsg)
{
  if (aState1 == aState2) {
    ok(true, aMsg);
    return;
  }

  var got = "0";
  if (aState1) {
    got = statesToString(aIsExtraStates ? 0 : aState1,
                         aIsExtraStates ? aState1 : 0);
  }

  var expected = "0";
  if (aState2) {
    expected = statesToString(aIsExtraStates ? 0 : aState2,
                              aIsExtraStates ? aState2 : 0);
  }

  ok(false, aMsg + "got '" + got + "', expected '" + expected + "'");
}
