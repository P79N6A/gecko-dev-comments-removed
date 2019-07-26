



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ['TraversalRules'];

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

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
    Ci.nsIAccessibleTraversalRule.PREFILTER_INVISIBLE,

    match: function BaseTraversalRule_match(aAccessible)
    {
      if (aAccessible.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME) {
        return (aAccessible.childCount) ?
          Ci.nsIAccessibleTraversalRule.FILTER_IGNORE :
          Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
      }

      if (this._matchFunc)
        return this._matchFunc(aAccessible);

      return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessibleTraversalRule])
};

var gSimpleTraversalRoles =
  [Ci.nsIAccessibleRole.ROLE_MENUITEM,
   Ci.nsIAccessibleRole.ROLE_LINK,
   Ci.nsIAccessibleRole.ROLE_PAGETAB,
   Ci.nsIAccessibleRole.ROLE_GRAPHIC,
   
   
   
   
   Ci.nsIAccessibleRole.ROLE_TEXT_LEAF,
   Ci.nsIAccessibleRole.ROLE_PUSHBUTTON,
   Ci.nsIAccessibleRole.ROLE_CHECKBUTTON,
   Ci.nsIAccessibleRole.ROLE_RADIOBUTTON,
   Ci.nsIAccessibleRole.ROLE_COMBOBOX,
   Ci.nsIAccessibleRole.ROLE_PROGRESSBAR,
   Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWN,
   Ci.nsIAccessibleRole.ROLE_BUTTONMENU,
   Ci.nsIAccessibleRole.ROLE_CHECK_MENU_ITEM,
   Ci.nsIAccessibleRole.ROLE_PASSWORD_TEXT,
   Ci.nsIAccessibleRole.ROLE_RADIO_MENU_ITEM,
   Ci.nsIAccessibleRole.ROLE_TOGGLE_BUTTON,
   Ci.nsIAccessibleRole.ROLE_ENTRY,
   
   Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME];

var TraversalRules = {
  Simple: new BaseTraversalRule(
    gSimpleTraversalRoles,
    function Simple_match(aAccessible) {
      switch (aAccessible.role) {
      case Ci.nsIAccessibleRole.ROLE_COMBOBOX:
        
        
        return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
      case Ci.nsIAccessibleRole.ROLE_TEXT_LEAF:
        {
          
          let name = aAccessible.name;
          if (name && name.trim())
            return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
          else
            return Ci.nsIAccessibleTraversalRule.FILTER_IGNORE;
        }
      case Ci.nsIAccessibleRole.ROLE_LINK:
        
        
        if (aAccessible.childCount == 0)
          return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
        else
          return Ci.nsIAccessibleTraversalRule.FILTER_IGNORE;
      default:
        
        
        return Ci.nsIAccessibleTraversalRule.FILTER_MATCH |
          Ci.nsIAccessibleTraversalRule.FILTER_IGNORE_SUBTREE;
      }
    }
  ),

  SimpleTouch: new BaseTraversalRule(
    gSimpleTraversalRoles,
    function Simple_match(aAccessible) {
      return Ci.nsIAccessibleTraversalRule.FILTER_MATCH |
        Ci.nsIAccessibleTraversalRule.FILTER_IGNORE_SUBTREE;
    }
  ),

  Anchor: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_LINK],
    function Anchor_match(aAccessible)
    {
      
      let state = {};
      let extraState = {};
      aAccessible.getState(state, extraState);
      if (state.value & Ci.nsIAccessibleStates.STATE_LINKED) {
        return Ci.nsIAccessibleTraversalRule.FILTER_IGNORE;
      } else {
        return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
      }
    }),

  Button: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_PUSHBUTTON,
     Ci.nsIAccessibleRole.ROLE_SPINBUTTON,
     Ci.nsIAccessibleRole.ROLE_TOGGLE_BUTTON,
     Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWN,
     Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWNGRID]),

  Combobox: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_COMBOBOX,
     Ci.nsIAccessibleRole.ROLE_LISTBOX]),

  Entry: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_ENTRY,
     Ci.nsIAccessibleRole.ROLE_PASSWORD_TEXT]),

  FormElement: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_PUSHBUTTON,
     Ci.nsIAccessibleRole.ROLE_SPINBUTTON,
     Ci.nsIAccessibleRole.ROLE_TOGGLE_BUTTON,
     Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWN,
     Ci.nsIAccessibleRole.ROLE_BUTTONDROPDOWNGRID,
     Ci.nsIAccessibleRole.ROLE_COMBOBOX,
     Ci.nsIAccessibleRole.ROLE_LISTBOX,
     Ci.nsIAccessibleRole.ROLE_ENTRY,
     Ci.nsIAccessibleRole.ROLE_PASSWORD_TEXT,
     Ci.nsIAccessibleRole.ROLE_PAGETAB,
     Ci.nsIAccessibleRole.ROLE_RADIOBUTTON,
     Ci.nsIAccessibleRole.ROLE_RADIO_MENU_ITEM,
     Ci.nsIAccessibleRole.ROLE_SLIDER,
     Ci.nsIAccessibleRole.ROLE_CHECKBUTTON,
     Ci.nsIAccessibleRole.ROLE_CHECK_MENU_ITEM]),

  Graphic: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_GRAPHIC]),

  Heading: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_HEADING]),

  ListItem: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_LISTITEM,
     Ci.nsIAccessibleRole.ROLE_TERM]),

  Link: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_LINK],
    function Link_match(aAccessible)
    {
      
      let state = {};
      let extraState = {};
      aAccessible.getState(state, extraState);
      if (state.value & Ci.nsIAccessibleStates.STATE_LINKED) {
        return Ci.nsIAccessibleTraversalRule.FILTER_MATCH;
      } else {
        return Ci.nsIAccessibleTraversalRule.FILTER_IGNORE;
      }
    }),

  List: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_LIST,
     Ci.nsIAccessibleRole.ROLE_DEFINITION_LIST]),

  PageTab: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_PAGETAB]),

  RadioButton: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_RADIOBUTTON,
     Ci.nsIAccessibleRole.ROLE_RADIO_MENU_ITEM]),

  Separator: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_SEPARATOR]),

  Table: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_TABLE]),

  Checkbox: new BaseTraversalRule(
    [Ci.nsIAccessibleRole.ROLE_CHECKBUTTON,
     Ci.nsIAccessibleRole.ROLE_CHECK_MENU_ITEM])
};
