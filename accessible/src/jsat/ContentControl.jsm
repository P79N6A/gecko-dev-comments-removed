



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

this.ContentControl = function ContentControl(aContentScope) {
  this._contentScope = Cu.getWeakReference(aContentScope);
  this._vcCache = new WeakMap();
  this._childMessageSenders = new WeakMap();
};

this.ContentControl.prototype = {
  messagesOfInterest: ['AccessFu:MoveCursor',
                       'AccessFu:ClearCursor',
                       'AccessFu:MoveToPoint',
                       'AccessFu:AutoMove'],

  start: function ContentControl_start() {
    let cs = this._contentScope.get();
    for (let message of this.messagesOfInterest) {
      cs.addMessageListener(message, this);
    }
  },

  stop: function ContentControl_stop() {
    let cs = this._contentScope.get();
    for (let message of this.messagesOfInterest) {
      cs.removeMessageListener(message, this);
    }
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

  receiveMessage: function ContentControl_receiveMessage(aMessage) {
    Logger.debug(() => {
      return ['ContentControl.receiveMessage',
        this.document.location.toString(),
        JSON.stringify(aMessage.json)];
    });

    try {
      switch (aMessage.name) {
        case 'AccessFu:MoveCursor':
          this.handleMove(aMessage);
          break;
        case 'AccessFu:ClearCursor':
          this.handleClear(aMessage);
          break;
        case 'AccessFu:MoveToPoint':
          this.handleMoveToPoint(aMessage);
          break;
        case 'AccessFu:AutoMove':
          this.handleAutoMove(aMessage);
          break;
        default:
          break;
      }
    } catch (x) {
      Logger.logException(
        x, 'Error handling message: ' + JSON.stringify(aMessage.json));
    }
  },

  handleMove: function ContentControl_handleMove(aMessage) {
    let origin = aMessage.json.origin;
    let action = aMessage.json.action;
    let vc = this.vc;

    if (origin != 'child' && this.sendToChild(vc, aMessage)) {
      
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

        
        
        this.sendToChild(vc, aMessage, { action: childAction});
      }
    } else if (!this._childMessageSenders.has(aMessage.target)) {
      
      
      this.sendToParent(aMessage);
    }
  },

  handleMoveToPoint: function ContentControl_handleMoveToPoint(aMessage) {
    let [x, y] = [aMessage.json.x, aMessage.json.y];
    let rule = TraversalRules[aMessage.json.rule];
    let vc = this.vc;
    let win = this.window;

    let dpr = win.devicePixelRatio;
    this.vc.moveToPoint(rule, x * dpr, y * dpr, true);

    let delta = Utils.isContentProcess ?
      { x: x - win.mozInnerScreenX, y: y - win.mozInnerScreenY } : {};
    this.sendToChild(vc, aMessage, delta);
  },

  handleClear: function ContentControl_handleClear(aMessage) {
    this.sendToChild(this.vc, aMessage);
    this.vc.position = null;
  },

  handleAutoMove: function ContentControl_handleAutoMove(aMessage) {
    this.autoMove(null, aMessage.json);
  },

  getChildCursor: function ContentControl_getChildCursor(aAccessible) {
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

  sendToChild: function ContentControl_sendToChild(aVirtualCursor,
                                                   aMessage,
                                                   aReplacer) {
    let mm = this.getChildCursor(aVirtualCursor.position);
    if (!mm) {
      return false;
    }

    
    let newJSON = JSON.parse(JSON.stringify(aMessage.json));
    newJSON.origin = 'parent';
    for (let attr in aReplacer) {
      newJSON[attr] = aReplacer[attr];
    }

    mm.sendAsyncMessage(aMessage.name, newJSON);
    return true;
  },

  sendToParent: function ContentControl_sendToParent(aMessage) {
    
    let newJSON = JSON.parse(JSON.stringify(aMessage.json));
    newJSON.origin = 'child';
    aMessage.target.sendAsyncMessage(aMessage.name, newJSON);
  },

  











  autoMove: function ContentControl_autoMove(aAnchor, aOptions = {}) {
    let win = this.window;
    win.clearTimeout(this._autoMove);

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
              vc.startOffset, vc.endOffset));
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
        
        moved = vc[moveFirstOrLast ? 'moveNext' : moveMethod](rule, acc, true);
      }
      if (moveFirstOrLast && !moved) {
        
        moved = vc[moveMethod](rule);
      }

      let sentToChild = this.sendToChild(vc, {
        name: 'AccessFu:AutoMove',
        json: aOptions
      });

      if (!moved && !sentToChild) {
        forcePresentFunc();
      }
    };

    if (aOptions.delay) {
      this._autoMove = win.setTimeout(moveFunc, aOptions.delay);
    } else {
      moveFunc();
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupportsWeakReference,
    Ci.nsIMessageListener
  ])
};