



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const FILTER_IGNORE = Ci.nsIAccessibleTraversalRule.FILTER_IGNORE;
const FILTER_MATCH = Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
const FILTER_IGNORE_SUBTREE = Ci.nsIAccessibleTraversalRule.FILTER_IGNORE_SUBTREE;

const ROLE_MENUITEM = Ci.nsIAccessibleRole.ROLE_MENUITEM;
const ROLE_LINK = Ci.nsIAccessibleRole.ROLE_LINK;
const ROLE_PAGETAB = Ci.nsIAccessibleRole.ROLE_PAGETAB;
const ROLE_GRAPHIC = Ci.nsIAccessibleRole.ROLE_GRAPHIC;
const ROLE_STATICTEXT = Ci.nsIAccessibleRole.ROLE_STATICTEXT;
const ROLE_TEXT_LEAF = Ci.nsIAccessibleRole.ROLE_TEXT_LEAF;
const ROLE_PUSHBUTTON = Ci.nsIAccessibleRole.ROLE_PUSHBUTTON;
const ROLE_SPINBUTTON = Ci.nsIAccessibleRole.ROLE_SPINBUTTON;
const ROLE_CHECKBUTTON = Ci.nsIAccessibleRole.ROLE_CHECKBUTTON;
const ROLE_RADIOBUTTON = Ci.nsIAccessibleRole.ROLE_RADIOBUTTON;
const ROLE_COMBOBOX = Ci.nsIAccessibleRole.ROLE_COMBOBOX;
const ROLE_PROGRESSBAR = Ci.nsIAccessibleRole.ROLE_PROGRESSBAR;
const ROLE_BUTTONDROPDOWN = Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWN;
const ROLE_BUTTONMENU = Ci.nsIAccessibleRole.ROLE_BUTTONMENU;
const ROLE_CHECK_MENU_ITEM = Ci.nsIAccessibleRole.ROLE_CHECK_MENU_ITEM;
const ROLE_PASSWORD_TEXT = Ci.nsIAccessibleRole.ROLE_PASSWORD_TEXT;
const ROLE_RADIO_MENU_ITEM = Ci.nsIAccessibleRole.ROLE_RADIO_MENU_ITEM;
const ROLE_TOGGLE_BUTTON = Ci.nsIAccessibleRole.ROLE_TOGGLE_BUTTON;
const ROLE_ENTRY = Ci.nsIAccessibleRole.ROLE_ENTRY;
const ROLE_LIST = Ci.nsIAccessibleRole.ROLE_LIST;
const ROLE_DEFINITION_LIST = Ci.nsIAccessibleRole.ROLE_DEFINITION_LIST;
const ROLE_LISTITEM = Ci.nsIAccessibleRole.ROLE_LISTITEM;
const ROLE_BUTTONDROPDOWNGRID = Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWNGRID;
const ROLE_LISTBOX = Ci.nsIAccessibleRole.ROLE_LISTBOX;
const ROLE_SLIDER = Ci.nsIAccessibleRole.ROLE_SLIDER;
const ROLE_HEADING = Ci.nsIAccessibleRole.ROLE_HEADING;
const ROLE_TERM = Ci.nsIAccessibleRole.ROLE_TERM;
const ROLE_SEPARATOR = Ci.nsIAccessibleRole.ROLE_SEPARATOR;
const ROLE_TABLE = Ci.nsIAccessibleRole.ROLE_TABLE;
const ROLE_INTERNAL_FRAME = Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME;

this.EXPORTED_SYMBOLS = ['TraversalRules'];

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

let gSkipEmptyImages = new PrefCache('accessibility.accessfu.skip_empty_images');

function BaseTraversalRule(aRoles, aMatchFunc) {
  this._matchRoles = aRoles;
  this._matchFunc = aMatchFunc;
}

BaseTraversalRule.prototype = {
    getMatchRoles: function BaseTraversalRule_getmatchRoles(aRules) {
      aRules.value = this._matchRoles;
      return aRules.value.length;
    },

    preFilter: Ci.nsIAccessibleTraversalRule.PREFILTER_DEFUNCT |
    Ci.nsIAccessibleTraversalRule.PREFILTER_INVISIBLE |
    Ci.nsIAccessibleTraversalRule.PREFILTER_ARIA_HIDDEN,

    match: function BaseTraversalRule_match(aAccessible)
    {
      if (aAccessible.role == ROLE_INTERNAL_FRAME) {
        return (Utils.getMessageManager(aAccessible.DOMNode)) ?
          FILTER_MATCH  | FILTER_IGNORE_SUBTREE : FILTER_IGNORE;
      }

      if (this._matchFunc)
        return this._matchFunc(aAccessible);

      return FILTER_MATCH;
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessibleTraversalRule])
};

var gSimpleTraversalRoles =
  [ROLE_MENUITEM,
   ROLE_LINK,
   ROLE_PAGETAB,
   ROLE_GRAPHIC,
   ROLE_STATICTEXT,
   ROLE_TEXT_LEAF,
   ROLE_PUSHBUTTON,
   ROLE_CHECKBUTTON,
   ROLE_RADIOBUTTON,
   ROLE_COMBOBOX,
   ROLE_PROGRESSBAR,
   ROLE_BUTTONDROPDOWN,
   ROLE_BUTTONMENU,
   ROLE_CHECK_MENU_ITEM,
   ROLE_PASSWORD_TEXT,
   ROLE_RADIO_MENU_ITEM,
   ROLE_TOGGLE_BUTTON,
   ROLE_ENTRY,
   
   ROLE_INTERNAL_FRAME];

