



'use strict';

const Ci = Components.interfaces;
const Cu = Components.utils;

const EVENT_VIRTUALCURSOR_CHANGED = Ci.nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED;
const EVENT_STATE_CHANGE = Ci.nsIAccessibleEvent.EVENT_STATE_CHANGE;
const EVENT_SCROLLING_START = Ci.nsIAccessibleEvent.EVENT_SCROLLING_START;
const EVENT_TEXT_CARET_MOVED = Ci.nsIAccessibleEvent.EVENT_TEXT_CARET_MOVED;
const EVENT_TEXT_INSERTED = Ci.nsIAccessibleEvent.EVENT_TEXT_INSERTED;
const EVENT_TEXT_REMOVED = Ci.nsIAccessibleEvent.EVENT_TEXT_REMOVED;
const EVENT_FOCUS = Ci.nsIAccessibleEvent.EVENT_FOCUS;

const ROLE_INTERNAL_FRAME = Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME;
const ROLE_DOCUMENT = Ci.nsIAccessibleRole.ROLE_DOCUMENT;
const ROLE_CHROME_WINDOW = Ci.nsIAccessibleRole.ROLE_CHROME_WINDOW;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Services',
  'resource://gre/modules/Services.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Presentation',
  'resource://gre/modules/accessibility/Presentation.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'TraversalRules',
  'resource://gre/modules/accessibility/TraversalRules.jsm');

this.EXPORTED_SYMBOLS = ['EventManager'];

this.EventManager = function EventManager(aContentScope) {
  this.contentScope = aContentScope;
  this.addEventListener = this.contentScope.addEventListener.bind(
    this.contentScope);
  this.removeEventListener = this.contentScope.removeEventListener.bind(
    this.contentScope);
  this.sendMsgFunc = this.contentScope.sendAsyncMessage.bind(
    this.contentScope);
  this.webProgress = this.contentScope.docShell.
    QueryInterface(Ci.nsIInterfaceRequestor).
    getInterface(Ci.nsIWebProgress);
};

