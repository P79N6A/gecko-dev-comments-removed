






'use strict';

const {utils: Cu, interfaces: Ci} = Components;

const INCLUDE_DESC = 0x01;
const INCLUDE_NAME = 0x02;
const INCLUDE_VALUE = 0x04;
const NAME_FROM_SUBTREE_RULE = 0x10;
const IGNORE_EXPLICIT_NAME = 0x20;

const OUTPUT_DESC_FIRST = 0;
const OUTPUT_DESC_LAST = 1;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'PrefCache', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles', 
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'States', 
  'resource://gre/modules/accessibility/Constants.jsm');

this.EXPORTED_SYMBOLS = ['UtteranceGenerator', 'BrailleGenerator']; 

let OutputGenerator = {

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
      
      
      return (((nameRule & INCLUDE_VALUE) && aAccessible.value) ||
              ((nameRule & NAME_FROM_SUBTREE_RULE) &&
               (Utils.getAttributes(aAccessible)['explicit-name'] === 'true' &&
               !(nameRule & IGNORE_EXPLICIT_NAME))));
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

    return output;
  },


  











  genForObject: function genForObject(aAccessible, aContext) {
    let roleString = Utils.AccRetrieval.getStringRole(aAccessible.role);
    let func = this.objectOutputFunctions[
      OutputGenerator._getOutputName(roleString)] ||
      this.objectOutputFunctions.defaultFunc;

    let flags = this.roleRuleMap[roleString] || 0;

    if (aAccessible.childCount === 0) {
      flags |= INCLUDE_NAME;
    }

    return func.apply(this, [aAccessible, roleString,
                             Utils.getState(aAccessible), flags, aContext]);
  },

  







  genForAction: function genForAction(aObject, aActionName) {}, 

  




  genForAnnouncement: function genForAnnouncement(aAnnouncement) {}, 

  







  genForTabStateChange: function genForTabStateChange(aObject, aTabState) {}, 

  




  genForEditingMode: function genForEditingMode(aIsEditing) {}, 

  _getContextStart: function getContextStart(aContext) {}, 

  





  _addName: function _addName(aOutput, aAccessible, aFlags) {
    let name;
    if ((Utils.getAttributes(aAccessible)['explicit-name'] === 'true' &&
         !(aFlags & IGNORE_EXPLICIT_NAME)) || (aFlags & INCLUDE_NAME)) {
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

    if (!name || !name.trim()) {
      return;
    }
    aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'push' : 'unshift'](name);
  },

  




  _addLandmark: function _addLandmark(aOutput, aAccessible) {
    let landmarkName = Utils.getLandmarkName(aAccessible);
    if (!landmarkName) {
      return;
    }
    aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'unshift' : 'push']({
      string: landmarkName
    });
  },

  





  _addMathRoles: function _addMathRoles(aOutput, aAccessible, aRoleStr) {
    
    let roleStr = aRoleStr;
    switch(aAccessible.role) {
      case Roles.MATHML_CELL:
      case Roles.MATHML_ENCLOSED:
      case Roles.MATHML_LABELED_ROW:
      case Roles.MATHML_ROOT:
      case Roles.MATHML_SQUARE_ROOT:
      case Roles.MATHML_TABLE:
      case Roles.MATHML_TABLE_ROW:
        
        break;
      case Roles.MATHML_MULTISCRIPTS:
      case Roles.MATHML_OVER:
      case Roles.MATHML_SUB:
      case Roles.MATHML_SUB_SUP:
      case Roles.MATHML_SUP:
      case Roles.MATHML_UNDER:
      case Roles.MATHML_UNDER_OVER:
        
        roleStr = 'mathmlscripted';
        break;
      case Roles.MATHML_FRACTION:
        
        
        
        
        
        let linethickness = Utils.getAttributes(aAccessible).linethickness;
        if (linethickness) {
            let numberMatch = linethickness.match(/^(?:\d|\.)+/);
            if (numberMatch && !parseFloat(numberMatch[0])) {
                roleStr += 'withoutbar';
            }
        }
        break;
      default:
        
        roleStr = null;
        break;
    }

    
    
    let mathRole = Utils.getMathRole(aAccessible);
    if (mathRole) {
      aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'push' : 'unshift']
        ({string: this._getOutputName(mathRole)});
    }
    if (roleStr) {
      aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'push' : 'unshift']
        ({string: this._getOutputName(roleStr)});
    }
  },

  




  _addMencloseNotations: function _addMencloseNotations(aOutput, aAccessible) {
    let notations = Utils.getAttributes(aAccessible).notation || 'longdiv';
    aOutput[this.outputOrder === OUTPUT_DESC_FIRST ? 'push' : 'unshift'].apply(
      aOutput, [for (notation of notations.split(' '))
        {string: this._getOutputName('notation-' + notation)}
      ]
    );
  },

  





  _addType: function _addType(aOutput, aAccessible, aRoleStr) {
    if (aRoleStr !== 'entry') {
      return;
    }

    let typeName = Utils.getAttributes(aAccessible)['text-input-type'];
    
    if (!typeName || typeName === 'text') {
      return;
    }
    aOutput.push({string: 'textInputType_' + typeName});
  },

  _addState: function _addState(aOutput, aState, aRoleStr) {}, 

  _addRole: function _addRole(aOutput, aAccessible, aRoleStr) {}, 

  get outputOrder() {
    if (!this._utteranceOrder) {
      this._utteranceOrder = new PrefCache('accessibility.accessfu.utterance');
    }
    return typeof this._utteranceOrder.value === 'number' ?
      this._utteranceOrder.value : this.defaultOutputOrder;
  },

  _getOutputName: function _getOutputName(aName) {
    return aName.replace(/\s/g, '');
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
    'switch': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'pushbutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'checkbutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'radiobutton': INCLUDE_DESC | NAME_FROM_SUBTREE_RULE,
    'buttondropdown': NAME_FROM_SUBTREE_RULE,
    'combobox': INCLUDE_DESC | INCLUDE_VALUE,
    'droplist': INCLUDE_DESC,
    'progressbar': INCLUDE_DESC | INCLUDE_VALUE,
    'slider': INCLUDE_DESC | INCLUDE_VALUE,
    'spinbutton': INCLUDE_DESC | INCLUDE_VALUE,
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
    'entry': INCLUDE_DESC | INCLUDE_NAME | INCLUDE_VALUE,
    'caption': INCLUDE_DESC,
    'document frame': INCLUDE_DESC,
    'heading': INCLUDE_DESC,
    'calendar': INCLUDE_DESC | INCLUDE_NAME,
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
    'definitionlist': INCLUDE_DESC | INCLUDE_NAME,
    'dialog': INCLUDE_DESC | INCLUDE_NAME,
    'chrome window': IGNORE_EXPLICIT_NAME,
    'app root': IGNORE_EXPLICIT_NAME,
    'statusbar': NAME_FROM_SUBTREE_RULE,
    'mathml table': INCLUDE_DESC | INCLUDE_NAME,
    'mathml labeled row': NAME_FROM_SUBTREE_RULE,
    'mathml table row': NAME_FROM_SUBTREE_RULE,
    'mathml cell': INCLUDE_DESC | INCLUDE_NAME,
    'mathml fraction': INCLUDE_DESC,
    'mathml square root': INCLUDE_DESC,
    'mathml root': INCLUDE_DESC,
    'mathml enclosed': INCLUDE_DESC,
    'mathml sub': INCLUDE_DESC,
    'mathml sup': INCLUDE_DESC,
    'mathml sub sup': INCLUDE_DESC,
    'mathml under': INCLUDE_DESC,
    'mathml over': INCLUDE_DESC,
    'mathml under over': INCLUDE_DESC,
    'mathml multiscripts': INCLUDE_DESC,
    'mathml identifier': INCLUDE_DESC,
    'mathml number': INCLUDE_DESC,
    'mathml operator': INCLUDE_DESC,
    'mathml text': INCLUDE_DESC,
    'mathml string literal': INCLUDE_DESC,
    'mathml row': INCLUDE_DESC,
    'mathml style': INCLUDE_DESC,
    'mathml error': INCLUDE_DESC },

  mathmlRolesSet: new Set([
    Roles.MATHML_MATH,
    Roles.MATHML_IDENTIFIER,
    Roles.MATHML_NUMBER,
    Roles.MATHML_OPERATOR,
    Roles.MATHML_TEXT,
    Roles.MATHML_STRING_LITERAL,
    Roles.MATHML_GLYPH,
    Roles.MATHML_ROW,
    Roles.MATHML_FRACTION,
    Roles.MATHML_SQUARE_ROOT,
    Roles.MATHML_ROOT,
    Roles.MATHML_FENCED,
    Roles.MATHML_ENCLOSED,
    Roles.MATHML_STYLE,
    Roles.MATHML_SUB,
    Roles.MATHML_SUP,
    Roles.MATHML_SUB_SUP,
    Roles.MATHML_UNDER,
    Roles.MATHML_OVER,
    Roles.MATHML_UNDER_OVER,
    Roles.MATHML_MULTISCRIPTS,
    Roles.MATHML_TABLE,
    Roles.LABELED_ROW,
    Roles.MATHML_TABLE_ROW,
    Roles.MATHML_CELL,
    Roles.MATHML_ACTION,
    Roles.MATHML_ERROR,
    Roles.MATHML_STACK,
    Roles.MATHML_LONG_DIVISION,
    Roles.MATHML_STACK_GROUP,
    Roles.MATHML_STACK_ROW,
    Roles.MATHML_STACK_CARRIES,
    Roles.MATHML_STACK_CARRY,
    Roles.MATHML_STACK_LINE
  ]),

  objectOutputFunctions: {
    _generateBaseOutput:
      function _generateBaseOutput(aAccessible, aRoleStr, aState, aFlags) {
        let output = [];

        if (aFlags & INCLUDE_DESC) {
          this._addState(output, aState, aRoleStr);
          this._addType(output, aAccessible, aRoleStr);
          this._addRole(output, aAccessible, aRoleStr);
        }

        if (aFlags & INCLUDE_VALUE && aAccessible.value.trim()) {
          output[this.outputOrder === OUTPUT_DESC_FIRST ? 'push' : 'unshift'](
            aAccessible.value);
        }

        this._addName(output, aAccessible, aFlags);
        this._addLandmark(output, aAccessible);

        return output;
      },

    label: function label(aAccessible, aRoleStr, aState, aFlags, aContext) {
      if (aContext.isNestedControl ||
          aContext.accessible == Utils.getEmbeddedControl(aAccessible)) {
        
        
        return [];
      }

      return this.objectOutputFunctions.defaultFunc.apply(this, arguments);
    },

    entry: function entry(aAccessible, aRoleStr, aState, aFlags) {
      let rolestr = aState.contains(States.MULTI_LINE) ? 'textarea' : 'entry';
      return this.objectOutputFunctions.defaultFunc.apply(
        this, [aAccessible, rolestr, aState, aFlags]);
    },

    pagetab: function pagetab(aAccessible, aRoleStr, aState, aFlags) {
      let itemno = {};
      let itemof = {};
      aAccessible.groupPosition({}, itemof, itemno);
      let output = [];
      this._addState(output, aState);
      this._addRole(output, aAccessible, aRoleStr);
      output.push({
        string: 'objItemOfN',
        args: [itemno.value, itemof.value]
      });

      this._addName(output, aAccessible, aFlags);
      this._addLandmark(output, aAccessible);

      return output;
    },

    table: function table(aAccessible, aRoleStr, aState, aFlags) {
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
        this._addRole(output, aAccessible, aRoleStr);
        output.push.call(output, {
          string: this._getOutputName('tblColumnInfo'),
          count: table.columnCount
        }, {
          string: this._getOutputName('tblRowInfo'),
          count: table.rowCount
        });
        this._addName(output, aAccessible, aFlags);
        this._addLandmark(output, aAccessible);
        return output;
      }
    },

    gridcell: function gridcell(aAccessible, aRoleStr, aState, aFlags) {
      let output = [];
      this._addState(output, aState);
      this._addName(output, aAccessible, aFlags);
      this._addLandmark(output, aAccessible);
      return output;
    },

    
    mathmltable: function mathmltable() {
      return this.objectOutputFunctions.table.apply(this, arguments);
    },

    mathmlcell: function mathmlcell() {
      return this.objectOutputFunctions.cell.apply(this, arguments);
    },

    mathmlenclosed: function mathmlenclosed(aAccessible, aRoleStr, aState,
                                            aFlags, aContext) {
      let output = this.objectOutputFunctions.defaultFunc.
        apply(this, [aAccessible, aRoleStr, aState, aFlags, aContext]);
      this._addMencloseNotations(output, aAccessible);
      return output;
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
    on: 'onAction',
    off: 'offAction',
    select: 'selectAction',
    unselect: 'unselectAction',
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
    return [{string: this.gActionMap[aActionName]}];
  },

  genForLiveRegion:
    function genForLiveRegion(aContext, aIsHide, aModifiedText) {
      let utterance = [];
      if (aIsHide) {
        utterance.push({string: 'hidden'});
      }
      return utterance.concat(aModifiedText || this.genForContext(aContext));
    },

  genForAnnouncement: function genForAnnouncement(aAnnouncement) {
    return [{
      string: aAnnouncement
    }];
  },

  genForTabStateChange: function genForTabStateChange(aObject, aTabState) {
    switch (aTabState) {
      case 'newtab':
        return [{string: 'tabNew'}];
      case 'loading':
        return [{string: 'tabLoading'}];
      case 'loaded':
        return [aObject.name, {string: 'tabLoaded'}];
      case 'loadstopped':
        return [{string: 'tabLoadStopped'}];
      case 'reload':
        return [{string: 'tabReload'}];
      default:
        return [];
    }
  },

  genForEditingMode: function genForEditingMode(aIsEditing) {
    return [{string: aIsEditing ? 'editingMode' : 'navigationMode'}];
  },

  objectOutputFunctions: {

    __proto__: OutputGenerator.objectOutputFunctions, 

    defaultFunc: function defaultFunc() {
      return this.objectOutputFunctions._generateBaseOutput.apply(
        this, arguments);
    },

    heading: function heading(aAccessible, aRoleStr, aState, aFlags) {
      let level = {};
      aAccessible.groupPosition(level, {}, {});
      let utterance = [{string: 'headingLevel', args: [level.value]}];

      this._addName(utterance, aAccessible, aFlags);
      this._addLandmark(utterance, aAccessible);

      return utterance;
    },

    listitem: function listitem(aAccessible, aRoleStr, aState, aFlags) {
      let itemno = {};
      let itemof = {};
      aAccessible.groupPosition({}, itemof, itemno);
      let utterance = [];
      if (itemno.value == 1) {
        
        utterance.push({string: 'listStart'});
      }
      else if (itemno.value == itemof.value) {
        
        utterance.push({string: 'listEnd'});
      }

      this._addName(utterance, aAccessible, aFlags);
      this._addLandmark(utterance, aAccessible);

      return utterance;
    },

    list: function list(aAccessible, aRoleStr, aState, aFlags) {
      return this._getListUtterance
        (aAccessible, aRoleStr, aFlags, aAccessible.childCount);
    },

    definitionlist:
      function definitionlist(aAccessible, aRoleStr, aState, aFlags) {
        return this._getListUtterance
          (aAccessible, aRoleStr, aFlags, aAccessible.childCount / 2);
      },

    application: function application(aAccessible, aRoleStr, aState, aFlags) {
      
      if (aAccessible.name != aAccessible.DOMNode.location) {
        return this.objectOutputFunctions.defaultFunc.apply(this,
          [aAccessible, aRoleStr, aState, aFlags]);
      }

      return [];
    },

    cell: function cell(aAccessible, aRoleStr, aState, aFlags, aContext) {
      let utterance = [];
      let cell = aContext.getCellInfo(aAccessible);
      if (cell) {
        let addCellChanged =
          function addCellChanged(aUtterance, aChanged, aString, aIndex) {
            if (aChanged) {
              aUtterance.push({string: aString, args: [aIndex + 1]});
            }
          };
        let addExtent = function addExtent(aUtterance, aExtent, aString) {
          if (aExtent > 1) {
            aUtterance.push({string: aString, args: [aExtent]});
          }
        };
        let addHeaders = function addHeaders(aUtterance, aHeaders) {
          if (aHeaders.length > 0) {
            aUtterance.push.apply(aUtterance, aHeaders);
          }
        };

        addCellChanged(utterance, cell.columnChanged, 'columnInfo',
          cell.columnIndex);
        addCellChanged(utterance, cell.rowChanged, 'rowInfo', cell.rowIndex);

        addExtent(utterance, cell.columnExtent, 'spansColumns');
        addExtent(utterance, cell.rowExtent, 'spansRows');

        addHeaders(utterance, cell.columnHeaders);
        addHeaders(utterance, cell.rowHeaders);
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
    },

    statictext: function statictext(aAccessible) {
      if (Utils.isListItemDecorator(aAccessible, true)) {
        return [];
      }

      return this.objectOutputFunctions.defaultFunc.apply(this, arguments);
    }
  },

  _getContextStart: function _getContextStart(aContext) {
    return aContext.newAncestry;
  },

  _addRole: function _addRole(aOutput, aAccessible, aRoleStr) {
    if (this.mathmlRolesSet.has(aAccessible.role)) {
      this._addMathRoles(aOutput, aAccessible, aRoleStr);
    } else {
      aOutput.push({string: this._getOutputName(aRoleStr)});
    }
  },

  _addState: function _addState(aOutput, aState, aRoleStr) {

    if (aState.contains(States.UNAVAILABLE)) {
      aOutput.push({string: 'stateUnavailable'});
    }

    if (aState.contains(States.READONLY)) {
      aOutput.push({string: 'stateReadonly'});
    }

    
    
    
    
    if ((Utils.AndroidSdkVersion < 16 || Utils.MozBuildApp === 'browser') &&
      aState.contains(States.CHECKABLE)) {
      let checked = aState.contains(States.CHECKED);
      let statetr;
      if (aRoleStr === 'switch') {
        statetr = checked ? 'stateOn' : 'stateOff';
      } else {
        statetr = checked ? 'stateChecked' : 'stateNotChecked';
      }
      aOutput.push({string: statetr});
    }

    if (aState.contains(States.PRESSED)) {
      aOutput.push({string: 'statePressed'});
    }

    if (aState.contains(States.EXPANDABLE)) {
      let statetr = aState.contains(States.EXPANDED) ?
        'stateExpanded' : 'stateCollapsed';
      aOutput.push({string: statetr});
    }

    if (aState.contains(States.REQUIRED)) {
      aOutput.push({string: 'stateRequired'});
    }

    if (aState.contains(States.TRAVERSED)) {
      aOutput.push({string: 'stateTraversed'});
    }

    if (aState.contains(States.HASPOPUP)) {
      aOutput.push({string: 'stateHasPopup'});
    }

    if (aState.contains(States.SELECTED)) {
      aOutput.push({string: 'stateSelected'});
    }
  },

  _getListUtterance:
    function _getListUtterance(aAccessible, aRoleStr, aFlags, aItemCount) {
      let utterance = [];
      this._addRole(utterance, aAccessible, aRoleStr);
      utterance.push({
        string: this._getOutputName('listItemsCount'),
        count: aItemCount
      });

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

    
    
    
    let addListitemIndicator = function addListitemIndicator(indicator = '*') {
      output.unshift(indicator);
    };

    if (acc.indexInParent === 1 &&
        acc.parent.role == Roles.LISTITEM &&
        acc.previousSibling.role == Roles.STATICTEXT) {
      if (acc.parent.parent && acc.parent.parent.DOMNode &&
          acc.parent.parent.DOMNode.nodeName == 'UL') {
        addListitemIndicator();
      } else {
        addListitemIndicator(acc.previousSibling.name.trim());
      }
    } else if (acc.role == Roles.LISTITEM && acc.firstChild &&
               acc.firstChild.role == Roles.STATICTEXT) {
      if (acc.parent.DOMNode.nodeName == 'UL') {
        addListitemIndicator();
      } else {
        addListitemIndicator(acc.firstChild.name.trim());
      }
    }

    return output;
  },

  objectOutputFunctions: {

    __proto__: OutputGenerator.objectOutputFunctions, 

    defaultFunc: function defaultFunc() {
      return this.objectOutputFunctions._generateBaseOutput.apply(
        this, arguments);
    },

    listitem: function listitem(aAccessible, aRoleStr, aState, aFlags) {
      let braille = [];

      this._addName(braille, aAccessible, aFlags);
      this._addLandmark(braille, aAccessible);

      return braille;
    },

    cell: function cell(aAccessible, aRoleStr, aState, aFlags, aContext) {
      let braille = [];
      let cell = aContext.getCellInfo(aAccessible);
      if (cell) {
        let addHeaders = function addHeaders(aBraille, aHeaders) {
          if (aHeaders.length > 0) {
            aBraille.push.apply(aBraille, aHeaders);
          }
        };

        braille.push({
          string: this._getOutputName('cellInfo'),
          args: [cell.columnIndex + 1, cell.rowIndex + 1]
        });

        addHeaders(braille, cell.columnHeaders);
        addHeaders(braille, cell.rowHeaders);
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

    statictext: function statictext(aAccessible) {
      
      
      if (Utils.isListItemDecorator(aAccessible)) {
        return [];
      }

      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    _useStateNotRole:
      function _useStateNotRole(aAccessible, aRoleStr, aState, aFlags) {
        let braille = [];
        this._addState(braille, aState, aRoleStr);
        this._addName(braille, aAccessible, aFlags);
        this._addLandmark(braille, aAccessible);

        return braille;
      },

    switch: function braille_generator_object_output_functions_switch() {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    checkbutton: function checkbutton() {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    radiobutton: function radiobutton() {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    },

    togglebutton: function togglebutton() {
      return this.objectOutputFunctions._useStateNotRole.apply(this, arguments);
    }
  },

  _getContextStart: function _getContextStart(aContext) {
    if (aContext.accessible.parent.role == Roles.LINK) {
      return [aContext.accessible.parent];
    }

    return [];
  },

  _getOutputName: function _getOutputName(aName) {
    return OutputGenerator._getOutputName(aName) + 'Abbr';
  },

  _addRole: function _addRole(aBraille, aAccessible, aRoleStr) {
    if (this.mathmlRolesSet.has(aAccessible.role)) {
      this._addMathRoles(aBraille, aAccessible, aRoleStr);
    } else {
      aBraille.push({string: this._getOutputName(aRoleStr)});
    }
  },

  _addState: function _addState(aBraille, aState, aRoleStr) {
    if (aState.contains(States.CHECKABLE)) {
      aBraille.push({
        string: aState.contains(States.CHECKED) ?
          this._getOutputName('stateChecked') :
          this._getOutputName('stateUnchecked')
      });
    }
    if (aRoleStr === 'toggle button') {
      aBraille.push({
        string: aState.contains(States.PRESSED) ?
          this._getOutputName('statePressed') :
          this._getOutputName('stateUnpressed')
      });
    }
  }
};
