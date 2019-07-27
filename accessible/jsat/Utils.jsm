







'use strict';

const {classes: Cc, utils: Cu, interfaces: Ci} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Services', 
  'resource://gre/modules/Services.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Rect', 
  'resource://gre/modules/Geometry.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles', 
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Events', 
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Relations', 
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'States', 
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'PluralForm', 
  'resource://gre/modules/PluralForm.jsm');

this.EXPORTED_SYMBOLS = ['Utils', 'Logger', 'PivotContext', 'PrefCache',  
                         'SettingCache'];

this.Utils = { 
  _buildAppMap: {
    '{3c2e2abc-06d4-11e1-ac3b-374f68613e61}': 'b2g',
    '{ec8030f7-c20a-464f-9b0e-13a3a9e97384}': 'browser',
    '{aa3c5121-dab2-40e2-81ca-7ea25febc110}': 'mobile/android',
    '{a23983c0-fd0e-11dc-95ff-0800200c9a66}': 'mobile/xul'
  },

  init: function Utils_init(aWindow) {
    if (this._win) {
      
      throw new Error('Only one top-level window could used with AccessFu');
    }
    this._win = Cu.getWeakReference(aWindow);
  },

  uninit: function Utils_uninit() {
    if (!this._win) {
      return;
    }
    delete this._win;
  },

  get win() {
    if (!this._win) {
      return null;
    }
    return this._win.get();
  },

  get winUtils() {
    let win = this.win;
    if (!win) {
      return null;
    }
    return win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(
      Ci.nsIDOMWindowUtils);
  },

  get AccRetrieval() {
    if (!this._AccRetrieval) {
      this._AccRetrieval = Cc['@mozilla.org/accessibleRetrieval;1'].
        getService(Ci.nsIAccessibleRetrieval);
    }

    return this._AccRetrieval;
  },

  set MozBuildApp(value) {
    this._buildApp = value;
  },

  get MozBuildApp() {
    if (!this._buildApp) {
      this._buildApp = this._buildAppMap[Services.appinfo.ID];
    }
    return this._buildApp;
  },

  get OS() {
    if (!this._OS) {
      this._OS = Services.appinfo.OS;
    }
    return this._OS;
  },

  get widgetToolkit() {
    if (!this._widgetToolkit) {
      this._widgetToolkit = Services.appinfo.widgetToolkit;
    }
    return this._widgetToolkit;
  },

  get ScriptName() {
    if (!this._ScriptName) {
      this._ScriptName =
        (Services.appinfo.processType == 2) ? 'AccessFuContent' : 'AccessFu';
    }
    return this._ScriptName;
  },

  get AndroidSdkVersion() {
    if (!this._AndroidSdkVersion) {
      if (Services.appinfo.OS == 'Android') {
        this._AndroidSdkVersion = Services.sysinfo.getPropertyAsInt32(
          'version');
      } else {
        
        this._AndroidSdkVersion = 16;
      }
    }
    return this._AndroidSdkVersion;
  },

  set AndroidSdkVersion(value) {
    
    this._AndroidSdkVersion = value;
  },

  get BrowserApp() {
    if (!this.win) {
      return null;
    }
    switch (this.MozBuildApp) {
      case 'mobile/android':
        return this.win.BrowserApp;
      case 'browser':
        return this.win.gBrowser;
      case 'b2g':
        return this.win.shell;
      default:
        return null;
    }
  },

  get CurrentBrowser() {
    if (!this.BrowserApp) {
      return null;
    }
    if (this.MozBuildApp == 'b2g') {
      return this.BrowserApp.contentBrowser;
    }
    return this.BrowserApp.selectedBrowser;
  },

  get CurrentContentDoc() {
    let browser = this.CurrentBrowser;
    return browser ? browser.contentDocument : null;
  },

  get AllMessageManagers() {
    let messageManagers = new Set();

    function collectLeafMessageManagers(mm) {
      for (let i = 0; i < mm.childCount; i++) {
        let childMM = mm.getChildAt(i);

        if ('sendAsyncMessage' in childMM) {
          messageManagers.add(childMM);
        } else {
          collectLeafMessageManagers(childMM);
        }
      }
    }

    collectLeafMessageManagers(this.win.messageManager);

    let document = this.CurrentContentDoc;

    if (document) {
      if (document.location.host === 'b2g') {
        
        let contentBrowser = this.win.content.shell.contentBrowser;
        messageManagers.add(this.getMessageManager(contentBrowser));
        document = contentBrowser.contentDocument;
      }

      let remoteframes = document.querySelectorAll('iframe');

      for (let i = 0; i < remoteframes.length; ++i) {
        let mm = this.getMessageManager(remoteframes[i]);
        if (mm) {
          messageManagers.add(mm);
        }
      }

    }

    return messageManagers;
  },

  get isContentProcess() {
    delete this.isContentProcess;
    this.isContentProcess =
      Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;
    return this.isContentProcess;
  },

  localize: function localize(aOutput) {
    let outputArray = Array.isArray(aOutput) ? aOutput : [aOutput];
    let localized =
      [this.stringBundle.get(details) for (details of outputArray)]; 
    
    let trimmed;
    return [trimmed for (word of localized) if (word && 
      (trimmed = word.trim()))]; 
  },

  get stringBundle() {
    delete this.stringBundle;
    let bundle = Services.strings.createBundle(
      'chrome://global/locale/AccessFu.properties');
    this.stringBundle = {
      get: function stringBundle_get(aDetails = {}) {
        if (!aDetails || typeof aDetails === 'string') {
          return aDetails;
        }
        let str = '';
        let string = aDetails.string;
        if (!string) {
          return str;
        }
        try {
          let args = aDetails.args;
          let count = aDetails.count;
          if (args) {
            str = bundle.formatStringFromName(string, args, args.length);
          } else {
            str = bundle.GetStringFromName(string);
          }
          if (count) {
            str = PluralForm.get(count, str);
            str = str.replace('#1', count);
          }
        } catch (e) {
          Logger.debug('Failed to get a string from a bundle for', string);
        } finally {
          return str;
        }
      }
    };
    return this.stringBundle;
  },

  getMessageManager: function getMessageManager(aBrowser) {
    try {
      return aBrowser.QueryInterface(Ci.nsIFrameLoaderOwner).
         frameLoader.messageManager;
    } catch (x) {
      return null;
    }
  },

  getState: function getState(aAccessibleOrEvent) {
    if (aAccessibleOrEvent instanceof Ci.nsIAccessibleStateChangeEvent) {
      return new State(
        aAccessibleOrEvent.isExtraState ? 0 : aAccessibleOrEvent.state,
        aAccessibleOrEvent.isExtraState ? aAccessibleOrEvent.state : 0);
    } else {
      let state = {};
      let extState = {};
      aAccessibleOrEvent.getState(state, extState);
      return new State(state.value, extState.value);
    }
  },

  getAttributes: function getAttributes(aAccessible) {
    let attributes = {};

    if (aAccessible && aAccessible.attributes) {
      let attributesEnum = aAccessible.attributes.enumerate();

      
      
      while (attributesEnum.hasMoreElements()) {
        let attribute = attributesEnum.getNext().QueryInterface(
          Ci.nsIPropertyElement);
        attributes[attribute.key] = attribute.value;
      }
    }

    return attributes;
  },

  getVirtualCursor: function getVirtualCursor(aDocument) {
    let doc = (aDocument instanceof Ci.nsIAccessible) ? aDocument :
      this.AccRetrieval.getAccessibleFor(aDocument);

    return doc.QueryInterface(Ci.nsIAccessibleDocument).virtualCursor;
  },

  getContentResolution: function _getContentResolution(aAccessible) {
    let resX = { value: 1 }, resY = { value: 1 };
    aAccessible.document.window.QueryInterface(
      Ci.nsIInterfaceRequestor).getInterface(
      Ci.nsIDOMWindowUtils).getResolution(resX, resY);
    return [resX.value, resY.value];
  },

  getBounds: function getBounds(aAccessible, aPreserveContentScale) {
    let objX = {}, objY = {}, objW = {}, objH = {};
    aAccessible.getBounds(objX, objY, objW, objH);

    let [scaleX, scaleY] = aPreserveContentScale ? [1, 1] :
      this.getContentResolution(aAccessible);

    return new Rect(objX.value, objY.value, objW.value, objH.value).scale(
      scaleX, scaleY);
  },

  getTextBounds: function getTextBounds(aAccessible, aStart, aEnd,
                                        aPreserveContentScale) {
    let accText = aAccessible.QueryInterface(Ci.nsIAccessibleText);
    let objX = {}, objY = {}, objW = {}, objH = {};
    accText.getRangeExtents(aStart, aEnd, objX, objY, objW, objH,
      Ci.nsIAccessibleCoordinateType.COORDTYPE_SCREEN_RELATIVE);

    let [scaleX, scaleY] = aPreserveContentScale ? [1, 1] :
      this.getContentResolution(aAccessible);

    return new Rect(objX.value, objY.value, objW.value, objH.value).scale(
      scaleX, scaleY);
  },

  


  get dpi() {
    delete this.dpi;
    this.dpi = this.winUtils.displayDPI;
    return this.dpi;
  },

  isInSubtree: function isInSubtree(aAccessible, aSubTreeRoot) {
    let acc = aAccessible;
    while (acc) {
      if (acc == aSubTreeRoot) {
        return true;
      }

      try {
        acc = acc.parent;
      } catch (x) {
        Logger.debug('Failed to get parent:', x);
        acc = null;
      }
    }

    return false;
  },

  isHidden: function isHidden(aAccessible) {
    
    
    let hidden = Utils.getAttributes(aAccessible).hidden;
    return hidden && hidden === 'true';
  },

  visibleChildCount: function visibleChildCount(aAccessible) {
    let count = 0;
    for (let child = aAccessible.firstChild; child; child = child.nextSibling) {
      if (!this.isHidden(child)) {
        ++count;
      }
    }
    return count;
  },

  inHiddenSubtree: function inHiddenSubtree(aAccessible) {
    for (let acc=aAccessible; acc; acc=acc.parent) {
      if (this.isHidden(acc)) {
        return true;
      }
    }
    return false;
  },

  isAliveAndVisible: function isAliveAndVisible(aAccessible, aIsOnScreen) {
    if (!aAccessible) {
      return false;
    }

    try {
      let state = this.getState(aAccessible);
      if (state.contains(States.DEFUNCT) || state.contains(States.INVISIBLE) ||
          (aIsOnScreen && state.contains(States.OFFSCREEN)) ||
          Utils.inHiddenSubtree(aAccessible)) {
        return false;
      }
    } catch (x) {
      return false;
    }

    return true;
  },

  matchAttributeValue: function matchAttributeValue(aAttributeValue, values) {
    let attrSet = new Set(aAttributeValue.split(' '));
    for (let value of values) {
      if (attrSet.has(value)) {
        return value;
      }
    }
  },

  getLandmarkName: function getLandmarkName(aAccessible) {
    const landmarks = [
      'banner',
      'complementary',
      'contentinfo',
      'main',
      'navigation',
      'search'
    ];
    let roles = this.getAttributes(aAccessible)['xml-roles'];
    if (!roles) {
      return;
    }

    
    return this.matchAttributeValue(roles, landmarks);
  },

  getEmbeddedControl: function getEmbeddedControl(aLabel) {
    if (aLabel) {
      let relation = aLabel.getRelationByType(Relations.LABEL_FOR);
      for (let i = 0; i < relation.targetsCount; i++) {
        let target = relation.getTarget(i);
        if (target.parent === aLabel) {
          return target;
        }
      }
    }

    return null;
  },

  isListItemDecorator: function isListItemDecorator(aStaticText,
                                                    aExcludeOrdered) {
    let parent = aStaticText.parent;
    if (aExcludeOrdered && parent.parent.DOMNode.nodeName === 'OL') {
      return false;
    }

    return parent.role === Roles.LISTITEM && parent.childCount > 1 &&
      aStaticText.indexInParent === 0;
  },

  dispatchChromeEvent: function dispatchChromeEvent(aType, aDetails) {
    let details = {
      type: aType,
      details: JSON.stringify(
        typeof aDetails === 'string' ? { eventType : aDetails } : aDetails)
    };
    let window = this.win;
    let shell = window.shell || window.content.shell;
    if (shell) {
      
      shell.sendChromeEvent(details);
    } else {
      
      
      window.dispatchEvent(new window.CustomEvent(aType, {
        bubbles: true,
        cancelable: true,
        detail: details
      }));
    }

  },

  isActivatableOnFingerUp: function isActivatableOnFingerUp(aAccessible) {
    if (aAccessible.role === Roles.KEY) {
      return true;
    }
    let quick_activate = this.getAttributes(aAccessible)['moz-quick-activate'];
    return quick_activate && JSON.parse(quick_activate);
  }
};