this.EventManager.prototype = {
  editState: {},

  start: function start() {
    try {
      if (!this._started) {
        Logger.info('EventManager.start', Utils.MozBuildApp);

        this._started = true;

        AccessibilityEventObserver.addListener(this);

        this.webProgress.addProgressListener(this,
          (Ci.nsIWebProgress.NOTIFY_STATE_ALL |
           Ci.nsIWebProgress.NOTIFY_LOCATION));
        this.addEventListener('scroll', this, true);
        this.addEventListener('resize', this, true);
        
        this.addEventListener('DOMActivate', this, true);
      }
      this.present(Presentation.tabStateChanged(null, 'newtab'));

    } catch (x) {
      Logger.logException(x, 'Failed to start EventManager');
    }
  },

  
  
  stop: function stop() {
    if (!this._started) {
      return;
    }
    Logger.info('EventManager.stop', Utils.MozBuildApp);
    AccessibilityEventObserver.removeListener(this);
    try {
      this.webProgress.removeProgressListener(this);
      this.removeEventListener('scroll', this, true);
      this.removeEventListener('resize', this, true);
      
      this.removeEventListener('DOMActivate', this, true);
    } catch (x) {
      
    } finally {
      this._started = false;
    }
  },

  handleEvent: function handleEvent(aEvent) {
    try {
      switch (aEvent.type) {
      case 'DOMActivate':
      {
        let activatedAcc =
          Utils.AccRetrieval.getAccessibleFor(aEvent.originalTarget);
        let [state, extState] = Utils.getStates(activatedAcc);

        
        
        
        if (state & Ci.nsIAccessibleStates.STATE_CHECKABLE)
          return;

        this.present(Presentation.actionInvoked(activatedAcc, 'click'));
        break;
      }
      case 'scroll':
      case 'resize':
      {
        
        let window = null;
        if (aEvent.target instanceof Ci.nsIDOMWindow)
          window = aEvent.target;
        else if (aEvent.target instanceof Ci.nsIDOMDocument)
          window = aEvent.target.defaultView;
        else if (aEvent.target instanceof Ci.nsIDOMElement)
          window = aEvent.target.ownerDocument.defaultView;
        this.present(Presentation.viewportChanged(window));
        break;
      }
      }
    } catch (x) {
      Logger.logException(x, 'Error handling DOM event');
    }
  },

  handleAccEvent: function handleAccEvent(aEvent) {
    if (Logger.logLevel >= Logger.DEBUG)
      Logger.debug('A11yEvent', Logger.eventToString(aEvent),
                   Logger.accessibleToString(aEvent.accessible));

    
    if (Utils.MozBuildApp == 'browser' &&
        aEvent.eventType != EVENT_VIRTUALCURSOR_CHANGED &&
        aEvent.accessibleDocument != Utils.CurrentContentDoc) {
      return;
    }

    switch (aEvent.eventType) {
      case EVENT_VIRTUALCURSOR_CHANGED:
      {
        let pivot = aEvent.accessible.
          QueryInterface(Ci.nsIAccessibleDocument).virtualCursor;
        let position = pivot.position;
        if (position && position.role == ROLE_INTERNAL_FRAME)
          break;
        let event = aEvent.
          QueryInterface(Ci.nsIAccessibleVirtualCursorChangeEvent);
        let reason = event.reason;

        if (this.editState.editing)
          aEvent.accessibleDocument.takeFocus();

        this.present(
          Presentation.pivotChanged(position, event.oldAccessible, reason));

        break;
      }
      case EVENT_STATE_CHANGE:
      {
        let event = aEvent.QueryInterface(Ci.nsIAccessibleStateChangeEvent);
        if (event.state == Ci.nsIAccessibleStates.STATE_CHECKED &&
            !(event.isExtraState)) {
          this.present(
            Presentation.
              actionInvoked(aEvent.accessible,
                            event.isEnabled ? 'check' : 'uncheck'));
        }
        break;
      }
      case EVENT_SCROLLING_START:
      {
        let vc = Utils.getVirtualCursor(aEvent.accessibleDocument);
        vc.moveNext(TraversalRules.Simple, aEvent.accessible, true);
        break;
      }
      case EVENT_TEXT_CARET_MOVED:
      {
        let acc = aEvent.accessible;
        let characterCount = acc.
          QueryInterface(Ci.nsIAccessibleText).characterCount;
        let caretOffset = aEvent.
          QueryInterface(Ci.nsIAccessibleCaretMoveEvent).caretOffset;

        
        let [,extState] = Utils.getStates(acc);
        let editState = {
          editing: !!(extState & Ci.nsIAccessibleStates.EXT_STATE_EDITABLE),
          multiline: !!(extState & Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE),
          atStart: caretOffset == 0,
          atEnd: caretOffset == characterCount
        };

        
        if (!editState.editing && editState.editing == this.editState.editing)
          break;

        if (editState.editing != this.editState.editing)
          this.present(Presentation.editingModeChanged(editState.editing));

        if (editState.editing != this.editState.editing ||
            editState.multiline != this.editState.multiline ||
            editState.atEnd != this.editState.atEnd ||
            editState.atStart != this.editState.atStart)
          this.sendMsgFunc("AccessFu:Input", editState);

        this.editState = editState;
        break;
      }
      case EVENT_TEXT_INSERTED:
      case EVENT_TEXT_REMOVED:
      {
        if (aEvent.isFromUserInput) {
          
          let event = aEvent.QueryInterface(Ci.nsIAccessibleTextChangeEvent);
          let isInserted = event.isInserted;
          let txtIface = aEvent.accessible.QueryInterface(Ci.nsIAccessibleText);

          let text = '';
          try {
            text = txtIface.
              getText(0, Ci.nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT);
          } catch (x) {
            
            
            if (txtIface.characterCount)
              throw x;
          }
          this.present(Presentation.textChanged(
                         isInserted, event.start, event.length,
                         text, event.modifiedText));
        }
        break;
      }
      case EVENT_FOCUS:
      {
        
        let acc = aEvent.accessible;
        let doc = aEvent.accessibleDocument;
        if (acc.role != ROLE_DOCUMENT && doc.role != ROLE_CHROME_WINDOW) {
          let vc = Utils.getVirtualCursor(doc);
          vc.moveNext(TraversalRules.Simple, acc, true);
        }
        break;
      }
    }
  },

  present: function present(aPresentationData) {
    this.sendMsgFunc("AccessFu:Present", aPresentationData);
  },

  onStateChange: function onStateChange(aWebProgress, aRequest, aStateFlags, aStatus) {
    let tabstate = '';

    let loadingState = Ci.nsIWebProgressListener.STATE_TRANSFERRING |
      Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let loadedState = Ci.nsIWebProgressListener.STATE_STOP |
      Ci.nsIWebProgressListener.STATE_IS_NETWORK;

    if ((aStateFlags & loadingState) == loadingState) {
      tabstate = 'loading';
    } else if ((aStateFlags & loadedState) == loadedState &&
               !aWebProgress.isLoadingDocument) {
      tabstate = 'loaded';
    }

    if (tabstate) {
      let docAcc = Utils.AccRetrieval.getAccessibleFor(aWebProgress.DOMWindow.document);
      this.present(Presentation.tabStateChanged(docAcc, tabstate));
    }
  },

  onProgressChange: function onProgressChange() {},

  onLocationChange: function onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    let docAcc = Utils.AccRetrieval.getAccessibleFor(aWebProgress.DOMWindow.document);
    this.present(Presentation.tabStateChanged(docAcc, 'newdoc'));
  },

  onStatusChange: function onStatusChange() {},

  onSecurityChange: function onSecurityChange() {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports,
                                         Ci.nsIObserver])
};

