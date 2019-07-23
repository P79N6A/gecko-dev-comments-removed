


const ROLE_ALERT = nsIAccessibleRole.ROLE_ALERT;
const ROLE_CELL = nsIAccessibleRole.ROLE_CELL;
const ROLE_CHROME_WINDOW = nsIAccessibleRole.ROLE_CHROME_WINDOW;
const ROLE_COMBOBOX = nsIAccessibleRole.ROLE_COMBOBOX;
const ROLE_COMBOBOX_LIST = nsIAccessibleRole.ROLE_COMBOBOX_LIST;
const ROLE_COMBOBOX_OPTION = nsIAccessibleRole.ROLE_COMBOBOX_OPTION;
const ROLE_COLUMNHEADER = nsIAccessibleRole.ROLE_COLUMNHEADER;
const ROLE_DOCUMENT = nsIAccessibleRole.ROLE_DOCUMENT;
const ROLE_ENTRY = nsIAccessibleRole.ROLE_ENTRY;
const ROLE_FLAT_EQUATION = nsIAccessibleRole.ROLE_FLAT_EQUATION;
const ROLE_FORM = nsIAccessibleRole.ROLE_FORM;
const ROLE_GRAPHIC = nsIAccessibleRole.ROLE_GRAPHIC;
const ROLE_GRID_CELL = nsIAccessibleRole.ROLE_GRID_CELL;
const ROLE_GROUPING = nsIAccessibleRole.ROLE_GROUPING;
const ROLE_HEADING = nsIAccessibleRole.ROLE_HEADING;
const ROLE_IMAGE_MAP = nsIAccessibleRole.ROLE_IMAGE_MAP;
const ROLE_INDICATOR = nsIAccessibleRole.ROLE_INDICATOR;
const ROLE_INTERNAL_FRAME = nsIAccessibleRole.ROLE_INTERNAL_FRAME;
const ROLE_LABEL = nsIAccessibleRole.ROLE_LABEL;
const ROLE_LINK = nsIAccessibleRole.ROLE_LINK;
const ROLE_LIST = nsIAccessibleRole.ROLE_LIST;
const ROLE_LISTBOX = nsIAccessibleRole.ROLE_LISTBOX;
const ROLE_NOTHING = nsIAccessibleRole.ROLE_NOTHING;
const ROLE_OPTION = nsIAccessibleRole.ROLE_OPTION;
const ROLE_PARAGRAPH = nsIAccessibleRole.ROLE_PARAGRAPH;
const ROLE_PASSWORD_TEXT = nsIAccessibleRole.ROLE_PASSWORD_TEXT;
const ROLE_PROGRESSBAR = nsIAccessibleRole.ROLE_PROGRESSBAR;
const ROLE_PUSHBUTTON = nsIAccessibleRole.ROLE_PUSHBUTTON;
const ROLE_ROW = nsIAccessibleRole.ROLE_ROW;
const ROLE_SCROLLBAR = nsIAccessibleRole.ROLE_SCROLLBAR;
const ROLE_SECTION = nsIAccessibleRole.ROLE_SECTION;
const ROLE_SLIDER = nsIAccessibleRole.ROLE_SLIDER;
const ROLE_TABLE = nsIAccessibleRole.ROLE_TABLE;
const ROLE_TEXT_CONTAINER = nsIAccessibleRole.ROLE_TEXT_CONTAINER;
const ROLE_TEXT_LEAF = nsIAccessibleRole.ROLE_TEXT_LEAF;
const ROLE_TOGGLE_BUTTON = nsIAccessibleRole.ROLE_TOGGLE_BUTTON;










function testRole(aAccOrElmOrID, aRole)
{
  var role = getRole(aAccOrElmOrID);
  is(role, aRole, "Wrong role for " + prettyName(aAccOrElmOrID) + "!");
}








function getRole(aAccOrElmOrID)
{
  var acc = getAccessible(aAccOrElmOrID);
  if (!acc)
    return -1;

  var role = -1;
  try {
    role = acc.role;
  } catch(e) {
    ok(false, "Role for " + aAccOrElmOrID + " could not be retrieved!");
  }

  return role;
}