function State(aBase, aExtended) {
  this.base = aBase;
  this.extended = aExtended;
}

State.prototype = {
  contains: function State_contains(other) {
    return !!(this.base & other.base || this.extended & other.extended);
  },
  toString: function State_toString() {
    let stateStrings = Utils.AccRetrieval.
      getStringStates(this.base, this.extended);
    let statesArray = new Array(stateStrings.length);
    for (let i = 0; i < statesArray.length; i++) {
      statesArray[i] = stateStrings.item(i);
    }
    return '[' + statesArray.join(', ') + ']';
  }
};

this.Logger = { 
  GESTURE: -1,
  DEBUG: 0,
  INFO: 1,
  WARNING: 2,
  ERROR: 3,
  _LEVEL_NAMES: ['GESTURE', 'DEBUG', 'INFO', 'WARNING', 'ERROR'],

  logLevel: 1, 

  test: false,

  log: function log(aLogLevel) {
    if (aLogLevel < this.logLevel) {
      return;
    }

    let args = Array.prototype.slice.call(arguments, 1);
    let message = (typeof(args[0]) === 'function' ? args[0]() : args).join(' ');
    message = '[' + Utils.ScriptName + '] ' + this._LEVEL_NAMES[aLogLevel + 1] +
      ' ' + message + '\n';
    dump(message);
    
    
    if (this.test) {
      try {
        Services.console.logStringMessage(message);
      } catch (ex) {
        
      }
    }
  },

  info: function info() {
    this.log.apply(
      this, [this.INFO].concat(Array.prototype.slice.call(arguments)));
  },

  gesture: function gesture() {
    this.log.apply(
      this, [this.GESTURE].concat(Array.prototype.slice.call(arguments)));
  },

  debug: function debug() {
    this.log.apply(
      this, [this.DEBUG].concat(Array.prototype.slice.call(arguments)));
  },

  warning: function warning() {
    this.log.apply(
      this, [this.WARNING].concat(Array.prototype.slice.call(arguments)));
  },

  error: function error() {
    this.log.apply(
      this, [this.ERROR].concat(Array.prototype.slice.call(arguments)));
  },

  logException: function logException(
    aException, aErrorMessage = 'An exception has occured') {
    try {
      let stackMessage = '';
      if (aException.stack) {
        stackMessage = '  ' + aException.stack.replace(/\n/g, '\n  ');
      } else if (aException.location) {
        let frame = aException.location;
        let stackLines = [];
        while (frame && frame.lineNumber) {
          stackLines.push(
            '  ' + frame.name + '@' + frame.filename + ':' + frame.lineNumber);
          frame = frame.caller;
        }
        stackMessage = stackLines.join('\n');
      } else {
        stackMessage =
          '(' + aException.fileName + ':' + aException.lineNumber + ')';
      }
      this.error(aErrorMessage + ':\n ' +
                 aException.message + '\n' +
                 stackMessage);
    } catch (x) {
      this.error(x);
    }
  },

  accessibleToString: function accessibleToString(aAccessible) {
    if (!aAccessible) {
      return '[ null ]';
    }

    try {
      return'[ ' + Utils.AccRetrieval.getStringRole(aAccessible.role) +
        ' | ' + aAccessible.name + ' ]';
    } catch (x) {
      return '[ defunct ]';
    }
  },

  eventToString: function eventToString(aEvent) {
    let str = Utils.AccRetrieval.getStringEventType(aEvent.eventType);
    if (aEvent.eventType == Events.STATE_CHANGE) {
      let event = aEvent.QueryInterface(Ci.nsIAccessibleStateChangeEvent);
      let stateStrings = event.isExtraState ?
        Utils.AccRetrieval.getStringStates(0, event.state) :
        Utils.AccRetrieval.getStringStates(event.state, 0);
      str += ' (' + stateStrings.item(0) + ')';
    }

    if (aEvent.eventType == Events.VIRTUALCURSOR_CHANGED) {
      let event = aEvent.QueryInterface(
        Ci.nsIAccessibleVirtualCursorChangeEvent);
      let pivot = aEvent.accessible.QueryInterface(
        Ci.nsIAccessibleDocument).virtualCursor;
      str += ' (' + this.accessibleToString(event.oldAccessible) + ' -> ' +
	this.accessibleToString(pivot.position) + ')';
    }

    return str;
  },

  statesToString: function statesToString(aAccessible) {
    return Utils.getState(aAccessible).toString();
  },

  dumpTree: function dumpTree(aLogLevel, aRootAccessible) {
    if (aLogLevel < this.logLevel) {
      return;
    }

    this._dumpTreeInternal(aLogLevel, aRootAccessible, 0);
  },

  _dumpTreeInternal:
    function _dumpTreeInternal(aLogLevel, aAccessible, aIndent) {
      let indentStr = '';
      for (let i = 0; i < aIndent; i++) {
        indentStr += ' ';
      }
      this.log(aLogLevel, indentStr,
               this.accessibleToString(aAccessible),
               '(' + this.statesToString(aAccessible) + ')');
      for (let i = 0; i < aAccessible.childCount; i++) {
        this._dumpTreeInternal(aLogLevel, aAccessible.getChildAt(i),
          aIndent + 1);
      }
    }
};











