



var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/Presenters.jsm');
Cu.import('resource://gre/modules/accessibility/TraversalRules.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

var EXPORTED_SYMBOLS = ['EventManager'];

var EventManager = {
  editState: {},

  start: function start(aSendMsgFunc) {
    try {
      if (!this._started) {
        this.sendMsgFunc = aSendMsgFunc || function() {};
        this.presenters = [new VisualPresenter()];

        if (Utils.MozBuildApp == 'b2g') {
          this.presenters.push(new SpeechPresenter());
        } else if (Utils.MozBuildApp == 'mobile/android') {
          this.presenters.push(new AndroidPresenter());
        }

        Logger.info('EventManager.start', Utils.MozBuildApp, [p.type for each(p in this.presenters)].join(', '));

        this._started = true;
        Services.obs.addObserver(this, 'accessible-event', false);
      }

      this.present(
        function(p) {
          return p.tabStateChanged(null, 'newtab');
        }
      );
    } catch (x) {
      Logger.error('Failed to start EventManager:', x);
    }
  },

  stop: function stop() {
    Services.obs.removeObserver(this, 'accessible-event');
    this.presenters = [];
    this._started = false;
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

        this.present(
          function(p) {
            return p.actionInvoked(activatedAcc, 'click');
          }
        );
        break;
      }
      case 'scroll':
      case 'resize':
      {
        this.present(
          function(p) {
            return p.viewportChanged();;
          }
        );
        break;
      }
      }
    } catch (x) {
      Logger.error('Error handling DOM event:', x);
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'accessible-event':
        var event;
        try {
          event = aSubject.QueryInterface(Ci.nsIAccessibleEvent);
          this.handleAccEvent(event);
        } catch (x) {
          Logger.error('Error handing accessible event:', x);
          return;
        }
    }
  },

  handleAccEvent: function handleAccEvent(aEvent) {
    if (Logger.logLevel >= Logger.DEBUG)
      Logger.debug('A11yEvent', Logger.eventToString(aEvent),
                   Logger.accessibleToString(aEvent.accessible));

    switch (aEvent.eventType) {
      case Ci.nsIAccessibleEvent.EVENT_VIRTUALCURSOR_CHANGED:
      {
        let pivot = aEvent.accessible.
          QueryInterface(Ci.nsIAccessibleCursorable).virtualCursor;
        let position = pivot.position;
        if (position.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME)
          break;
        let event = aEvent.
          QueryInterface(Ci.nsIAccessibleVirtualCursorChangeEvent);
        let presenterContext =
          new PresenterContext(position, event.oldAccessible);
        let reason = event.reason;

        if (this.editState.editing)
          aEvent.accessibleDocument.takeFocus();

        this.present(
          function(p) {
            return p.pivotChanged(presenterContext, reason);
          }
        );
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_STATE_CHANGE:
      {
        let event = aEvent.QueryInterface(Ci.nsIAccessibleStateChangeEvent);
        if (event.state == Ci.nsIAccessibleStates.STATE_CHECKED &&
            !(event.isExtraState())) {
          this.present(
            function(p) {
              return p.actionInvoked(aEvent.accessible,
                                     event.isEnabled() ? 'check' : 'uncheck');
            }
          );
        }
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_SCROLLING_START:
      {
        let vc = Utils.getVirtualCursor(aEvent.accessibleDocument);
        vc.moveNext(TraversalRules.Simple, aEvent.accessible, true);
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_TEXT_CARET_MOVED:
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
          this.present(
            function(p) {
              return p.editingModeChanged(editState.editing);
            }
          );

        if (editState.editing != this.editState.editing ||
            editState.multiline != this.editState.multiline ||
            editState.atEnd != this.editState.atEnd ||
            editState.atStart != this.editState.atStart)
          this.sendMsgFunc("AccessFu:Input", editState);

        this.editState = editState;
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
          this.present(
            function(p) {
              return p.textChanged(isInserted, event.start, event.length,
                                   text, event.modifiedText);
            }
          );
        }
        break;
      }
      case Ci.nsIAccessibleEvent.EVENT_FOCUS:
      {
        
        let acc = aEvent.accessible;
        let doc = aEvent.accessibleDocument;
        if (acc.role != Ci.nsIAccessibleRole.ROLE_DOCUMENT &&
            doc.role != Ci.nsIAccessibleRole.ROLE_CHROME_WINDOW) {
          let vc = Utils.getVirtualCursor(doc);
          vc.moveNext(TraversalRules.Simple, acc, true);
        }
        break;
      }
    }
  },

  present: function present(aPresenterFunc) {
    try {
      this.sendMsgFunc(
        "AccessFu:Present",
        [aPresenterFunc(p) for each (p in this.presenters)].
          filter(function(d) {return !!d;}));
    } catch (x) {
      Logger.error(x);
    }
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
      this.present(
        function(p) {
          return p.tabStateChanged(docAcc, tabstate);
        }
      );
    }
  },

  onProgressChange: function onProgressChange() {},

  onLocationChange: function onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    let docAcc = Utils.AccRetrieval.getAccessibleFor(aWebProgress.DOMWindow.document);
    this.present(
      function(p) {
        return p.tabStateChanged(docAcc, 'newdoc');
      }
    );
  },

  onStatusChange: function onStatusChange() {},

  onSecurityChange: function onSecurityChange() {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports,
                                         Ci.nsIObserver])
};
