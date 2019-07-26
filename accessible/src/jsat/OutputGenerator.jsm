



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const INCLUDE_DESC = 0x01;
const INCLUDE_NAME = 0x02;
const INCLUDE_CUSTOM = 0x04;
const NAME_FROM_SUBTREE_RULE = 0x08;

const OUTPUT_DESC_FIRST = 0;
const OUTPUT_DESC_LAST = 1;

const ROLE_LISTITEM = Ci.nsIAccessibleRole.ROLE_LISTITEM;
const ROLE_STATICTEXT = Ci.nsIAccessibleRole.ROLE_STATICTEXT;
const ROLE_LINK = Ci.nsIAccessibleRole.ROLE_LINK;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'PrefCache',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'PluralForm',
  'resource://gre/modules/PluralForm.jsm');

var gStringBundle = Cc['@mozilla.org/intl/stringbundle;1'].
  getService(Ci.nsIStringBundleService).
  createBundle('chrome://global/locale/AccessFu.properties');

this.EXPORTED_SYMBOLS = ['UtteranceGenerator', 'BrailleGenerator'];

this.OutputGenerator = {

  defaultOutputOrder: OUTPUT_DESC_LAST,

  











  genForContext: function genForContext(aContext) {
    let output = [];
    let self = this;
    let addOutput = function addOutput(aAccessible) {
      output.push.apply(output, self.genForObject(aAccessible, aContext));
    };
    let ignoreSubtree = function ignoreSubtree(aAccessible) {
      let roleString = Utils.AccRetrieval.getStringRole(aAccessible.role);
      let nameRule = self.roleRuleMap[roleString] || 0;
      
      
      return (nameRule & NAME_FROM_SUBTREE_RULE) &&
        (Utils.getAttributes(aAccessible)['explicit-name'] === 'true');
    };

    let contextStart = this._getContextStart(aContext);

    if (this.outputOrder === OUTPUT_DESC_FIRST) {
      contextStart.forEach(addOutput);
      addOutput(aContext.accessible);
      [addOutput(node) for
        (node of aContext.subtreeGenerator(true, ignoreSubtree))];
    } else {
      [addOutput(node) for
        (node of aContext.subtreeGenerator(false, ignoreSubtree))];
      addOutput(aContext.accessible);
      contextStart.reverse().forEach(addOutput);
    }

    
    let trimmed;
    output = [trimmed for (word of output) if (trimmed = word.trim())];
    return {output: output};
  },


  











  genForObject: function genForObject(aAccessible, aContext) {
    let roleString = Utils.AccRetrieval.getStringRole(aAccessible.role);
    let func = this.objectOutputFunctions[
      OutputGenerator._getOutputName(roleString)] ||
      this.objectOutputFunctions.defaultFunc;

    let flags = this.roleRuleMap[roleString] || 0;

    if (aAccessible.childCount == 0)
      flags |= INCLUDE_NAME;

    let state = {};
    let extState = {};
    aAccessible.getState(state, extState);
    let states = {base: state.value, ext: extState.value};
    return func.apply(this, [aAccessible, roleString, states, flags, aContext]);
  },

  







  genForAction: function genForAction(aObject, aActionName) {},

  





  genForAnnouncement: function genForAnnouncement(aAnnouncement) {},

  







  genForTabStateChange: function genForTabStateChange(aObject, aTabState) {},

  




  genForEditingMode: function genForEditingMode(aIsEditing) {},

  _getContextStart: function getContextStart(aContext) {},

  _addName: function _addName(aOutput, aAccessible, aFlags) {
    let name;
    if (Utils.getAttributes(aAccessible)['explicit-name'] === 'true' ||
      (aFlags & INCLUDE_NAME)) {
      name = aAccessible.name;
    }

    let description = aAccessible.description;
    if (description) {
      
      
      let tmpName = name || aAccessible.name;
      if (tmpName && (description !== tmpName)) {
        name = name || '';
        name = this.outputOrder === OUTPUT_DESC_FIRST ?
          description + ' - ' + name :
          name + ' - ' + description;
      }
    }

    if (name) {
      aOutput[this.outputOrder === OUTPUT_DESC_FIRST ?
        'push' : 'unshift'](name);
    }
  },

  




  _addLandmark: function _addLandmark(aOutput, aAccessible) {
    let landmarkName = Utils.getLandmarkName(aAccessible);
    if (!landmarkName) {
      return;
    }

    let landmark = gStringBundle.GetStringFromName(landmarkName);
    if (!landmark) {
      return;
    }

    aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'unshift' : 'push'](
      landmark);
  },

  get outputOrder() {
    if (!this._utteranceOrder) {
      this._utteranceOrder = new PrefCache('accessibility.accessfu.utterance');
    }
    return typeof this._utteranceOrder.value === 'number' ?
      this._utteranceOrder.value : this.defaultOutputOrder;
  },

  _getOutputName: function _getOutputName(aName) {
    return aName.replace(' ', '');
  },

  _getLocalizedRole: function _getLocalizedRole(aRoleStr) {},

  _getLocalizedStates: function _getLocalizedStates(aStates) {},

  _getPluralFormString: function _getPluralFormString(aString, aCount) {
    let str = gStringBundle.GetStringFromName(this._getOutputName(aString));
    str = PluralForm.get(aCount, str);
    return str.replace('#1', aCount);
  },

  roleRuleMap: {
    'menubar': INCLUDE_DESC,
    'scrollbar': INCLUDE_DESC,
    'grip': INCLUDE_DESC,
    'alert': INCLUDE_DESC | INCLUDE_NAME,
    'menupopup': INCLUDE_DESC,
    'menuitem': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'tooltip': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'columnheader': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'rowheader': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'column': NAME_FROM_SUBTREE_RULE,
    'row': NAME_FROM_SUBTREE_RULE,
    'cell': INCLUDE_DESC | INCLUDE_NAME,
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
    'listbox option': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
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

  objectOutputFunctions: {
    _generateBaseOutput: function _generateBaseOutput(aAccessible, aRoleStr, aStates, aFlags) {
      let output = [];

      if (aFlags & INCLUDE_DESC) {
        let desc = this._getLocalizedStates(aStates);
        let roleStr = this._getLocalizedRole(aRoleStr);
        if (roleStr)
          desc.push(roleStr);
        output.push(desc.join(' '));
      }

      this._addName(output, aAccessible, aFlags);
      this._addLandmark(output, aAccessible);

      return output;
    },

    label: function label(aAccessible, aRoleStr, aStates, aFlags, aContext) {
      if (aContext.isNestedControl ||
          aContext.accessible == Utils.getEmbeddedControl(aAccessible)) {
        
        
        return [];
      }

      return this.objectOutputFunctions.defaultFunc.apply(this, arguments);
    },

    entry: function entry(aAccessible, aRoleStr, aStates, aFlags) {
      let output = [];
      let desc = this._getLocalizedStates(aStates);
      desc.push(this._getLocalizedRole(
                  (aStates.ext & Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE) ?
                    'textarea' : 'entry'));

      output.push(desc.join(' '));

      this._addName(output, aAccessible, aFlags);
      this._addLandmark(output, aAccessible);

      return output;
    },

    pagetab: function pagetab(aAccessible, aRoleStr, aStates, aFlags) {
      let localizedRole = this._getLocalizedRole(aRoleStr);
      let itemno = {};
      let itemof = {};
      aAccessible.groupPosition({}, itemof, itemno);
      let output = [];
      let desc = this._getLocalizedStates(aStates);
      desc.push(
        gStringBundle.formatStringFromName(
          'objItemOf', [localizedRole, itemno.value, itemof.value], 3));
      output.push(desc.join(' '));

      this._addName(output, aAccessible, aFlags);
      this._addLandmark(output, aAccessible);

      return output;
    },

    table: function table(aAccessible, aRoleStr, aStates, aFlags) {
      let output = [];
      let table;
      try {
        table = aAccessible.QueryInterface(Ci.nsIAccessibleTable);
      } catch (x) {
        Logger.logException(x);
        return output;
      } finally {
        
        
        if (table.isProbablyForLayout()) {
          return output;
        }
        let tableColumnInfo = this._getPluralFormString('tableColumnInfo',
          table.columnCount);
        let tableRowInfo = this._getPluralFormString('tableRowInfo',
          table.rowCount);
        output.push(gStringBundle.formatStringFromName(
          this._getOutputName('tableInfo'), [this._getLocalizedRole(aRoleStr),
            tableColumnInfo, tableRowInfo], 3));
        this._addName(output, aAccessible, aFlags);
        this._addLandmark(output, aAccessible);
        return output;
      }
    }
  }
};

















