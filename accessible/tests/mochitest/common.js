


const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;

const nsIAccessibleEvent = Components.interfaces.nsIAccessibleEvent;
const nsIAccessibleStateChangeEvent =
  Components.interfaces.nsIAccessibleStateChangeEvent;
const nsIAccessibleCaretMoveEvent =
  Components.interfaces.nsIAccessibleCaretMoveEvent;

const nsIAccessibleStates = Components.interfaces.nsIAccessibleStates;
const nsIAccessibleRole = Components.interfaces.nsIAccessibleRole;
const nsIAccessibleTypes = Components.interfaces.nsIAccessibleTypes;

const nsIAccessibleRelation = Components.interfaces.nsIAccessibleRelation;

const nsIAccessNode = Components.interfaces.nsIAccessNode;
const nsIAccessible = Components.interfaces.nsIAccessible;

const nsIAccessibleDocument = Components.interfaces.nsIAccessibleDocument;

const nsIAccessibleText = Components.interfaces.nsIAccessibleText;
const nsIAccessibleEditableText = Components.interfaces.nsIAccessibleEditableText;

const nsIAccessibleHyperLink = Components.interfaces.nsIAccessibleHyperLink;
const nsIAccessibleHyperText = Components.interfaces.nsIAccessibleHyperText;

const nsIAccessibleImage = Components.interfaces.nsIAccessibleImage;
const nsIAccessibleSelectable = Components.interfaces.nsIAccessibleSelectable;
const nsIAccessibleTable = Components.interfaces.nsIAccessibleTable;
const nsIAccessibleValue = Components.interfaces.nsIAccessibleValue;

const nsIObserverService = Components.interfaces.nsIObserverService;

const nsIDOMDocument = Components.interfaces.nsIDOMDocument;
const nsIDOMNode = Components.interfaces.nsIDOMNode;
const nsIDOMWindow = Components.interfaces.nsIDOMWindow;

const nsIPropertyElement = Components.interfaces.nsIPropertyElement;




const ROLE_PUSHBUTTON = nsIAccessibleRole.ROLE_PUSHBUTTON;
const ROLE_CELL = nsIAccessibleRole.ROLE_CELL;
const ROLE_CHROME_WINDOW = nsIAccessibleRole.ROLE_CHROME_WINDOW;
const ROLE_COMBOBOX = nsIAccessibleRole.ROLE_COMBOBOX;
const ROLE_COMBOBOX_LIST = nsIAccessibleRole.ROLE_COMBOBOX_LIST;
const ROLE_COMBOBOX_OPTION = nsIAccessibleRole.ROLE_COMBOBOX_OPTION;
const ROLE_DOCUMENT = nsIAccessibleRole.ROLE_DOCUMENT;
const ROLE_ENTRY = nsIAccessibleRole.ROLE_ENTRY;
const ROLE_FLAT_EQUATION = nsIAccessibleRole.ROLE_FLAT_EQUATION;
const ROLE_FORM = nsIAccessibleRole.ROLE_FORM;
const ROLE_GRID_CELL = nsIAccessibleRole.ROLE_GRID_CELL;
const ROLE_HEADING = nsIAccessibleRole.ROLE_HEADING;
const ROLE_LABEL = nsIAccessibleRole.ROLE_LABEL;
const ROLE_LIST = nsIAccessibleRole.ROLE_LIST;
const ROLE_OPTION = nsIAccessibleRole.ROLE_OPTION;
const ROLE_PARAGRAPH = nsIAccessibleRole.ROLE_PARAGRAPH;
const ROLE_PASSWORD_TEXT = nsIAccessibleRole.ROLE_PASSWORD_TEXT;
const ROLE_SECTION = nsIAccessibleRole.ROLE_SECTION;
const ROLE_TEXT_CONTAINER = nsIAccessibleRole.ROLE_TEXT_CONTAINER;
const ROLE_TEXT_LEAF = nsIAccessibleRole.ROLE_TEXT_LEAF;
const ROLE_TOGGLE_BUTTON = nsIAccessibleRole.ROLE_TOGGLE_BUTTON;




const STATE_CHECKED = nsIAccessibleStates.STATE_CHECKED;
const STATE_CHECKABLE = nsIAccessibleStates.STATE_CHECKABLE;
const STATE_COLLAPSED = nsIAccessibleStates.STATE_COLLAPSED;
const STATE_EXPANDED = nsIAccessibleStates.STATE_EXPANDED;
const STATE_EXTSELECTABLE = nsIAccessibleStates.STATE_EXTSELECTABLE;
const STATE_FOCUSABLE = nsIAccessibleStates.STATE_FOCUSABLE;
const STATE_FOCUSED = nsIAccessibleStates.STATE_FOCUSED;
const STATE_HASPOPUP = nsIAccessibleStates.STATE_HASPOPUP;
const STATE_MULTISELECTABLE = nsIAccessibleStates.STATE_MULTISELECTABLE;
const STATE_PRESSED = nsIAccessibleStates.STATE_PRESSED;
const STATE_READONLY = nsIAccessibleStates.STATE_READONLY;
const STATE_SELECTABLE = nsIAccessibleStates.STATE_SELECTABLE;
const STATE_SELECTED = nsIAccessibleStates.STATE_SELECTED;

