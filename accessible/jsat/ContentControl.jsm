



let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, 'Services',
  'resource://gre/modules/Services.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles',
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'TraversalRules',
  'resource://gre/modules/accessibility/TraversalRules.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Presentation',
  'resource://gre/modules/accessibility/Presentation.jsm');

this.EXPORTED_SYMBOLS = ['ContentControl'];

const MOVEMENT_GRANULARITY_CHARACTER = 1;
const MOVEMENT_GRANULARITY_WORD = 2;
const MOVEMENT_GRANULARITY_PARAGRAPH = 8;

this.ContentControl = function ContentControl(aContentScope) {
  this._contentScope = Cu.getWeakReference(aContentScope);
  this._vcCache = new WeakMap();
  this._childMessageSenders = new WeakMap();
};

this.ContentControl.prototype = {
  messagesOfInterest: ['AccessFu:MoveCursor',
                       'AccessFu:ClearCursor',
                       'AccessFu:MoveToPoint',
                       'AccessFu:AutoMove',
                       'AccessFu:Activate',
                       'AccessFu:MoveCaret',
                       'AccessFu:MoveByGranularity'],

  start: function cc_start() {
    let cs = this._contentScope.get();
    for (let message of this.messagesOfInterest) {
      cs.addMessageListener(message, this);
    }
    cs.addEventListener('mousemove', this);
  },

  stop: function cc_stop() {
    let cs = this._contentScope.get();
    for (let message of this.messagesOfInterest) {
      cs.removeMessageListener(message, this);
    }
    cs.removeEventListener('mousemove', this);
  },

  get document() {
    return this._contentScope.get().content.document;
  },

  get window() {
    return this._contentScope.get().content;
  },

  get vc() {
    return Utils.getVirtualCursor(this.document);
  },

  receiveMessage: function cc_receiveMessage(aMessage) {
    Logger.debug(() => {
      return ['ContentControl.receiveMessage',
        aMessage.name,
        JSON.stringify(aMessage.json)];
    });

    
    this.cancelAutoMove();

    try {
      let func = this['handle' + aMessage.name.slice(9)]; 
      if (func) {
        func.bind(this)(aMessage);
      } else {
        Logger.warning('ContentControl: Unhandled message:', aMessage.name);
      }
    } catch (x) {
      Logger.logException(
        x, 'Error handling message: ' + JSON.stringify(aMessage.json));
    }
  },

  handleMoveCursor: function cc_handleMoveCursor(aMessage) {
    let origin = aMessage.json.origin;
    let action = aMessage.json.action;
    let adjustRange = aMessage.json.adjustRange;
    let vc = this.vc;

    if (origin != 'child' && this.sendToChild(vc, aMessage)) {
      
      return;
    }

    if (adjustRange && this.adjustRange(vc.position, action === 'moveNext')) {
      return;
    }

    let moved = vc[action](TraversalRules[aMessage.json.rule]);

    if (moved) {
      if (origin === 'child') {
        
        Utils.getMessageManager(aMessage.target).sendAsyncMessage(
          'AccessFu:ClearCursor', {});
      } else {
        
        
        let childAction = action;
        if (action === 'moveNext') {
          childAction = 'moveFirst';
        } else if (action === 'movePrevious') {
          childAction = 'moveLast';
        }

        
        
        this.sendToChild(vc, aMessage, { action: childAction }, true);
      }
    } else if (!this._childMessageSenders.has(aMessage.target) &&
               origin !== 'top') {
      
      
      this.sendToParent(aMessage);
    } else {
      this._contentScope.get().sendAsyncMessage('AccessFu:Present',
        Presentation.noMove(action));
    }
  },

  handleEvent: function cc_handleEvent(aEvent) {
    if (aEvent.type === 'mousemove') {
      this.handleMoveToPoint(
        { json: { x: aEvent.screenX, y: aEvent.screenY, rule: 'Simple' } });
    }
    if (!Utils.getMessageManager(aEvent.target)) {
      aEvent.preventDefault();
    } else {
      aEvent.target.focus();
    }
  },

  handleMoveToPoint: function cc_handleMoveToPoint(aMessage) {
    let [x, y] = [aMessage.json.x, aMessage.json.y];
    let rule = TraversalRules[aMessage.json.rule];

    let dpr = this.window.devicePixelRatio;
    this.vc.moveToPoint(rule, x * dpr, y * dpr, true);
  },

  handleClearCursor: function cc_handleClearCursor(aMessage) {
    let forwarded = this.sendToChild(this.vc, aMessage);
    this.vc.position = null;
    if (!forwarded) {
      this._contentScope.get().sendAsyncMessage('AccessFu:CursorCleared');
    }
    this.document.activeElement.blur();
  },

  handleAutoMove: function cc_handleAutoMove(aMessage) {
    this.autoMove(null, aMessage.json);
  },

  handleActivate: function cc_handleActivate(aMessage) {
    let activateAccessible = (aAccessible) => {
      Logger.debug(() => {
        return ['activateAccessible', Logger.accessibleToString(aAccessible)];
      });
      try {
        if (aMessage.json.activateIfKey &&
          !Utils.isActivatableOnFingerUp(aAccessible)) {
          
          return;
        }
      } catch (e) {
        
        return;
      }

      if (aAccessible.actionCount > 0) {
        aAccessible.doAction(0);
      } else {
        let control = Utils.getEmbeddedControl(aAccessible);
        if (control && control.actionCount > 0) {
          control.doAction(0);
        }

        
        
        
        
        let docAcc = Utils.AccRetrieval.getAccessibleFor(this.document);
        let docX = {}, docY = {}, docW = {}, docH = {};
        docAcc.getBounds(docX, docY, docW, docH);

        let objX = {}, objY = {}, objW = {}, objH = {};
        aAccessible.getBounds(objX, objY, objW, objH);

        let x = Math.round((objX.value - docX.value) + objW.value / 2);
        let y = Math.round((objY.value - docY.value) + objH.value / 2);

        let node = aAccessible.DOMNode || aAccessible.parent.DOMNode;

        for (let eventType of ['mousedown', 'mouseup']) {
          let evt = this.document.createEvent('MouseEvents');
          evt.initMouseEvent(eventType, true, true, this.window,
            x, y, 0, 0, 0, false, false, false, false, 0, null);
          node.dispatchEvent(evt);
        }
      }

      if (!Utils.isActivatableOnFingerUp(aAccessible)) {
        
        this._contentScope.get().sendAsyncMessage('AccessFu:Present',
          Presentation.actionInvoked(aAccessible, 'click'));
      }
    };

    let focusedAcc = Utils.AccRetrieval.getAccessibleFor(
      this.document.activeElement);
    if (focusedAcc && this.vc.position === focusedAcc
        && focusedAcc.role === Roles.ENTRY) {
      let accText = focusedAcc.QueryInterface(Ci.nsIAccessibleText);
      let oldOffset = accText.caretOffset;
      let newOffset = aMessage.json.offset;
      let text = accText.getText(0, accText.characterCount);

      if (newOffset >= 0 && newOffset <= accText.characterCount) {
        accText.caretOffset = newOffset;
      }

      this.presentCaretChange(text, oldOffset, accText.caretOffset);
      return;
    }

    
    let getActivatableDescendant = (aAccessible) => {
      if (aAccessible.actionCount > 0) {
        return aAccessible;
      }

      for (let acc = aAccessible.firstChild; acc; acc = acc.nextSibling) {
        let activatable = getActivatableDescendant(acc);
        if (activatable) {
          return activatable;
        }
      }

      return null;
    };

    let vc = this.vc;
    if (!this.sendToChild(vc, aMessage, null, true)) {
      let position = vc.position;
      activateAccessible(getActivatableDescendant(position) || position);
    }
  },

  adjustRange: function cc_adjustRange(aAccessible, aStepUp) {
    let acc = Utils.getEmbeddedControl(aAccessible) || aAccessible;
    try {
      acc.QueryInterface(Ci.nsIAccessibleValue);
    } catch (x) {
      
      return false;
    }

    let elem = acc.DOMNode;
    if (!elem) {
      return false;
    }

    if (elem.tagName === 'INPUT' && elem.type === 'range') {
      elem[aStepUp ? 'stepDown' : 'stepUp']();
      let evt = this.document.createEvent('UIEvent');
      evt.initEvent('change', true, true);
      elem.dispatchEvent(evt);
    } else {
      let evt = this.document.createEvent('KeyboardEvent');
      let keycode = aStepUp ? content.KeyEvent.DOM_VK_DOWN :
        content.KeyEvent.DOM_VK_UP;
      evt.initKeyEvent(
        "keypress", false, true, null, false, false, false, false, keycode, 0);
      elem.dispatchEvent(evt);
    }

    return true;
  },

  handleMoveByGranularity: function cc_handleMoveByGranularity(aMessage) {
    
    let direction = aMessage.json.direction;
    let granularity;

    switch(aMessage.json.granularity) {
      case MOVEMENT_GRANULARITY_CHARACTER:
        granularity = Ci.nsIAccessiblePivot.CHAR_BOUNDARY;
        break;
      case MOVEMENT_GRANULARITY_WORD:
        granularity = Ci.nsIAccessiblePivot.WORD_BOUNDARY;
        break;
      default:
        return;
    }

    if (direction === 'Previous') {
      this.vc.movePreviousByText(granularity);
    } else if (direction === 'Next') {
      this.vc.moveNextByText(granularity);
    }
  },

  presentCaretChange: function cc_presentCaretChange(
    aText, aOldOffset, aNewOffset) {
    if (aOldOffset !== aNewOffset) {
      let msg = Presentation.textSelectionChanged(aText, aNewOffset, aNewOffset,
        aOldOffset, aOldOffset, true);
      this._contentScope.get().sendAsyncMessage('AccessFu:Present', msg);
    }
  },

  handleMoveCaret: function cc_handleMoveCaret(aMessage) {
    let direction = aMessage.json.direction;
    let granularity = aMessage.json.granularity;
    let accessible = this.vc.position;
    let accText = accessible.QueryInterface(Ci.nsIAccessibleText);
    let oldOffset = accText.caretOffset;
    let text = accText.getText(0, accText.characterCount);

    let start = {}, end = {};
    if (direction === 'Previous' && !aMessage.json.atStart) {
      switch (granularity) {
        case MOVEMENT_GRANULARITY_CHARACTER:
          accText.caretOffset--;
          break;
        case MOVEMENT_GRANULARITY_WORD:
          accText.getTextBeforeOffset(accText.caretOffset,
            Ci.nsIAccessibleText.BOUNDARY_WORD_START, start, end);
          accText.caretOffset = end.value === accText.caretOffset ?
            start.value : end.value;
          break;
        case MOVEMENT_GRANULARITY_PARAGRAPH:
          let startOfParagraph = text.lastIndexOf('\n', accText.caretOffset - 1);
          accText.caretOffset = startOfParagraph !== -1 ? startOfParagraph : 0;
          break;
      }
    } else if (direction === 'Next' && !aMessage.json.atEnd) {
      switch (granularity) {
        case MOVEMENT_GRANULARITY_CHARACTER:
          accText.caretOffset++;
          break;
        case MOVEMENT_GRANULARITY_WORD:
          accText.getTextAtOffset(accText.caretOffset,
                                  Ci.nsIAccessibleText.BOUNDARY_WORD_END, start, end);
          accText.caretOffset = end.value;
          break;
        case MOVEMENT_GRANULARITY_PARAGRAPH:
          accText.caretOffset = text.indexOf('\n', accText.caretOffset + 1);
          break;
      }
    }

    this.presentCaretChange(text, oldOffset, accText.caretOffset);
  },

  getChildCursor: function cc_getChildCursor(aAccessible) {
    let acc = aAccessible || this.vc.position;
    if (Utils.isAliveAndVisible(acc) && acc.role === Roles.INTERNAL_FRAME) {
      let domNode = acc.DOMNode;
      let mm = this._childMessageSenders.get(domNode, null);
      if (!mm) {
        mm = Utils.getMessageManager(domNode);
        mm.addWeakMessageListener('AccessFu:MoveCursor', this);
        this._childMessageSenders.set(domNode, mm);
      }

      return mm;
    }

    return null;
  },

  sendToChild: function cc_sendToChild(aVirtualCursor, aMessage, aReplacer,
                                       aFocus) {
    let position = aVirtualCursor.position;
    let mm = this.getChildCursor(position);
    if (!mm) {
      return false;
    }

    if (aFocus) {
      position.takeFocus();
    }

    
    let newJSON = JSON.parse(JSON.stringify(aMessage.json));
    newJSON.origin = 'parent';
    for (let attr in aReplacer) {
      newJSON[attr] = aReplacer[attr];
    }

    mm.sendAsyncMessage(aMessage.name, newJSON);
    return true;
  },

  sendToParent: function cc_sendToParent(aMessage) {
    
    let newJSON = JSON.parse(JSON.stringify(aMessage.json));
    newJSON.origin = 'child';
    aMessage.target.sendAsyncMessage(aMessage.name, newJSON);
  },

  











  autoMove: function cc_autoMove(aAnchor, aOptions = {}) {
    this.cancelAutoMove();

    let moveFunc = () => {
      let vc = this.vc;
      let acc = aAnchor;
      let rule = aOptions.onScreenOnly ?
        TraversalRules.SimpleOnScreen : TraversalRules.Simple;
      let forcePresentFunc = () => {
        if (aOptions.forcePresent) {
          this._contentScope.get().sendAsyncMessage(
            'AccessFu:Present', Presentation.pivotChanged(
              vc.position, null, Ci.nsIAccessiblePivot.REASON_NONE,
              vc.startOffset, vc.endOffset, false));
        }
      };

      if (aOptions.noOpIfOnScreen &&
        Utils.isAliveAndVisible(vc.position, true)) {
        forcePresentFunc();
        return;
      }

      if (aOptions.moveToFocused) {
        acc = Utils.AccRetrieval.getAccessibleFor(
          this.document.activeElement) || acc;
      }

      let moved = false;
      let moveMethod = aOptions.moveMethod || 'moveNext'; 
      let moveFirstOrLast = moveMethod in ['moveFirst', 'moveLast'];
      if (!moveFirstOrLast || acc) {
        
        moved = vc[moveFirstOrLast ? 'moveNext' : moveMethod](rule, acc, true,
                                                              false);
      }
      if (moveFirstOrLast && !moved) {
        
        moved = vc[moveMethod](rule, false);
      }

      let sentToChild = this.sendToChild(vc, {
        name: 'AccessFu:AutoMove',
        json: {
          moveMethod: aOptions.moveMethod,
          moveToFocused: aOptions.moveToFocused,
          noOpIfOnScreen: true,
          forcePresent: true
        }
      }, null, true);

      if (!moved && !sentToChild) {
        forcePresentFunc();
      }
    };

    if (aOptions.delay) {
      this._autoMove = this.window.setTimeout(moveFunc, aOptions.delay);
    } else {
      moveFunc();
    }
  },

  cancelAutoMove: function cc_cancelAutoMove() {
    this.window.clearTimeout(this._autoMove);
    this._autoMove = 0;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupportsWeakReference,
    Ci.nsIMessageListener
  ])
};
