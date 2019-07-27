







'use strict';

const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ['TraversalRules']; 

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles',  
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Filters',  
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'States',  
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Prefilters',  
  'resource://gre/modules/accessibility/Constants.jsm');

let gSkipEmptyImages = new PrefCache('accessibility.accessfu.skip_empty_images');

function BaseTraversalRule(aRoles, aMatchFunc, aPreFilter) {
  this._explicitMatchRoles = new Set(aRoles);
  this._matchRoles = aRoles;
  if (aRoles.indexOf(Roles.LABEL) < 0) {
    this._matchRoles.push(Roles.LABEL);
  }
  if (aRoles.indexOf(Roles.INTERNAL_FRAME) < 0) {
    
    this._matchRoles.push(Roles.INTERNAL_FRAME);
  }
  this._matchFunc = aMatchFunc || function() { return Filters.MATCH; };
  this.preFilter = aPreFilter || gSimplePreFilter;
}

BaseTraversalRule.prototype = {
    getMatchRoles: function BaseTraversalRule_getmatchRoles(aRules) {
      aRules.value = this._matchRoles;
      return aRules.value.length;
    },

    match: function BaseTraversalRule_match(aAccessible)
    {
      let role = aAccessible.role;
      if (role == Roles.INTERNAL_FRAME) {
        return (Utils.getMessageManager(aAccessible.DOMNode)) ?
          Filters.MATCH  | Filters.IGNORE_SUBTREE : Filters.IGNORE;
      }

      let matchResult = this._explicitMatchRoles.has(role) ?
          this._matchFunc(aAccessible) : Filters.IGNORE;

      
      
      if (role == Roles.LABEL && !(matchResult & Filters.IGNORE_SUBTREE)) {
        let control = Utils.getEmbeddedControl(aAccessible);
        if (control && this._explicitMatchRoles.has(control.role)) {
          matchResult = this._matchFunc(control) | Filters.IGNORE_SUBTREE;
        }
      }

      return matchResult;
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessibleTraversalRule])
};

var gSimpleTraversalRoles =
  [Roles.MENUITEM,
   Roles.LINK,
   Roles.PAGETAB,
   Roles.GRAPHIC,
   Roles.STATICTEXT,
   Roles.TEXT_LEAF,
   Roles.PUSHBUTTON,
   Roles.CHECKBUTTON,
   Roles.RADIOBUTTON,
   Roles.COMBOBOX,
   Roles.PROGRESSBAR,
   Roles.BUTTONDROPDOWN,
   Roles.BUTTONMENU,
   Roles.CHECK_MENU_ITEM,
   Roles.PASSWORD_TEXT,
   Roles.RADIO_MENU_ITEM,
   Roles.TOGGLE_BUTTON,
   Roles.ENTRY,
   Roles.KEY,
   Roles.HEADER,
   Roles.HEADING,
   Roles.SLIDER,
   Roles.SPINBUTTON,
   Roles.OPTION,
   Roles.LISTITEM,
   Roles.GRID_CELL,
   Roles.COLUMNHEADER,
   Roles.ROWHEADER,
   Roles.STATUSBAR,
   Roles.SWITCH,
   Roles.MATHML_MATH];

var gSimpleMatchFunc = function gSimpleMatchFunc(aAccessible) {
  
  
  function isSingleLineage(acc) {
    for (let child = acc; child; child = child.firstChild) {
      if (Utils.visibleChildCount(child) > 1) {
        return false;
      }
    }
    return true;
  }

  function isFlatSubtree(acc) {
    for (let child = acc.firstChild; child; child = child.nextSibling) {
      
      
      if ([Roles.TEXT_LEAF, Roles.STATICTEXT].indexOf(child.role) >= 0) {
        continue;
      }
      if (Utils.visibleChildCount(child) > 0 || child.actionCount > 0) {
        return false;
      }
    }
    return true;
  }

  switch (aAccessible.role) {
  case Roles.COMBOBOX:
    
    
    return Filters.MATCH;
  case Roles.TEXT_LEAF:
    {
      
      let name = aAccessible.name;
      return (name && name.trim()) ? Filters.MATCH : Filters.IGNORE;
    }
  case Roles.STATICTEXT:
    
    return Utils.isListItemDecorator(aAccessible) ?
      Filters.IGNORE : Filters.MATCH;
  case Roles.GRAPHIC:
    return TraversalRules._shouldSkipImage(aAccessible);
  case Roles.HEADER:
  case Roles.HEADING:
  case Roles.COLUMNHEADER:
  case Roles.ROWHEADER:
  case Roles.STATUSBAR:
    if ((aAccessible.childCount > 0 || aAccessible.name) &&
        (isSingleLineage(aAccessible) || isFlatSubtree(aAccessible))) {
      return Filters.MATCH | Filters.IGNORE_SUBTREE;
    }
    return Filters.IGNORE;
  case Roles.GRID_CELL:
    return isSingleLineage(aAccessible) || isFlatSubtree(aAccessible) ?
      Filters.MATCH | Filters.IGNORE_SUBTREE : Filters.IGNORE;
  case Roles.LISTITEM:
    {
      let item = aAccessible.childCount === 2 &&
        aAccessible.firstChild.role === Roles.STATICTEXT ?
        aAccessible.lastChild : aAccessible;
        return isSingleLineage(item) || isFlatSubtree(item) ?
          Filters.MATCH | Filters.IGNORE_SUBTREE : Filters.IGNORE;
    }
  default:
    
    
    return Filters.MATCH |
      Filters.IGNORE_SUBTREE;
  }
};

