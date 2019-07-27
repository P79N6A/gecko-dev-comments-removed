












const STATE_CHECKED = nsIAccessibleStates.STATE_CHECKED;
const STATE_CHECKABLE = nsIAccessibleStates.STATE_CHECKABLE;
const STATE_COLLAPSED = nsIAccessibleStates.STATE_COLLAPSED;
const STATE_DEFAULT = nsIAccessibleStates.STATE_DEFAULT;
const STATE_EXPANDED = nsIAccessibleStates.STATE_EXPANDED;
const STATE_EXTSELECTABLE = nsIAccessibleStates.STATE_EXTSELECTABLE;
const STATE_FLOATING = nsIAccessibleStates.STATE_FLOATING;
const STATE_FOCUSABLE = nsIAccessibleStates.STATE_FOCUSABLE;
const STATE_FOCUSED = nsIAccessibleStates.STATE_FOCUSED;
const STATE_HASPOPUP = nsIAccessibleStates.STATE_HASPOPUP;
const STATE_INVALID = nsIAccessibleStates.STATE_INVALID;
const STATE_INVISIBLE = nsIAccessibleStates.STATE_INVISIBLE;
const STATE_LINKED = nsIAccessibleStates.STATE_LINKED;
const STATE_MIXED = nsIAccessibleStates.STATE_MIXED;
const STATE_MULTISELECTABLE = nsIAccessibleStates.STATE_MULTISELECTABLE;
const STATE_OFFSCREEN = nsIAccessibleStates.STATE_OFFSCREEN;
const STATE_PRESSED = nsIAccessibleStates.STATE_PRESSED;
const STATE_PROTECTED = nsIAccessibleStates.STATE_PROTECTED;
const STATE_READONLY = nsIAccessibleStates.STATE_READONLY;
const STATE_REQUIRED = nsIAccessibleStates.STATE_REQUIRED;
const STATE_SELECTABLE = nsIAccessibleStates.STATE_SELECTABLE;
const STATE_SELECTED = nsIAccessibleStates.STATE_SELECTED;
const STATE_TRAVERSED = nsIAccessibleStates.STATE_TRAVERSED;
const STATE_UNAVAILABLE = nsIAccessibleStates.STATE_UNAVAILABLE;

const EXT_STATE_ACTIVE = nsIAccessibleStates.EXT_STATE_ACTIVE;
const EXT_STATE_DEFUNCT = nsIAccessibleStates.EXT_STATE_DEFUNCT;
const EXT_STATE_EDITABLE = nsIAccessibleStates.EXT_STATE_EDITABLE;
const EXT_STATE_ENABLED = nsIAccessibleStates.EXT_STATE_ENABLED;
const EXT_STATE_EXPANDABLE = nsIAccessibleStates.EXT_STATE_EXPANDABLE;
const EXT_STATE_HORIZONTAL = nsIAccessibleStates.EXT_STATE_HORIZONTAL;
const EXT_STATE_MODAL = nsIAccessibleStates.EXT_STATE_MODAL;
const EXT_STATE_MULTI_LINE = nsIAccessibleStates.EXT_STATE_MULTI_LINE;
const EXT_STATE_PINNED = nsIAccessibleStates.EXT_STATE_PINNED;
const EXT_STATE_SENSITIVE = nsIAccessibleStates.EXT_STATE_SENSITIVE;
const EXT_STATE_SINGLE_LINE = nsIAccessibleStates.EXT_STATE_SINGLE_LINE;
const EXT_STATE_STALE = nsIAccessibleStates.EXT_STATE_STALE;
const EXT_STATE_SUPPORTS_AUTOCOMPLETION =
  nsIAccessibleStates.EXT_STATE_SUPPORTS_AUTOCOMPLETION;
const EXT_STATE_VERTICAL = nsIAccessibleStates.EXT_STATE_VERTICAL;

const kOrdinalState = false;
const kExtraState = 1;

















function testStates(aAccOrElmOrID, aState, aExtraState, aAbsentState,
                    aAbsentExtraState, aTestName)
{
  var [state, extraState] = getStates(aAccOrElmOrID);
  var role = getRole(aAccOrElmOrID);
  var id = prettyName(aAccOrElmOrID) + (aTestName ? " [" + aTestName + "]": "");

  
  if (aState) {
    isState(state & aState, aState, false,
            "wrong state bits for " + id + "!");
  }

  if (aExtraState)
    isState(extraState & aExtraState, aExtraState, true,
            "wrong extra state bits for " + id + "!");

  if (aAbsentState)
    isState(state & aAbsentState, 0, false,
            "state bits should not be present in ID " + id + "!");

  if (aAbsentExtraState)
    isState(extraState & aAbsentExtraState, 0, true,
            "extraState bits should not be present in ID " + id + "!");

  

  
  if (state & STATE_FOCUSED)
    isState(state & STATE_FOCUSABLE, STATE_FOCUSABLE, false,
            "Focussed " + id + " must be focusable!");

  if (aAbsentState && (aAbsentState & STATE_FOCUSABLE)) {
    isState(state & STATE_FOCUSED, 0, false,
              "Not focusable " + id + " must be not focused!");
  }

  
  if (extraState & EXT_STATE_MULTI_LINE)
    isState(extraState & EXT_STATE_SINGLE_LINE, 0, true,
            "Multiline " + id + " cannot be singleline!");

  if (extraState & EXT_STATE_SINGLE_LINE)
    isState(extraState & EXT_STATE_MULTI_LINE, 0, true,
            "Singleline " + id + " cannot be multiline!");

  
  if (state & STATE_COLLAPSED || state & STATE_EXPANDED)
    isState(extraState & EXT_STATE_EXPANDABLE, EXT_STATE_EXPANDABLE, true,
            "Collapsed or expanded " + id + " must be expandable!");

  if (state & STATE_COLLAPSED)
    isState(state & STATE_EXPANDED, 0, false,
            "Collapsed " + id + " cannot be expanded!");

  if (state & STATE_EXPANDED)
    isState(state & STATE_COLLAPSED, 0, false,
            "Expanded " + id + " cannot be collapsed!");

  if (aAbsentState && (extraState & EXT_STATE_EXPANDABLE)) {
    if (aAbsentState & STATE_EXPANDED) {
      isState(state & STATE_COLLAPSED, STATE_COLLAPSED, false,
              "Not expanded " + id + " must be collapsed!");
    } else if (aAbsentState & STATE_COLLAPSED) {
      isState(state & STATE_EXPANDED, STATE_EXPANDED, false,
              "Not collapsed " + id + " must be expanded!");
    }
  }

  
  if (state & STATE_CHECKED || state & STATE_MIXED &&
      role != ROLE_TOGGLE_BUTTON && role != ROLE_PROGRESSBAR)
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
            "Selected element must be selectable!");
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




function testIsDefunct(aAccessible, aTestName)
{
  var id = prettyName(aAccessible) + (aTestName ? " [" + aTestName + "]" : "");
  var [state, extraState] = getStates(aAccessible);
  isState(extraState & EXT_STATE_DEFUNCT, EXT_STATE_DEFUNCT, true,
          "no defuct state for " + id + "!");
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




function hasState(aAccOrElmOrID, aState, aExtraState)
{
  var [state, exstate] = getStates(aAccOrElmOrID);
  return (aState ? state & aState : true) &&
    (aExtraState ? exstate & aExtraState : true);
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
