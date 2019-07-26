



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const INCLUDE_DESC = 0x01;
const INCLUDE_NAME = 0x02;
const INCLUDE_CUSTOM = 0x04;
const NAME_FROM_SUBTREE_RULE = 0x08;

const UTTERANCE_DESC_FIRST = 0;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');

let gUtteranceOrder = new PrefCache('accessibility.accessfu.utterance');

var gStringBundle = Cc['@mozilla.org/intl/stringbundle;1'].
  getService(Ci.nsIStringBundleService).
  createBundle('chrome://global/locale/AccessFu.properties');

this.EXPORTED_SYMBOLS = ['UtteranceGenerator'];


















this.UtteranceGenerator = {
  gActionMap: {
    jump: 'jumpAction',
    press: 'pressAction',
    check: 'checkAction',
    uncheck: 'uncheckAction',
    select: 'selectAction',
    open: 'openAction',
    close: 'closeAction',
    switch: 'switchAction',
    click: 'clickAction',
    collapse: 'collapseAction',
    expand: 'expandAction',
    activate: 'activateAction',
    cycle: 'cycleAction'
  },

  








  genForContext: function genForContext(aContext) {
    let utterance = [];
    let addUtterance = function addUtterance(aAccessible) {
      utterance.push.apply(utterance,
        UtteranceGenerator.genForObject(aAccessible));
    };
    let roleString = Utils.AccRetrieval.getStringRole(aContext.accessible.role);
    let nameRule = this.roleRuleMap[roleString] || 0;
    let utteranceOrder = gUtteranceOrder.value || UTTERANCE_DESC_FIRST;
    
    
    let includeSubtree = (Utils.getAttributes(aContext.accessible)[
      'explicit-name'] !== 'true') || !(nameRule & NAME_FROM_SUBTREE_RULE);

    if (utteranceOrder === UTTERANCE_DESC_FIRST) {
      aContext.newAncestry.forEach(addUtterance);
      addUtterance(aContext.accessible);
      if (includeSubtree) {
        aContext.subtreePreorder.forEach(addUtterance);
      }
    } else {
      if (includeSubtree) {
        aContext.subtreePostorder.forEach(addUtterance);
      }
      addUtterance(aContext.accessible);
      aContext.newAncestry.reverse().forEach(addUtterance);
    }

    return utterance;
  },


  








  genForObject: function genForObject(aAccessible) {
    let roleString = Utils.AccRetrieval.getStringRole(aAccessible.role);

    let func = this.objectUtteranceFunctions[roleString] ||
      this.objectUtteranceFunctions.defaultFunc;

    let flags = this.roleRuleMap[roleString] || 0;

    if (aAccessible.childCount == 0)
      flags |= INCLUDE_NAME;

    let state = {};
    let extState = {};
    aAccessible.getState(state, extState);
    let states = {base: state.value, ext: extState.value};

    return func.apply(this, [aAccessible, roleString, states, flags]);
  },

  








  genForAction: function genForAction(aObject, aActionName) {
    return [gStringBundle.GetStringFromName(this.gActionMap[aActionName])];
  },

  





  genForAnnouncement: function genForAnnouncement(aAnnouncement) {
    try {
      return [gStringBundle.GetStringFromName(aAnnouncement)];
    } catch (x) {
      return [aAnnouncement];
    }
  },

  







  genForTabStateChange: function genForTabStateChange(aObject, aTabState) {
    switch (aTabState) {
      case 'newtab':
        return [gStringBundle.GetStringFromName('tabNew')];
      case 'loading':
        return [gStringBundle.GetStringFromName('tabLoading')];
      case 'loaded':
        return [aObject.name || '',
                gStringBundle.GetStringFromName('tabLoaded')];
      case 'loadstopped':
        return [gStringBundle.GetStringFromName('tabLoadStopped')];
      case 'reload':
        return [gStringBundle.GetStringFromName('tabReload')];
      default:
        return [];
    }
  },

  




  genForEditingMode: function genForEditingMode(aIsEditing) {
    return [gStringBundle.GetStringFromName(
              aIsEditing ? 'editingMode' : 'navigationMode')];
  },

  roleRuleMap: {
    'menubar': INCLUDE_DESC,
    'scrollbar': INCLUDE_DESC,
    'grip': INCLUDE_DESC,
    'alert': INCLUDE_DESC | INCLUDE_NAME,
    'menupopup': INCLUDE_DESC,
    'menuitem': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'tooltip': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'columnheader': NAME_FROM_SUBTREE_RULE,
    'rowheader': NAME_FROM_SUBTREE_RULE,
    'column': NAME_FROM_SUBTREE_RULE,
    'row': NAME_FROM_SUBTREE_RULE,
    'application': INCLUDE_NAME,
    'document': INCLUDE_NAME,
    'grouping': INCLUDE_DESC | INCLUDE_NAME,
    'toolbar': INCLUDE_DESC,
    'table': INCLUDE_DESC | INCLUDE_NAME,
    'link': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'helpballoon': NAME_FROM_SUBTREE_RULE,
    'list': INCLUDE_DESC | INCLUDE_NAME,
    'listitem': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'outline': INCLUDE_DESC,
    'outlineitem': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'pagetab': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'graphic': INCLUDE_DESC,
    'pushbutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'checkbutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'radiobutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'buttondropdown': NAME_FROM_SUBTREE_RULE,
    'combobox': INCLUDE_DESC,
    'droplist': INCLUDE_DESC,
    'progressbar': INCLUDE_DESC,
    'slider': INCLUDE_DESC,
    'spinbutton': INCLUDE_DESC,
    'diagram': INCLUDE_DESC,
    'animation': INCLUDE_DESC,
    'equation': INCLUDE_DESC,
    'buttonmenu': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'buttondropdowngrid': NAME_FROM_SUBTREE_RULE,
    'pagetablist': INCLUDE_DESC,
    'canvas': INCLUDE_DESC,
    'check menu item': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'label': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'password text': INCLUDE_DESC,
    'popup menu': INCLUDE_DESC,
    'radio menu item': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'table column header': NAME_FROM_SUBTREE_RULE,
    'table row header': NAME_FROM_SUBTREE_RULE,
    'tear off menu item': NAME_FROM_SUBTREE_RULE,
    'toggle button': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'parent menuitem': NAME_FROM_SUBTREE_RULE,
    'header': INCLUDE_DESC,
    'footer': INCLUDE_DESC,
    'entry': INCLUDE_DESC | INCLUDE_NAME,
    'caption': INCLUDE_DESC,
    'document frame': INCLUDE_DESC,
    'heading': INCLUDE_DESC,
    'calendar': INCLUDE_DESC | INCLUDE_NAME,
    'combobox list': INCLUDE_DESC,
    'combobox option': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'listbox option': NAME_FROM_SUBTREE_RULE,
    'listbox rich option': NAME_FROM_SUBTREE_RULE,
    'gridcell': NAME_FROM_SUBTREE_RULE,
    'check rich option': NAME_FROM_SUBTREE_RULE,
    'term': NAME_FROM_SUBTREE_RULE,
    'definition': NAME_FROM_SUBTREE_RULE,
    'key': NAME_FROM_SUBTREE_RULE,
    'image map': INCLUDE_DESC,
    'option': INCLUDE_DESC,
    'listbox': INCLUDE_DESC,
    'definitionlist': INCLUDE_DESC | INCLUDE_NAME},

  objectUtteranceFunctions: {
    defaultFunc: function defaultFunc(aAccessible, aRoleStr, aStates, aFlags) {
      let utterance = [];

      if (aFlags & INCLUDE_DESC) {
        let desc = this._getLocalizedStates(aStates);
        let roleStr = this._getLocalizedRole(aRoleStr);
        if (roleStr)
          desc.push(roleStr);
        utterance.push(desc.join(' '));
      }

      this._addName(utterance, aAccessible, aFlags);

      return utterance;
    },

    entry: function entry(aAccessible, aRoleStr, aStates, aFlags) {
      let utterance = [];
      let desc = this._getLocalizedStates(aStates);
      desc.push(this._getLocalizedRole(
                  (aStates.ext & Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE) ?
                    'textarea' : 'entry'));

      utterance.push(desc.join(' '));

      this._addName(utterance, aAccessible, aFlags);

      return utterance;
    },

    heading: function heading(aAccessible, aRoleStr, aStates, aFlags) {
      let level = {};
      aAccessible.groupPosition(level, {}, {});
      let utterance =
        [gStringBundle.formatStringFromName('headingLevel', [level.value], 1)];

      this._addName(utterance, aAccessible, aFlags);

      return utterance;
    },

    listitem: function listitem(aAccessible, aRoleStr, aStates, aFlags) {
      let itemno = {};
      let itemof = {};
      aAccessible.groupPosition({}, itemof, itemno);
      let utterance = [];
      if (itemno.value == 1) 
        utterance.push(gStringBundle.GetStringFromName('listStart'));
      else if (itemno.value == itemof.value) 
        utterance.push(gStringBundle.GetStringFromName('listEnd'));

      this._addName(utterance, aAccessible, aFlags);

      return utterance;
    },

    list: function list(aAccessible, aRoleStr, aStates, aFlags) {
      return this._getListUtterance
        (aAccessible, aRoleStr, aFlags, aAccessible.childCount);
    },

    definitionlist: function definitionlist(aAccessible, aRoleStr, aStates, aFlags) {
      return this._getListUtterance
        (aAccessible, aRoleStr, aFlags, aAccessible.childCount / 2);
    },

    application: function application(aAccessible, aRoleStr, aStates, aFlags) {
      
      if (aAccessible.name != aAccessible.DOMNode.location)
        return this.objectUtteranceFunctions.defaultFunc.apply(this,
          [aAccessible, aRoleStr, aStates, aFlags]);

      return [];
    }
  },

  _addName: function _addName(utterance, aAccessible, aFlags) {
    let name;
    if (Utils.getAttributes(aAccessible)['explicit-name'] === 'true' ||
      (aFlags & INCLUDE_NAME)) {
      name = aAccessible.name;
    }

    if (name) {
      let utteranceOrder = gUtteranceOrder.value || UTTERANCE_DESC_FIRST;
      utterance[utteranceOrder === UTTERANCE_DESC_FIRST ?
        'push' : 'unshift'](name);
    }
  },

  _getLocalizedRole: function _getLocalizedRole(aRoleStr) {
    try {
      return gStringBundle.GetStringFromName(aRoleStr.replace(' ', ''));
    } catch (x) {
      return '';
    }
  },

  _getLocalizedStates: function _getLocalizedStates(aStates) {
    let stateUtterances = [];

    if (aStates.base & Ci.nsIAccessibleStates.STATE_UNAVAILABLE) {
      stateUtterances.push(gStringBundle.GetStringFromName('stateUnavailable'));
    }

    
    
    
    
    if (Utils.AndroidSdkVersion < 16 && aStates.base & Ci.nsIAccessibleStates.STATE_CHECKABLE) {
      let stateStr = (aStates.base & Ci.nsIAccessibleStates.STATE_CHECKED) ?
        'stateChecked' : 'stateNotChecked';
      stateUtterances.push(gStringBundle.GetStringFromName(stateStr));
    }

    if (aStates.ext & Ci.nsIAccessibleStates.EXT_STATE_EXPANDABLE) {
      let stateStr = (aStates.base & Ci.nsIAccessibleStates.STATE_EXPANDED) ?
        'stateExpanded' : 'stateCollapsed';
      stateUtterances.push(gStringBundle.GetStringFromName(stateStr));
    }

    if (aStates.base & Ci.nsIAccessibleStates.STATE_REQUIRED) {
      stateUtterances.push(gStringBundle.GetStringFromName('stateRequired'));
    }

    if (aStates.base & Ci.nsIAccessibleStates.STATE_TRAVERSED) {
      stateUtterances.push(gStringBundle.GetStringFromName('stateTraversed'));
    }

    return stateUtterances;
  },

  _getListUtterance: function _getListUtterance(aAccessible, aRoleStr, aFlags, aItemCount) {
    let desc = [];
    let roleStr = this._getLocalizedRole(aRoleStr);
    if (roleStr)
      desc.push(roleStr);
    desc.push
      (gStringBundle.formatStringFromName('listItemCount', [aItemCount], 1));
    let utterance = [desc.join(' ')];

    this._addName(utterance, aAccessible, aFlags);

    return utterance;
  }
};
