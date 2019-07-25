



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const INCLUDE_DESC = 0x01;
const INCLUDE_NAME = 0x02;
const INCLUDE_CUSTOM = 0x04;

var gStringBundle = Cc['@mozilla.org/intl/stringbundle;1'].
  getService(Ci.nsIStringBundleService).
  createBundle('chrome://global/locale/AccessFu.properties');


var EXPORTED_SYMBOLS = ['UtteranceGenerator'];

Cu.import('resource://gre/modules/accessibility/Utils.jsm');

















var UtteranceGenerator = {
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


  








  genForObject: function genForObject(aAccessible) {
    let roleString = Utils.AccRetrieval.getStringRole(aAccessible.role);

    let func = this.objectUtteranceFunctions[roleString] ||
      this.objectUtteranceFunctions.defaultFunc;

    let flags = this.verbosityRoleMap[roleString] || 0;

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

  verbosityRoleMap: {
    'menubar': INCLUDE_DESC,
    'scrollbar': INCLUDE_DESC,
    'grip': INCLUDE_DESC,
    'alert': INCLUDE_DESC | INCLUDE_NAME,
    'menupopup': INCLUDE_DESC,
    'menuitem': INCLUDE_DESC,
    'tooltip': INCLUDE_DESC,
    'application': INCLUDE_NAME,
    'document': INCLUDE_NAME,
    'grouping': INCLUDE_DESC | INCLUDE_NAME,
    'toolbar': INCLUDE_DESC,
    'table': INCLUDE_DESC | INCLUDE_NAME,
    'link': INCLUDE_DESC,
    'listitem': INCLUDE_DESC,
    'outline': INCLUDE_DESC,
    'outlineitem': INCLUDE_DESC,
    'pagetab': INCLUDE_DESC,
    'graphic': INCLUDE_DESC,
    'pushbutton': INCLUDE_DESC,
    'checkbutton': INCLUDE_DESC,
    'radiobutton': INCLUDE_DESC,
    'combobox': INCLUDE_DESC,
    'droplist': INCLUDE_DESC,
    'progressbar': INCLUDE_DESC,
    'slider': INCLUDE_DESC,
    'spinbutton': INCLUDE_DESC,
    'diagram': INCLUDE_DESC,
    'animation': INCLUDE_DESC,
    'equation': INCLUDE_DESC,
    'buttonmenu': INCLUDE_DESC,
    'pagetablist': INCLUDE_DESC,
    'canvas': INCLUDE_DESC,
    'check menu item': INCLUDE_DESC,
    'label': INCLUDE_DESC,
    'password text': INCLUDE_DESC,
    'popup menu': INCLUDE_DESC,
    'radio menu item': INCLUDE_DESC,
    'toggle button': INCLUDE_DESC,
    'header': INCLUDE_DESC,
    'footer': INCLUDE_DESC,
    'entry': INCLUDE_DESC | INCLUDE_NAME,
    'caption': INCLUDE_DESC,
    'document frame': INCLUDE_DESC,
    'heading': INCLUDE_DESC,
    'calendar': INCLUDE_DESC | INCLUDE_NAME,
    'combobox list': INCLUDE_DESC,
    'combobox option': INCLUDE_DESC,
    'image map': INCLUDE_DESC,
    'option': INCLUDE_DESC,
    'listbox': INCLUDE_DESC},

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

      let name = (aFlags & INCLUDE_NAME) ? (aAccessible.name || '') : '';
      if (name)
        utterance.push(name);

      return utterance;
    },

    entry: function entry(aAccessible, aRoleStr, aStates, aFlags) {
      let utterance = [];
      let desc = this._getLocalizedStates(aStates);
      desc.push(this._getLocalizedRole(
                  (aStates.ext & Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE) ?
                    'textarea' : 'entry'));

      utterance.push(desc.join(' '));

      let name = (aFlags & INCLUDE_NAME) ? (aAccessible.name || '') : '';
      if (name)
        utterance.push(name);

      return utterance;
    },

    heading: function heading(aAccessible, aRoleStr, aStates, aFlags) {
      let name = (aFlags & INCLUDE_NAME) ? (aAccessible.name || '') : '';
      let level = {};
      aAccessible.groupPosition(level, {}, {});
      let utterance =
        [gStringBundle.formatStringFromName('headingLevel', [level.value], 1)];

      if (name)
        utterance.push(name);

      return utterance;
    },

    listitem: function listitem(aAccessible, aRoleStr, aStates, aFlags) {
      let name = (aFlags & INCLUDE_NAME) ? (aAccessible.name || '') : '';
      let localizedRole = this._getLocalizedRole(aRoleStr);
      let itemno = {};
      let itemof = {};
      aAccessible.groupPosition({}, itemof, itemno);
      let utterance =
        [gStringBundle.formatStringFromName(
           'objItemOf', [localizedRole, itemno.value, itemof.value], 3)];

      if (name)
        utterance.push(name);

      return utterance;
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

    if (aStates.base & Ci.nsIAccessibleStates.STATE_CHECKABLE) {
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
  }
};