const EXT_STATE_EDITABLE = nsIAccessibleStates.EXT_STATE_EDITABLE;
const EXT_STATE_EXPANDABLE = nsIAccessibleStates.EXT_STATE_EXPANDABLE;
const EXT_STATE_INVALID = nsIAccessibleStates.STATE_INVALID;
const EXT_STATE_MULTI_LINE = nsIAccessibleStates.EXT_STATE_MULTI_LINE;
const EXT_STATE_REQUIRED = nsIAccessibleStates.STATE_REQUIRED;
const EXT_STATE_SUPPORTS_AUTOCOMPLETION = 
      nsIAccessibleStates.EXT_STATE_SUPPORTS_AUTOCOMPLETION;



const MAC = (navigator.platform.indexOf("Mac") != -1)? true : false;
const LINUX = (navigator.platform.indexOf("Linux") != -1)? true : false;
const WIN = (navigator.platform.indexOf("Win") != -1)? true : false;







var gAccRetrieval = null;








function addA11yLoadEvent(aFunc)
{
  function waitForDocLoad()
  {
    window.setTimeout(
      function()
      {
        var accDoc = getAccessible(document);
        var state = {};
        accDoc.getState(state, {});
        if (state.value & nsIAccessibleStates.STATE_BUSY)
          return waitForDocLoad();

        aFunc.call();
      },
      200
    );
  }

  addLoadEvent(waitForDocLoad);
}







function getNode(aNodeOrID)
{
  if (!aNodeOrID)
    return null;

  var node = aNodeOrID;

  if (!(aNodeOrID instanceof nsIDOMNode)) {
    node = document.getElementById(aNodeOrID);

    if (!node) {
      ok(false, "Can't get DOM element for " + aNodeOrID);
      return null;
    }
  }

  return node;
}














function getAccessible(aAccOrElmOrID, aInterfaces, aElmObj, aDoNotFailIfNoAcc)
{
  var elm = null;

  if (aAccOrElmOrID instanceof nsIAccessible) {
    aAccOrElmOrID.QueryInterface(nsIAccessNode);
    elm = aAccOrElmOrID.DOMNode;

  } else if (aAccOrElmOrID instanceof nsIDOMNode) {
    elm = aAccOrElmOrID;

  } else {
    var elm = document.getElementById(aAccOrElmOrID);
    if (!elm) {
      ok(false, "Can't get DOM element for " + aAccOrElmOrID);
      return null;
    }
  }

  if (aElmObj && (typeof aElmObj == "object"))
    aElmObj.value = elm;

  var acc = (aAccOrElmOrID instanceof nsIAccessible) ? aAccOrElmOrID : null;
  if (!acc) {
    try {
      acc = gAccRetrieval.getAccessibleFor(elm);
    } catch (e) {
    }

    if (!acc) {
      if (!aDoNotFailIfNoAcc)
        ok(false, "Can't get accessible for " + aAccOrElmOrID);

      return null;
    }
  }

  if (!aInterfaces)
    return acc;

  if (aInterfaces instanceof Array) {
    for (var index = 0; index < aInterfaces.length; index++) {
      try {
        acc.QueryInterface(aInterfaces[index]);
      } catch (e) {
        ok(false, "Can't query " + aInterfaces[index] + " for " + aID);
        return null;
      }
    }
    return acc;
  }
  
  try {
    acc.QueryInterface(aInterfaces);
  } catch (e) {
    ok(false, "Can't query " + aInterfaces + " for " + aID);
    return null;
  }
  
  return acc;
}




function isAccessible(aAccOrElmOrID)
{
  return getAccessible(aAccOrElmOrID, null, null, true) ? true : false;
}




function roleToString(aRole)
{
  return gAccRetrieval.getStringRole(aRole);
}




function statesToString(aStates, aExtraStates)
{
  var list = gAccRetrieval.getStringStates(aStates, aExtraStates);

  var str = "";
  for (var index = 0; index < list.length; index++)
    str += list.item(index) + ", ";

  return str;
}




function eventTypeToString(aEventType)
{
  gAccRetrieval.getStringEventType(aEventType);
}




function relationTypeToString(aRelationType)
{
  return gAccRetrieval.getStringRelationType(aRelationType);
}








function initialize()
{
  gAccRetrieval = Components.classes["@mozilla.org/accessibleRetrieval;1"].
    getService(nsIAccessibleRetrieval);
}

addLoadEvent(initialize);
