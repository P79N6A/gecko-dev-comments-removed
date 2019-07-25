



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ['AccessFu'];

Cu.import('resource://gre/modules/Services.jsm');

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/Presenters.jsm');
Cu.import('resource://gre/modules/accessibility/VirtualCursorController.jsm');
Cu.import('resource://gre/modules/accessibility/TouchAdapter.jsm');

const ACCESSFU_DISABLE = 0;
const ACCESSFU_ENABLE = 1;
const ACCESSFU_AUTO = 2;

var AccessFu = {
  







  attach: function attach(aWindow) {
    if (this.chromeWin)
      
      throw new Error('Only one window could be attached to AccessFu');

    Logger.info('attach');
    this.chromeWin = aWindow;
    this.presenters = [];

    this.prefsBranch = Cc['@mozilla.org/preferences-service;1']
      .getService(Ci.nsIPrefService).getBranch('accessibility.accessfu.');
    this.prefsBranch.addObserver('activate', this, false);
    this.prefsBranch.addObserver('explorebytouch', this, false);

    this.touchAdapter = TouchAdapter;

    switch(Utils.MozBuildApp) {
      case 'mobile/android':
        Services.obs.addObserver(this, 'Accessibility:Settings', false);
        this.touchAdapter = AndroidTouchAdapter;
        break;
      case 'b2g':
        aWindow.addEventListener(
          'ContentStart',
          (function(event) {
             let content = aWindow.shell.contentBrowser.contentWindow;
             content.addEventListener('mozContentEvent', this, false, true);
           }).bind(this), false);
        break;
      default:
        break;
    }

    this._processPreferences();
  },

  



  _enable: function _enable() {
    if (this._enabled)
      return;
    this._enabled = true;

    Logger.info('enable');

    
    let stylesheetURL = 'chrome://global/content/accessibility/AccessFu.css';
    this.stylesheet = this.chromeWin.document.createProcessingInstruction(
      'xml-stylesheet', 'href="' + stylesheetURL + '" type="text/css"');
    this.chromeWin.document.insertBefore(this.stylesheet, this.chromeWin.document.firstChild);

    this.addPresenter(new VisualPresenter());

    
    if (Utils.MozBuildApp == 'mobile/android')
      this.addPresenter(new AndroidPresenter());
    else if (Utils.MozBuildApp == 'b2g')
      this.addPresenter(new SpeechPresenter());

    VirtualCursorController.attach(this.chromeWin);

    Services.obs.addObserver(this, 'accessible-event', false);
    this.chromeWin.addEventListener('DOMActivate', this, true);
    this.chromeWin.addEventListener('resize', this, true);
    this.chromeWin.addEventListener('scroll', this, true);
    this.chromeWin.addEventListener('TabOpen', this, true);
    this.chromeWin.addEventListener('focus', this, true);
  },

  


  _disable: function _disable() {
    if (!this._enabled)
      return;
    this._enabled = false;

    Logger.info('disable');

    this.chromeWin.document.removeChild(this.stylesheet);

    this.presenters.forEach(function(p) { p.detach(); });
    this.presenters = [];

    VirtualCursorController.detach();

    Services.obs.removeObserver(this, 'accessible-event');
    this.chromeWin.removeEventListener('DOMActivate', this, true);
    this.chromeWin.removeEventListener('resize', this, true);
    this.chromeWin.removeEventListener('scroll', this, true);
    this.chromeWin.removeEventListener('TabOpen', this, true);
    this.chromeWin.removeEventListener('focus', this, true);
  },

  _processPreferences: function _processPreferences(aEnabled, aTouchEnabled) {
    let accessPref = ACCESSFU_DISABLE;
    try {
      accessPref = (aEnabled == undefined) ?
        this.prefsBranch.getIntPref('activate') : aEnabled;
    } catch (x) {
    }

    let ebtPref = ACCESSFU_DISABLE;
    try {
      ebtPref = (aTouchEnabled == undefined) ?
        this.prefsBranch.getIntPref('explorebytouch') : aTouchEnabled;
    } catch (x) {
    }

    if (Utils.MozBuildApp == 'mobile/android') {
      if (accessPref == ACCESSFU_AUTO) {
        Cc['@mozilla.org/android/bridge;1'].
          getService(Ci.nsIAndroidBridge).handleGeckoMessage(
            JSON.stringify({ gecko: { type: 'Accessibility:Ready' } }));
        return;
      }
    }

    if (accessPref == ACCESSFU_ENABLE)
      this._enable();
    else
      this._disable();

    if (ebtPref == ACCESSFU_ENABLE)
      this.touchAdapter.attach(this.chromeWin);
    else
      this.touchAdapter.detach(this.chromeWin);
  },

  addPresenter: function addPresenter(presenter) {
    this.presenters.push(presenter);
    presenter.attach(this.chromeWin);
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'focus':
      {
        if (aEvent.target instanceof Ci.nsIDOMWindow) {
          let docAcc = getAccessible(aEvent.target.document);
          let docContext = new PresenterContext(docAcc, null);
          let cursorable = docAcc.QueryInterface(Ci.nsIAccessibleCursorable);
          let vcContext = new PresenterContext(
            (cursorable) ? cursorable.virtualCursor.position : null, null);
          this.presenters.forEach(
            function(p) { p.tabSelected(docContext, vcContext); });
        }
        break;
      }
      case 'TabOpen':
      {
        let browser = aEvent.target.linkedBrowser || aEvent.target;
        
        
        
        
        this._pendingDocuments[browser] = true;
        this.presenters.forEach(
          function(p) {
            p.tabStateChanged(null, 'newtab');
          }
        );
        break;
      }
      case 'DOMActivate':
      {
        let activatedAcc = getAccessible(aEvent.originalTarget);
        let state = {};
        activatedAcc.getState(state, {});

        
        
        
        if (state.value & Ci.nsIAccessibleStates.STATE_CHECKABLE)
          return;

        this.presenters.forEach(function(p) {
                                  p.actionInvoked(activatedAcc, 'click');
                                });
        break;
      }
      case 'scroll':
      case 'resize':
      {
        this.presenters.forEach(function(p) { p.viewportChanged(); });
        break;
      }
      case 'mozContentEvent':
      {
        if (aEvent.detail.type == 'accessibility-screenreader') {
          let pref = aEvent.detail.enabled + 0;
          this._processPreferences(pref, pref);
        }
        break;
      }
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'Accessibility:Settings':
        this._processPreferences(JSON.parse(aData).enabled + 0,
                                 JSON.parse(aData).exploreByTouch + 0);
        break;
      case 'nsPref:changed':
        this._processPreferences(this.prefsBranch.getIntPref('activate'),
                                 this.prefsBranch.getIntPref('explorebytouch'));
        break;
      case 'accessible-event':
        let event;
        try {
          event = aSubject.QueryInterface(Ci.nsIAccessibleEvent);
          this._handleAccEvent(event);
        } catch (ex) {
          Logger.error(ex);
          return;
        }
    }
  },

  _handleAccEvent: function _handleAccEvent(aEvent) {
    if (Logger.logLevel <= Logger.DEBUG)
      Logger.debug(Logger.eventToString(aEvent),
                   Logger.accessibleToString(aEvent.accessible));

    switch (aEvent.eventType) {
      case Ci.nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED:
        {
          let pivot = aEvent.accessible.
            QueryInterface(Ci.nsIAccessibleCursorable).virtualCursor;
          let event = aEvent.
            QueryInterface(Ci.nsIAccessibleVirtualCursorChangeEvent);
          let position = pivot.position;
          let doc = aEvent.DOMNode;

          let presenterContext =
            new PresenterContext(position, event.oldAccessible);
          let reason = event.reason;
          this.presenters.forEach(
            function(p) { p.pivotChanged(presenterContext, reason); });

          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_STATE_CHANGE:
        {
          let event = aEvent.QueryInterface(Ci.nsIAccessibleStateChangeEvent);
          if (event.state == Ci.nsIAccessibleStates.STATE_CHECKED &&
              !(event.isExtraState())) {
            this.presenters.forEach(
              function(p) {
                p.actionInvoked(aEvent.accessible,
                                event.isEnabled() ? 'check' : 'uncheck');
              }
            );
          }
          else if (event.state == Ci.nsIAccessibleStates.STATE_BUSY &&
                   !(event.isExtraState()) && event.isEnabled()) {
            let role = event.accessible.role;
            if ((role == Ci.nsIAccessibleRole.ROLE_DOCUMENT ||
                 role == Ci.nsIAccessibleRole.ROLE_APPLICATION)) {
              
              
              this.presenters.forEach(
                function(p) {
                  p.tabStateChanged(event.accessible, 'loading');
                }
              );
            }
          }
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_REORDER:
        {
          let acc = aEvent.accessible;
          if (acc.childCount) {
            let docAcc = acc.getChildAt(0);
            if (this._pendingDocuments[aEvent.DOMNode]) {
              
              
              
              
              
              
              let state = {};
              docAcc.getState(state, {});
              if (state.value & Ci.nsIAccessibleStates.STATE_BUSY &&
                  this._isNotChromeDoc(docAcc))
                this.presenters.forEach(
                  function(p) { p.tabStateChanged(docAcc, 'loading'); }
                );
              delete this._pendingDocuments[aEvent.DOMNode];
            }
            if (this._isBrowserDoc(docAcc))
              
              this.presenters.forEach(
                function(p) { p.tabStateChanged(docAcc, 'newdoc'); }
              );
          }
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_DOCUMENT_LOAD_COMPLETE:
        {
          if (this._isNotChromeDoc(aEvent.accessible)) {
            this.presenters.forEach(
              function(p) {
                p.tabStateChanged(aEvent.accessible, 'loaded');
              }
            );
          }
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_DOCUMENT_LOAD_STOPPED:
        {
          this.presenters.forEach(
            function(p) {
              p.tabStateChanged(aEvent.accessible, 'loadstopped');
            }
          );
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_DOCUMENT_RELOAD:
        {
          this.presenters.forEach(
            function(p) {
              p.tabStateChanged(aEvent.accessible, 'reload');
            }
          );
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_TEXT_INSERTED:
      case Ci.nsIAccessibleEvent.EVENT_TEXT_REMOVED:
      {
        if (aEvent.isFromUserInput) {
          
          let event = aEvent.QueryInterface(Ci.nsIAccessibleTextChangeEvent);
          let isInserted = event.isInserted();
          let txtIface = aEvent.accessible.QueryInterface(Ci.nsIAccessibleText);

          let text = '';
          try {
            text = txtIface.
              getText(0, Ci.nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT);
          } catch (x) {
            
            
            if (txtIface.characterCount)
              throw x;
          }

          this.presenters.forEach(
            function(p) {
              p.textChanged(isInserted, event.start, event.length,
                            text, event.modifiedText);
            }
          );
        }
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_SCROLLING_START:
      {
        VirtualCursorController.moveCursorToObject(
          Utils.getVirtualCursor(aEvent.accessibleDocument), aEvent.accessible);
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_FOCUS:
      {
        let acc = aEvent.accessible;
        let doc = aEvent.accessibleDocument;
        if (acc.role != Ci.nsIAccessibleRole.ROLE_DOCUMENT &&
            doc.role != Ci.nsIAccessibleRole.ROLE_CHROME_WINDOW)
          VirtualCursorController.moveCursorToObject(
            Utils.getVirtualCursor(doc), acc);

        let [,extState] = Utils.getStates(acc);
        let editableState = extState &
          (Ci.nsIAccessibleStates.EXT_STATE_EDITABLE |
           Ci.nsIAccessibleStates.EXT_STATE_MULTI_LINE);

        if (editableState != VirtualCursorController.editableState) {
          if (!VirtualCursorController.editableState)
            this.presenters.forEach(
              function(p) {
                p.editingModeChanged(true);
              }
            );
        }
        VirtualCursorController.editableState = editableState;
        break;
      }
      default:
        break;
    }
  },

  





  _isBrowserDoc: function _isBrowserDoc(aDocAcc) {
    let parent = aDocAcc.parent;
    if (!parent)
      return false;

    let domNode = parent.DOMNode;
    if (!domNode)
      return false;

    const ns = 'http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul';
    return (domNode.localName == 'browser' && domNode.namespaceURI == ns);
  },

  




  _isNotChromeDoc: function _isNotChromeDoc(aDocument) {
    let location = aDocument.DOMNode.location;
    if (!location)
      return false;

    return location.protocol != 'about:';
  },

  
  _pendingDocuments: {},

  
  _enabled: false
};

function getAccessible(aNode) {
  try {
    return Cc['@mozilla.org/accessibleRetrieval;1'].
      getService(Ci.nsIAccessibleRetrieval).getAccessibleFor(aNode);
  } catch (e) {
    return null;
  }
}