this.PivotContext = function PivotContext(aAccessible, aOldAccessible, 
  aStartOffset, aEndOffset, aIgnoreAncestry = false,
  aIncludeInvisible = false) {
  this._accessible = aAccessible;
  this._nestedControl = Utils.getEmbeddedControl(aAccessible);
  this._oldAccessible =
    this._isDefunct(aOldAccessible) ? null : aOldAccessible;
  this.startOffset = aStartOffset;
  this.endOffset = aEndOffset;
  this._ignoreAncestry = aIgnoreAncestry;
  this._includeInvisible = aIncludeInvisible;
};

PivotContext.prototype = {
  get accessible() {
    
    
    return this._nestedControl || this._accessible;
  },

  get oldAccessible() {
    return this._oldAccessible;
  },

  get isNestedControl() {
    return !!this._nestedControl;
  },

  get accessibleForBounds() {
    return this._accessible;
  },

  get textAndAdjustedOffsets() {
    if (this.startOffset === -1 && this.endOffset === -1) {
      return null;
    }

    if (!this._textAndAdjustedOffsets) {
      let result = {startOffset: this.startOffset,
                    endOffset: this.endOffset,
                    text: this._accessible.QueryInterface(Ci.nsIAccessibleText).
                          getText(0,
                            Ci.nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT)};
      let hypertextAcc = this._accessible.QueryInterface(
        Ci.nsIAccessibleHyperText);

      
      
      for (let i = hypertextAcc.linkCount - 1; i >= 0; i--) {
        let link = hypertextAcc.getLinkAt(i);
        let linkText = '';
        if (link instanceof Ci.nsIAccessibleText) {
          linkText = link.QueryInterface(Ci.nsIAccessibleText).
                          getText(0,
                            Ci.nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT);
        }

        let start = link.startIndex;
        let end = link.endIndex;
        for (let offset of ['startOffset', 'endOffset']) {
          if (this[offset] >= end) {
            result[offset] += linkText.length - (end - start);
          }
        }
        result.text = result.text.substring(0, start) + linkText +
                      result.text.substring(end);
      }

      this._textAndAdjustedOffsets = result;
    }

    return this._textAndAdjustedOffsets;
  },

  




  _getAncestry: function _getAncestry(aAccessible) {
    let ancestry = [];
    let parent = aAccessible;
    try {
      while (parent && (parent = parent.parent)) {
       ancestry.push(parent);
      }
    } catch (x) {
      
      Logger.debug('Failed to get parent:', x);
    }
    return ancestry.reverse();
  },

  


  get oldAncestry() {
    if (!this._oldAncestry) {
      if (!this._oldAccessible || this._ignoreAncestry) {
        this._oldAncestry = [];
      } else {
        this._oldAncestry = this._getAncestry(this._oldAccessible);
        this._oldAncestry.push(this._oldAccessible);
      }
    }
    return this._oldAncestry;
  },

  


  get currentAncestry() {
    if (!this._currentAncestry) {
      this._currentAncestry = this._ignoreAncestry ? [] :
        this._getAncestry(this.accessible);
    }
    return this._currentAncestry;
  },

  




  get newAncestry() {
    if (!this._newAncestry) {
      this._newAncestry = this._ignoreAncestry ? [] : [currentAncestor for ( 
        [index, currentAncestor] of Iterator(this.currentAncestry)) if ( 
          currentAncestor !== this.oldAncestry[index])]; 
    }
    return this._newAncestry;
  },

  





  _traverse: function* _traverse(aAccessible, aPreorder, aStop) {
    if (aStop && aStop(aAccessible)) {
      return;
    }
    let child = aAccessible.firstChild;
    while (child) {
      let include;
      if (this._includeInvisible) {
        include = true;
      } else {
        include = !Utils.isHidden(child);
      }
      if (include) {
        if (aPreorder) {
          yield child;
          [yield node for (node of this._traverse(child, aPreorder, aStop))]; 
        } else {
          [yield node for (node of this._traverse(child, aPreorder, aStop))]; 
          yield child;
        }
      }
      child = child.nextSibling;
    }
  },

  



  get interactionHints() {
    let hints = [];
    this.newAncestry.concat(this.accessible).reverse().forEach(aAccessible => {
      let hint = Utils.getAttributes(aAccessible)['moz-hint'];
      if (hint) {
        hints.push(hint);
      } else if (aAccessible.actionCount > 0) {
        hints.push({
          string: Utils.AccRetrieval.getStringRole(aAccessible.role) + '-hint'
        });
      }
    });
    return hints;
  },

  








  subtreeGenerator: function subtreeGenerator(aPreorder, aStop) {
    return this._traverse(this.accessible, aPreorder, aStop);
  },

  getCellInfo: function getCellInfo(aAccessible) {
    if (!this._cells) {
      this._cells = new WeakMap();
    }

    let domNode = aAccessible.DOMNode;
    if (this._cells.has(domNode)) {
      return this._cells.get(domNode);
    }

    let cellInfo = {};
    let getAccessibleCell = function getAccessibleCell(aAccessible) {
      if (!aAccessible) {
        return null;
      }
      if ([Roles.CELL, Roles.COLUMNHEADER, Roles.ROWHEADER].indexOf(
        aAccessible.role) < 0) {
          return null;
      }
      try {
        return aAccessible.QueryInterface(Ci.nsIAccessibleTableCell);
      } catch (x) {
        Logger.logException(x);
        return null;
      }
    };
    let getHeaders = function* getHeaders(aHeaderCells) {
      let enumerator = aHeaderCells.enumerate();
      while (enumerator.hasMoreElements()) {
        yield enumerator.getNext().QueryInterface(Ci.nsIAccessible).name;
      }
    };

    cellInfo.current = getAccessibleCell(aAccessible);

    if (!cellInfo.current) {
      Logger.warning(aAccessible,
        'does not support nsIAccessibleTableCell interface.');
      this._cells.set(domNode, null);
      return null;
    }

    let table = cellInfo.current.table;
    if (table.isProbablyForLayout()) {
      this._cells.set(domNode, null);
      return null;
    }

    cellInfo.previous = null;
    let oldAncestry = this.oldAncestry.reverse();
    let ancestor = oldAncestry.shift();
    while (!cellInfo.previous && ancestor) {
      let cell = getAccessibleCell(ancestor);
      if (cell && cell.table === table) {
        cellInfo.previous = cell;
      }
      ancestor = oldAncestry.shift();
    }

    if (cellInfo.previous) {
      cellInfo.rowChanged = cellInfo.current.rowIndex !==
        cellInfo.previous.rowIndex;
      cellInfo.columnChanged = cellInfo.current.columnIndex !==
        cellInfo.previous.columnIndex;
    } else {
      cellInfo.rowChanged = true;
      cellInfo.columnChanged = true;
    }

    cellInfo.rowExtent = cellInfo.current.rowExtent;
    cellInfo.columnExtent = cellInfo.current.columnExtent;
    cellInfo.columnIndex = cellInfo.current.columnIndex;
    cellInfo.rowIndex = cellInfo.current.rowIndex;

    cellInfo.columnHeaders = [];
    if (cellInfo.columnChanged && cellInfo.current.role !==
      Roles.COLUMNHEADER) {
      cellInfo.columnHeaders = [headers for (headers of getHeaders( 
        cellInfo.current.columnHeaderCells))];
    }
    cellInfo.rowHeaders = [];
    if (cellInfo.rowChanged && cellInfo.current.role === Roles.CELL) {
      cellInfo.rowHeaders = [headers for (headers of getHeaders( 
        cellInfo.current.rowHeaderCells))];
    }

    this._cells.set(domNode, cellInfo);
    return cellInfo;
  },

  get bounds() {
    if (!this._bounds) {
      this._bounds = Utils.getBounds(this.accessibleForBounds);
    }

    return this._bounds.clone();
  },

  _isDefunct: function _isDefunct(aAccessible) {
    try {
      return Utils.getState(aAccessible).contains(States.DEFUNCT);
    } catch (x) {
      return true;
    }
  }
};