this.TraversalRules = {
  Simple: new BaseTraversalRule(
    gSimpleTraversalRoles,
    function Simple_match(aAccessible) {
      switch (aAccessible.role) {
      case ROLE_COMBOBOX:
        
        
        return FILTER_MATCH;
      case ROLE_TEXT_LEAF:
        {
          
          let name = aAccessible.name;
          if (name && name.trim())
            return FILTER_MATCH;
          else
            return FILTER_IGNORE;
        }
      case ROLE_LINK:
        
        
        if (aAccessible.childCount == 0)
          return FILTER_MATCH;
        else
          return FILTER_IGNORE;
      case ROLE_STATICTEXT:
        {
          let parent = aAccessible.parent;
          
          if (parent.childCount > 1 && aAccessible.indexInParent == 0 &&
              parent.role == ROLE_LISTITEM)
            return FILTER_IGNORE;

          return FILTER_MATCH;
        }
      case ROLE_GRAPHIC:
        return TraversalRules._shouldSkipImage(aAccessible);
      default:
        
        
        return FILTER_MATCH |
          FILTER_IGNORE_SUBTREE;
      }
    }
  ),

  SimpleTouch: new BaseTraversalRule(
    gSimpleTraversalRoles,
    function Simple_match(aAccessible) {
      return FILTER_MATCH |
        FILTER_IGNORE_SUBTREE;
    }
  ),

  Anchor: new BaseTraversalRule(
    [ROLE_LINK],
    function Anchor_match(aAccessible)
    {
      
      let state = {};
      let extraState = {};
      aAccessible.getState(state, extraState);
      if (state.value & Ci.nsIAccessibleStates.STATE_LINKED) {
        return FILTER_IGNORE;
      } else {
        return FILTER_MATCH;
      }
    }),

  Button: new BaseTraversalRule(
    [ROLE_PUSHBUTTON,
     ROLE_SPINBUTTON,
     ROLE_TOGGLE_BUTTON,
     ROLE_BUTTONDROPDOWN,
     ROLE_BUTTONDROPDOWNGRID]),

  Combobox: new BaseTraversalRule(
    [ROLE_COMBOBOX,
     ROLE_LISTBOX]),

  Landmark: new BaseTraversalRule(
    [],
    function Landmark_match(aAccessible) {
      return Utils.getLandmarkName(aAccessible) ? FILTER_MATCH :
        FILTER_IGNORE;
    }
  ),

  Entry: new BaseTraversalRule(
    [ROLE_ENTRY,
     ROLE_PASSWORD_TEXT]),

  FormElement: new BaseTraversalRule(
    [ROLE_PUSHBUTTON,
     ROLE_SPINBUTTON,
     ROLE_TOGGLE_BUTTON,
     ROLE_BUTTONDROPDOWN,
     ROLE_BUTTONDROPDOWNGRID,
     ROLE_COMBOBOX,
     ROLE_LISTBOX,
     ROLE_ENTRY,
     ROLE_PASSWORD_TEXT,
     ROLE_PAGETAB,
     ROLE_RADIOBUTTON,
     ROLE_RADIO_MENU_ITEM,
     ROLE_SLIDER,
     ROLE_CHECKBUTTON,
     ROLE_CHECK_MENU_ITEM]),

  Graphic: new BaseTraversalRule(
    [ROLE_GRAPHIC],
    function Graphic_match(aAccessible) {
      return TraversalRules._shouldSkipImage(aAccessible);
    }),

  Heading: new BaseTraversalRule(
    [ROLE_HEADING]),

  ListItem: new BaseTraversalRule(
    [ROLE_LISTITEM,
     ROLE_TERM]),

  Link: new BaseTraversalRule(
    [ROLE_LINK],
    function Link_match(aAccessible)
    {
      
      let state = {};
      let extraState = {};
      aAccessible.getState(state, extraState);
      if (state.value & Ci.nsIAccessibleStates.STATE_LINKED) {
        return FILTER_MATCH;
      } else {
        return FILTER_IGNORE;
      }
    }),

  List: new BaseTraversalRule(
    [ROLE_LIST,
     ROLE_DEFINITION_LIST]),

  PageTab: new BaseTraversalRule(
    [ROLE_PAGETAB]),

  RadioButton: new BaseTraversalRule(
    [ROLE_RADIOBUTTON,
     ROLE_RADIO_MENU_ITEM]),

  Separator: new BaseTraversalRule(
    [ROLE_SEPARATOR]),

  Table: new BaseTraversalRule(
    [ROLE_TABLE]),

  Checkbox: new BaseTraversalRule(
    [ROLE_CHECKBUTTON,
     ROLE_CHECK_MENU_ITEM]),

  _shouldSkipImage: function _shouldSkipImage(aAccessible) {
    if (gSkipEmptyImages.value && aAccessible.name === '') {
      return FILTER_IGNORE;
    }
    return FILTER_MATCH;
  }
};
