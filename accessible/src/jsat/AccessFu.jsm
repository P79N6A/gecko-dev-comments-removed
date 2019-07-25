



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ['AccessFu'];

Cu.import('resource://gre/modules/Services.jsm');

Cu.import('resource://gre/modules/accessibility/Presenters.jsm');
Cu.import('resource://gre/modules/accessibility/VirtualCursorController.jsm');

var AccessFu = {
  







  attach: function attach(aWindow) {
    dump('AccessFu attach!! ' + Services.appinfo.OS + '\n');
    this.chromeWin = aWindow;
    this.presenters = [];

    function checkA11y() {
      if (Services.appinfo.OS == 'Android') {
        let msg = Cc['@mozilla.org/android/bridge;1'].
          getService(Ci.nsIAndroidBridge).handleGeckoMessage(
            JSON.stringify(
                { gecko: {
                    type: 'Accessibility:IsEnabled',
                    eventType: 1,
                    text: []
                  }
                }));
        return JSON.parse(msg).enabled;
      }
      return false;
    }

    if (checkA11y())
      this.enable();
  },

  




  enable: function enable() {
    dump('AccessFu enable');
    this.addPresenter(new VisualPresenter());

    
    if (Services.appinfo.OS == 'Android')
      this.addPresenter(new AndroidPresenter());

    VirtualCursorController.attach(this.chromeWin);

    Services.obs.addObserver(this, 'accessible-event', false);
    this.chromeWin.addEventListener('DOMActivate', this, true);
    this.chromeWin.addEventListener('resize', this, true);
    this.chromeWin.addEventListener('scroll', this, true);
    this.chromeWin.addEventListener('TabOpen', this, true);
    this.chromeWin.addEventListener('TabSelect', this, true);
    this.chromeWin.addEventListener('TabClosed', this, true);
  },

  


  disable: function disable() {
    dump('AccessFu disable');

    this.presenters.forEach(function(p) {p.detach();});
    this.presenters = [];

    VirtualCursorController.detach();

    Services.obs.addObserver(this, 'accessible-event', false);
    this.chromeWin.removeEventListener('DOMActivate', this);
    this.chromeWin.removeEventListener('resize', this);
    this.chromeWin.removeEventListener('scroll', this);
    this.chromeWin.removeEventListener('TabOpen', this);
    this.chromeWin.removeEventListener('TabSelect', this);
    this.chromeWin.removeEventListener('TabClose', this);
  },

  addPresenter: function addPresenter(presenter) {
    this.presenters.push(presenter);
    presenter.attach(this.chromeWin);
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'TabSelect':
        {
          this.getDocAccessible(
              function(docAcc) {
                this.presenters.forEach(function(p) {p.tabSelected(docAcc);});
              });
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
        this.presenters.forEach(function(p) {p.viewportChanged();});
        break;
      }
    }
  },

  getDocAccessible: function getDocAccessible(aCallback) {
    let browserApp = (Services.appinfo.OS == 'Android') ?
      this.chromeWin.BrowserApp : this.chromeWin.gBrowser;

    let docAcc = getAccessible(browserApp.selectedBrowser.contentDocument);
    if (!docAcc) {
      
      this._pendingDocuments[browserApp.selectedBrowser] = aCallback;
    } else {
      aCallback.apply(this, [docAcc]);
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'accessible-event':
        let event;
        try {
          event = aSubject.QueryInterface(Ci.nsIAccessibleEvent);
          this.handleAccEvent(event);
        } catch (ex) {
          dump(ex);
          return;
        }
    }
  },

  handleAccEvent: function handleAccEvent(aEvent) {
    switch (aEvent.eventType) {
      case Ci.nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED:
        {
          let pivot = aEvent.accessible.
            QueryInterface(Ci.nsIAccessibleCursorable).virtualCursor;
          let event = aEvent.
            QueryInterface(Ci.nsIAccessibleVirtualCursorChangeEvent);

          let newContext = this.getNewContext(event.oldAccessible,
                                              pivot.position);
          this.presenters.forEach(
            function(p) {
              p.pivotChanged(pivot.position, newContext);
            });
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
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_REORDER:
        {
          let node = aEvent.accessible.DOMNode;
          let callback = this._pendingDocuments[node];
          if (callback && aEvent.accessible.childCount) {
            
            callback.apply(this, [aEvent.accessible.getChildAt(0)]);
            delete this._pendingDocuments[node];
          }
          break;
        }
      case Ci.nsIAccessibleEvent.EVENT_TEXT_INSERTED:
      case Ci.nsIAccessibleEvent.EVENT_TEXT_REMOVED:
      {
        if (aEvent.isFromUserInput) {
          
          let event = aEvent.QueryInterface(Ci.nsIAccessibleTextChangeEvent);
          let isInserted = event.isInserted();
          let textIface = aEvent.accessible.QueryInterface(Ci.nsIAccessibleText);

          let text = '';
          try {
            text = textIface.
              getText(0, Ci.nsIAccessibleText.TEXT_OFFSET_END_OF_TEXT);
          } catch (x) {
            
            
            if (textIface.characterCount)
              throw x;
          }

          this.presenters.forEach(
            function(p) {
              p.textChanged(isInserted, event.start, event.length, text, event.modifiedText);
            }
          );
        }
        break;
      }
      default:
        break;
    }
  },

  getNewContext: function getNewContext(aOldObject, aNewObject) {
    let newLineage = [];
    let oldLineage = [];

    let parent = aNewObject;
    while ((parent = parent.parent))
      newLineage.push(parent);

    if (aOldObject) {
      parent = aOldObject;
      while ((parent = parent.parent))
        oldLineage.push(parent);
    }




    let i = 0;
    let newContext = [];

    while (true) {
      let newAncestor = newLineage.pop();
      let oldAncestor = oldLineage.pop();

      if (newAncestor == undefined)
        break;

      if (newAncestor != oldAncestor)
        newContext.push(newAncestor);
      i++;
    }

    return newContext;
  },

  
  _pendingDocuments: {}
};

function getAccessible(aNode) {
  try {
    return Cc['@mozilla.org/accessibleRetrieval;1'].
      getService(Ci.nsIAccessibleRetrieval).getAccessibleFor(aNode);
  } catch (e) {
    return null;
  }
}