this.PrefCache = function PrefCache(aName, aCallback, aRunCallbackNow) { 
  this.name = aName;
  this.callback = aCallback;

  let branch = Services.prefs;
  this.value = this._getValue(branch);

  if (this.callback && aRunCallbackNow) {
    try {
      this.callback(this.name, this.value, true);
    } catch (x) {
      Logger.logException(x);
    }
  }

  branch.addObserver(aName, this, true);
};

PrefCache.prototype = {
  _getValue: function _getValue(aBranch) {
    try {
      if (!this.type) {
        this.type = aBranch.getPrefType(this.name);
      }
      switch (this.type) {
        case Ci.nsIPrefBranch.PREF_STRING:
          return aBranch.getCharPref(this.name);
        case Ci.nsIPrefBranch.PREF_INT:
          return aBranch.getIntPref(this.name);
        case Ci.nsIPrefBranch.PREF_BOOL:
          return aBranch.getBoolPref(this.name);
        default:
          return null;
      }
    } catch (x) {
      
      return null;
    }
  },

  observe: function observe(aSubject) {
    this.value = this._getValue(aSubject.QueryInterface(Ci.nsIPrefBranch));
    Logger.info('pref changed', this.name, this.value);
    if (this.callback) {
      try {
        this.callback(this.name, this.value, false);
      } catch (x) {
        Logger.logException(x);
      }
    }
  },

  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsISupportsWeakReference])
};

this.SettingCache = function SettingCache(aName, aCallback, aOptions = {}) { 
  this.value = aOptions.defaultValue;
  let runCallback = () => {
    if (aCallback) {
      aCallback(aName, this.value);
      if (aOptions.callbackOnce) {
        runCallback = () => {};
      }
    }
  };

  let settings = Utils.win.navigator.mozSettings;
  if (!settings) {
    if (aOptions.callbackNow) {
      runCallback();
    }
    return;
  }


  let lock = settings.createLock();
  let req = lock.get(aName);

  req.addEventListener('success', () => {
    this.value = req.result[aName] === undefined ?
      aOptions.defaultValue : req.result[aName];
    if (aOptions.callbackNow) {
      runCallback();
    }
  });

  settings.addObserver(aName,
                       (evt) => {
                         this.value = evt.settingValue;
                         runCallback();
                       });
};