this.UtteranceGenerator = {
  __proto__: OutputGenerator,

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

  
  genForAction: function genForAction(aObject, aActionName) {
    return [gStringBundle.GetStringFromName(this.gActionMap[aActionName])];
  },

  genForLiveRegion: function genForLiveRegion(aContext, aIsHide, aModifiedText) {
    let utterance = [];
    if (aIsHide) {
      utterance.push(gStringBundle.GetStringFromName('hidden'));
    }
    return utterance.concat(
      aModifiedText || this.genForContext(aContext).output);
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

  objectOutputFunctions: {

    __proto__: OutputGenerator.objectOutputFunctions,

    defaultFunc: function defaultFunc(aAccessible, aRoleStr, aStates, aFlags) {
      return this.objectOutputFunctions._generateBaseOutput.apply(this, arguments);
    },

    heading: function heading(aAccessible, aRoleStr, aStates, aFlags) {
      let level = {};
      aAccessible.groupPosition(level, {}, {});
      let utterance =
        [gStringBundle.formatStringFromName('headingLevel', [level.value], 1)];

      this._addName(utterance, aAccessible, aFlags);
      this._addLandmark(utterance, aAccessible);

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
      this._addLandmark(utterance, aAccessible);

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
        return this.objectOutputFunctions.defaultFunc.apply(this,
          [aAccessible, aRoleStr, aStates, aFlags]);

      return [];
    },

    cell: function cell(aAccessible, aRoleStr, aStates, aFlags, aContext) {
      let utterance = [];
      let cell = aContext.getCellInfo(aAccessible);
      if (cell) {
        let desc = [];
        let addCellChanged = function addCellChanged(aDesc, aChanged, aString, aIndex) {
          if (aChanged) {
            aDesc.push(gStringBundle.formatStringFromName(aString,
              [aIndex + 1], 1));
          }
        };
        let addExtent = function addExtent(aDesc, aExtent, aString) {
          if (aExtent > 1) {
            aDesc.push(gStringBundle.formatStringFromName(aString,
              [aExtent], 1));
          }
        };
        let addHeaders = function addHeaders(aDesc, aHeaders) {
          if (aHeaders.length > 0) {
            aDesc.push.apply(aDesc, aHeaders);
          }
        };

        addCellChanged(desc, cell.columnChanged, 'columnInfo', cell.columnIndex);
        addCellChanged(desc, cell.rowChanged, 'rowInfo', cell.rowIndex);

        addExtent(desc, cell.columnExtent, 'spansColumns');
        addExtent(desc, cell.rowExtent, 'spansRows');

        addHeaders(desc, cell.columnHeaders);
        addHeaders(desc, cell.rowHeaders);

        utterance.push(desc.join(' '));
      }

      this._addName(utterance, aAccessible, aFlags);
      this._addLandmark(utterance, aAccessible);

      return utterance;
    },

    columnheader: function columnheader() {
      return this.objectOutputFunctions.cell.apply(this, arguments);
    },

    rowheader: function rowheader() {
      return this.objectOutputFunctions.cell.apply(this, arguments);
    }
  },

  _getContextStart: function _getContextStart(aContext) {
    return aContext.newAncestry;
  },

  _getLocalizedRole: function _getLocalizedRole(aRoleStr) {
    try {
      return gStringBundle.GetStringFromName(this._getOutputName(aRoleStr));
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

    if (aStates.base & Ci.nsIAccessibleStates.STATE_HASPOPUP) {
      stateUtterances.push(gStringBundle.GetStringFromName('stateHasPopup'));
    }

    if (aStates.base & Ci.nsIAccessibleStates.STATE_SELECTED) {
      stateUtterances.push(gStringBundle.GetStringFromName('stateSelected'));
    }

    return stateUtterances;
  },

  _getListUtterance: function _getListUtterance(aAccessible, aRoleStr, aFlags, aItemCount) {
    let desc = [];
    let roleStr = this._getLocalizedRole(aRoleStr);
    if (roleStr) {
      desc.push(roleStr);
    }
    desc.push(this._getPluralFormString('listItemsCount', aItemCount));
    let utterance = [desc.join(' ')];

    this._addName(utterance, aAccessible, aFlags);
    this._addLandmark(utterance, aAccessible);

    return utterance;
  }
};


this.BrailleGenerator = {
  __proto__: OutputGenerator,

  genForContext: function genForContext(aContext) {
    let output = OutputGenerator.genForContext.apply(this, arguments);

    let acc = aContext.accessible;
    if (acc instanceof Ci.nsIAccessibleText) {
      output.endOffset = this.outputOrder === OUTPUT_DESC_FIRST ?
                         output.output.join(' ').length : acc.characterCount;
      output.startOffset = output.endOffset - acc.characterCount;
    }

    return output;
  },

  objectOutputFunctions: {

    __proto__: OutputGenerator.objectOutputFunctions,

    defaultFunc: function defaultFunc(aAccessible, aRoleStr, aStates, aFlags) {
      let braille = this.objectOutputFunctions._generateBaseOutput.apply(this, arguments);

      if (aAccessible.indexInParent === 1 &&
          aAccessible.parent.role == ROLE_LISTITEM &&
          aAccessible.previousSibling.role == ROLE_STATICTEXT) {
        if (aAccessible.parent.parent && aAccessible.parent.parent.DOMNode &&
            aAccessible.parent.parent.DOMNode.nodeName == 'UL') {
          braille.unshift('*');
        } else {
          braille.unshift(aAccessible.previousSibling.name);
        }
      }

      return braille;
    },

    listitem: function listitem(aAccessible, aRoleStr, aStates, aFlags) {
      let braille = [];

      this._addName(braille, aAccessible, aFlags);
      this._addLandmark(braille, aAccessible);

      return braille;
    },

    cell: function cell(aAccessible, aRoleStr, aStates, aFlags, aContext) {
      let braille = [];
      let cell = aContext.getCellInfo(aAccessible);
      if (cell) {
        let desc = [];
        let addHeaders = function addHeaders(aDesc, aHeaders) {
          if (aHeaders.length > 0) {
            aDesc.push.apply(aDesc, aHeaders);
          }
        };

        desc.push(gStringBundle.formatStringFromName(
          this._getOutputName('cellInfo'), [cell.columnIndex + 1,
            cell.rowIndex + 1], 2));

        addHeaders(desc, cell.columnHeaders);
        addHeaders(desc, cell.rowHeaders);
        braille.push(desc.join(' '));
      }

      this._addName(braille, aAccessible, aFlags);
      this._addLandmark(braille, aAccessible);
      return braille;
    },

    columnheader: function columnheader() {
      return this.objectOutputFunctions.cell.apply(this, arguments);
    },

    rowheader: function rowheader() {
      return this.objectOutputFunctions.cell.apply(this, arguments);
    },

    statictext: function statictext(aAccessible, aRoleStr, aStates, aFlags) {
      
      
      if (aAccessible.parent.role == ROLE_LISTITEM) {
        return [];
      }

      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    _useStateNotRole: function _useStateNotRole(aAccessible, aRoleStr, aStates, aFlags) {
      let braille = [];

      let desc = this._getLocalizedStates(aStates);
      braille.push(desc.join(' '));

      this._addName(braille, aAccessible, aFlags);
      this._addLandmark(braille, aAccessible);

      return braille;
    },

    checkbutton: function checkbutton(aAccessible, aRoleStr, aStates, aFlags) {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    radiobutton: function radiobutton(aAccessible, aRoleStr, aStates, aFlags) {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    togglebutton: function radiobutton(aAccessible, aRoleStr, aStates, aFlags) {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    }
  },

  _getContextStart: function _getContextStart(aContext) {
    if (aContext.accessible.parent.role == ROLE_LINK) {
      return [aContext.accessible.parent];
    }

    return [];
  },

  _getOutputName: function _getOutputName(aName) {
    return OutputGenerator._getOutputName(aName) + 'Abbr';
  },

  _getLocalizedRole: function _getLocalizedRole(aRoleStr) {
    try {
      return gStringBundle.GetStringFromName(this._getOutputName(aRoleStr));
    } catch (x) {
      try {
        return gStringBundle.GetStringFromName(
          OutputGenerator._getOutputName(aRoleStr));
      } catch (y) {
        return '';
      }
    }
  },

  _getLocalizedStates: function _getLocalizedStates(aStates) {
    let stateBraille = [];

    let getCheckedState = function getCheckedState() {
      let resultMarker = [];
      let state = aStates.base;
      let fill = !!(state & Ci.nsIAccessibleStates.STATE_CHECKED) ||
                 !!(state & Ci.nsIAccessibleStates.STATE_PRESSED);

      resultMarker.push('(');
      resultMarker.push(fill ? 'x' : ' ');
      resultMarker.push(')');

      return resultMarker.join('');
    };

    if (aStates.base & Ci.nsIAccessibleStates.STATE_CHECKABLE) {
      stateBraille.push(getCheckedState());
    }

    return stateBraille;
  }

};