const AccessibilityEventObserver = {

  


  eventManagers: new WeakMap(),

  


  listenerCount: 0,

  


  started: false,

  


  start: function start() {
    if (this.started || this.listenerCount === 0) {
      return;
    }
    Services.obs.addObserver(this, 'accessible-event', false);
    this.started = true;
  },

  


  stop: function stop() {
    if (!this.started) {
      return;
    }
    Services.obs.removeObserver(this, 'accessible-event');
    
    this.eventManagers.clear();
    this.listenerCount = 0;
    this.started = false;
  },

  






  addListener: function addListener(aEventManager) {
    let content = aEventManager.contentScope.content;
    if (!this.eventManagers.has(content)) {
      this.listenerCount++;
    }
    this.eventManagers.set(content, aEventManager);
    
    Logger.debug('AccessibilityEventObserver.addListener. Total:',
      this.listenerCount);
    this.start();
  },

  






  removeListener: function removeListener(aEventManager) {
    let content = aEventManager.contentScope.content;
    if (!this.eventManagers.delete(content)) {
      return;
    }
    this.listenerCount--;
    Logger.debug('AccessibilityEventObserver.removeListener. Total:',
      this.listenerCount);
    if (this.listenerCount === 0) {
      
      
      this.stop();
    }
  },

  





  getListener: function getListener(content) {
    let eventManager = this.eventManagers.get(content);
    if (eventManager) {
      return eventManager;
    }
    let parent = content.parent;
    if (parent === content) {
      
      return null;
    }
    return this.getListener(parent);
  },

  


  observe: function observe(aSubject, aTopic, aData) {
    if (aTopic !== 'accessible-event') {
      return;
    }
    let event = aSubject.QueryInterface(Ci.nsIAccessibleEvent);
    if (!event.accessibleDocument) {
      Logger.warning(
        'AccessibilityEventObserver.observe: no accessible document:',
        Logger.eventToString(event), "accessible:",
        Logger.accessibleToString(event.accessible));
      return;
    }
    let content = event.accessibleDocument.window;
    
    let eventManager = this.getListener(content);
    if (!eventManager || !eventManager._started) {
      if (Utils.MozBuildApp === 'browser' &&
          !(content instanceof Ci.nsIDOMChromeWindow)) {
        Logger.warning(
          'AccessibilityEventObserver.observe: ignored event:',
          Logger.eventToString(event), "accessible:",
          Logger.accessibleToString(event.accessible), "document:",
          Logger.accessibleToString(event.accessibleDocument));
      }
      return;
    }
    try {
      eventManager.handleAccEvent(event);
    } catch (x) {
      Logger.logException(x, 'Error handing accessible event');
    } finally {
      return;
    }
  }
};