var gSimplePreFilter = Prefilters.DEFUNCT |
  Prefilters.INVISIBLE |
  Prefilters.ARIA_HIDDEN |
  Prefilters.TRANSPARENT;

this.TraversalRules = { 
  Simple: new BaseTraversalRule(gSimpleTraversalRoles, gSimpleMatchFunc),

  SimpleOnScreen: new BaseTraversalRule(
    gSimpleTraversalRoles, gSimpleMatchFunc,
    Prefilters.DEFUNCT | Prefilters.INVISIBLE | Prefilters.ARIA_HIDDEN |
    Prefilters.TRANSPARENT | Prefilters.OFFSCREEN),

  Anchor: new BaseTraversalRule(
    [Roles.LINK],
    function Anchor_match(aAccessible)
    {
      
      if (Utils.getState(aAccessible).contains(States.LINKED)) {
        return Filters.IGNORE;
      } else {
        return Filters.MATCH;
      }
    }),

  Button: new BaseTraversalRule(
    [Roles.PUSHBUTTON,
     Roles.SPINBUTTON,
     Roles.TOGGLE_BUTTON,
     Roles.BUTTONDROPDOWN,
     Roles.BUTTONDROPDOWNGRID]),

  Combobox: new BaseTraversalRule(
    [Roles.COMBOBOX,
     Roles.LISTBOX]),

  Landmark: new BaseTraversalRule(
    [],
    function Landmark_match(aAccessible) {
      return Utils.getLandmarkName(aAccessible) ? Filters.MATCH :
        Filters.IGNORE;
    }
  ),

  Entry: new BaseTraversalRule(
    [Roles.ENTRY,
     Roles.PASSWORD_TEXT]),

  FormElement: new BaseTraversalRule(
    [Roles.PUSHBUTTON,
     Roles.SPINBUTTON,
     Roles.TOGGLE_BUTTON,
     Roles.BUTTONDROPDOWN,
     Roles.BUTTONDROPDOWNGRID,
     Roles.COMBOBOX,
     Roles.LISTBOX,
     Roles.ENTRY,
     Roles.PASSWORD_TEXT,
     Roles.PAGETAB,
     Roles.RADIOBUTTON,
     Roles.RADIO_MENU_ITEM,
     Roles.SLIDER,
     Roles.CHECKBUTTON,
     Roles.CHECK_MENU_ITEM,
     Roles.SWITCH]),

  Graphic: new BaseTraversalRule(
    [Roles.GRAPHIC],
    function Graphic_match(aAccessible) {
      return TraversalRules._shouldSkipImage(aAccessible);
    }),

  Heading: new BaseTraversalRule(
    [Roles.HEADING],
    function Heading_match(aAccessible) {
      return aAccessible.childCount > 0 ? Filters.MATCH : Filters.IGNORE;
    }),

  ListItem: new BaseTraversalRule(
    [Roles.LISTITEM,
     Roles.TERM]),

  Link: new BaseTraversalRule(
    [Roles.LINK],
    function Link_match(aAccessible)
    {
      
      if (Utils.getState(aAccessible).contains(States.LINKED)) {
        return Filters.MATCH;
      } else {
        return Filters.IGNORE;
      }
    }),

  List: new BaseTraversalRule(
    [Roles.LIST,
     Roles.DEFINITION_LIST]),

  PageTab: new BaseTraversalRule(
    [Roles.PAGETAB]),

  Paragraph: new BaseTraversalRule(
    [Roles.PARAGRAPH,
     Roles.SECTION],
    function Paragraph_match(aAccessible) {
      for (let child = aAccessible.firstChild; child; child = child.nextSibling) {
        if (child.role === Roles.TEXT_LEAF) {
          return Filters.MATCH | Filters.IGNORE_SUBTREE;
        }
      }

      return Filters.IGNORE;
    }),

  RadioButton: new BaseTraversalRule(
    [Roles.RADIOBUTTON,
     Roles.RADIO_MENU_ITEM]),

  Separator: new BaseTraversalRule(
    [Roles.SEPARATOR]),

  Table: new BaseTraversalRule(
    [Roles.TABLE]),

  Checkbox: new BaseTraversalRule(
    [Roles.CHECKBUTTON,
     Roles.CHECK_MENU_ITEM,
     Roles.SWITCH ]),

  _shouldSkipImage: function _shouldSkipImage(aAccessible) {
    if (gSkipEmptyImages.value && aAccessible.name === '') {
      return Filters.IGNORE;
    }
    return Filters.MATCH;
  }
};
