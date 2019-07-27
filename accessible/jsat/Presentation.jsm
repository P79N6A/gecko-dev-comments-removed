



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'PivotContext',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'UtteranceGenerator',
  'resource://gre/modules/accessibility/OutputGenerator.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'BrailleGenerator',
  'resource://gre/modules/accessibility/OutputGenerator.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles',
  'resource://gre/modules/accessibility/Constants.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'States',
  'resource://gre/modules/accessibility/Constants.jsm');

this.EXPORTED_SYMBOLS = ['Presentation'];





function Presenter() {}

Presenter.prototype = {
  


  type: 'Base',

  






  pivotChanged: function pivotChanged(aContext, aReason) {},

  




  actionInvoked: function actionInvoked(aObject, aActionName) {},

  


  textChanged: function textChanged(aIsInserted, aStartOffset,
                                    aLength, aText,
                                    aModifiedText) {},

  


  textSelectionChanged: function textSelectionChanged(aText, aStart, aEnd, aOldStart, aOldEnd, aIsFromUser) {},

  



  selectionChanged: function selectionChanged(aObject) {},

  



  valueChanged: function valueChanged(aAccessible) {},

  






  tabStateChanged: function tabStateChanged(aDocObj, aPageState) {},

  






  tabSelected: function tabSelected(aDocContext, aVCContext) {},

  




  viewportChanged: function viewportChanged(aWindow) {},

  


  editingModeChanged: function editingModeChanged(aIsEditing) {},

  


  announce: function announce(aAnnouncement) {},



  






  liveRegion: function liveRegionShown(aContext, aIsPolite, aIsHide,
    aModifiedText) {}
};




this.VisualPresenter = function VisualPresenter() {
  this._displayedAccessibles = new WeakMap();
};

VisualPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Visual',

  


  BORDER_PADDING: 2,

  viewportChanged: function VisualPresenter_viewportChanged(aWindow) {
    let currentDisplay = this._displayedAccessibles.get(aWindow);
    if (!currentDisplay) {
      return null;
    }

    let currentAcc = currentDisplay.accessible;
    let start = currentDisplay.startOffset;
    let end = currentDisplay.endOffset;
    if (Utils.isAliveAndVisible(currentAcc)) {
      let bounds = (start === -1 && end === -1) ? Utils.getBounds(currentAcc) :
                   Utils.getTextBounds(currentAcc, start, end);

      return {
        type: this.type,
        details: {
          eventType: 'viewport-change',
          bounds: bounds,
          padding: this.BORDER_PADDING
        }
      };
    }

    return null;
  },

  pivotChanged: function VisualPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible) {
      
      return null;
    }

    this._displayedAccessibles.set(aContext.accessible.document.window,
                                   { accessible: aContext.accessibleForBounds,
                                     startOffset: aContext.startOffset,
                                     endOffset: aContext.endOffset });

    try {
      aContext.accessibleForBounds.scrollTo(
        Ci.nsIAccessibleScrollType.SCROLL_TYPE_ANYWHERE);

      let bounds = (aContext.startOffset === -1 && aContext.endOffset === -1) ?
            aContext.bounds : Utils.getTextBounds(aContext.accessibleForBounds,
                                                  aContext.startOffset,
                                                  aContext.endOffset);

      return {
        type: this.type,
        details: {
          eventType: 'vc-change',
          bounds: bounds,
          padding: this.BORDER_PADDING
        }
      };
    } catch (e) {
      Logger.logException(e, 'Failed to get bounds');
      return null;
    }
  },

  tabSelected: function VisualPresenter_tabSelected(aDocContext, aVCContext) {
    return this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function VisualPresenter_tabStateChanged(aDocObj,
                                                            aPageState) {
    if (aPageState == 'newdoc')
      return {type: this.type, details: {eventType: 'tabstate-change'}};

    return null;
  }
};





this.AndroidPresenter = function AndroidPresenter() {};

AndroidPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Android',

  
  ANDROID_VIEW_CLICKED: 0x01,
  ANDROID_VIEW_LONG_CLICKED: 0x02,
  ANDROID_VIEW_SELECTED: 0x04,
  ANDROID_VIEW_FOCUSED: 0x08,
  ANDROID_VIEW_TEXT_CHANGED: 0x10,
  ANDROID_WINDOW_STATE_CHANGED: 0x20,
  ANDROID_VIEW_HOVER_ENTER: 0x80,
  ANDROID_VIEW_HOVER_EXIT: 0x100,
  ANDROID_VIEW_SCROLLED: 0x1000,
  ANDROID_VIEW_TEXT_SELECTION_CHANGED: 0x2000,
  ANDROID_ANNOUNCEMENT: 0x4000,
  ANDROID_VIEW_ACCESSIBILITY_FOCUSED: 0x8000,
  ANDROID_VIEW_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY: 0x20000,

  pivotChanged: function AndroidPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible)
      return null;

    let androidEvents = [];

    let isExploreByTouch = (aReason == Ci.nsIAccessiblePivot.REASON_POINT &&
                            Utils.AndroidSdkVersion >= 14);
    let focusEventType = (Utils.AndroidSdkVersion >= 16) ?
      this.ANDROID_VIEW_ACCESSIBILITY_FOCUSED :
      this.ANDROID_VIEW_FOCUSED;

    if (isExploreByTouch) {
      
      
      androidEvents.push({eventType: this.ANDROID_VIEW_HOVER_EXIT, text: []});
    }

    let brailleOutput = {};
    if (Utils.AndroidSdkVersion >= 16) {
      if (!this._braillePresenter) {
        this._braillePresenter = new BraillePresenter();
      }
      brailleOutput = this._braillePresenter.pivotChanged(aContext, aReason).
                         details;
    }

    if (aReason === Ci.nsIAccessiblePivot.REASON_TEXT) {
      if (Utils.AndroidSdkVersion >= 16) {
        let adjustedText = aContext.textAndAdjustedOffsets;

        androidEvents.push({
          eventType: this.ANDROID_VIEW_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY,
          text: [adjustedText.text],
          fromIndex: adjustedText.startOffset,
          toIndex: adjustedText.endOffset
        });
      }
    } else {
      let state = Utils.getState(aContext.accessible);
      androidEvents.push({eventType: (isExploreByTouch) ?
                           this.ANDROID_VIEW_HOVER_ENTER : focusEventType,
                         text: Utils.localize(UtteranceGenerator.genForContext(
                           aContext)),
                         bounds: aContext.bounds,
                         clickable: aContext.accessible.actionCount > 0,
                         checkable: state.contains(States.CHECKABLE),
                         checked: state.contains(States.CHECKED),
                         brailleOutput: brailleOutput});
    }


    return {
      type: this.type,
      details: androidEvents
    };
  },

  actionInvoked: function AndroidPresenter_actionInvoked(aObject, aActionName) {
    let state = Utils.getState(aObject);

    
    if (state.contains(States.CHECKABLE))
      return null;

    return {
      type: this.type,
      details: [{
        eventType: this.ANDROID_VIEW_CLICKED,
        text: Utils.localize(UtteranceGenerator.genForAction(aObject,
          aActionName)),
        checked: state.contains(States.CHECKED)
      }]
    };
  },

  tabSelected: function AndroidPresenter_tabSelected(aDocContext, aVCContext) {
    
    return this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function AndroidPresenter_tabStateChanged(aDocObj,
                                                             aPageState) {
    return this.announce(
      UtteranceGenerator.genForTabStateChange(aDocObj, aPageState));
  },

  textChanged: function AndroidPresenter_textChanged(aIsInserted, aStart,
                                                     aLength, aText,
                                                     aModifiedText) {
    let eventDetails = {
      eventType: this.ANDROID_VIEW_TEXT_CHANGED,
      text: [aText],
      fromIndex: aStart,
      removedCount: 0,
      addedCount: 0
    };

    if (aIsInserted) {
      eventDetails.addedCount = aLength;
      eventDetails.beforeText =
        aText.substring(0, aStart) + aText.substring(aStart + aLength);
    } else {
      eventDetails.removedCount = aLength;
      eventDetails.beforeText =
        aText.substring(0, aStart) + aModifiedText + aText.substring(aStart);
    }

    return {type: this.type, details: [eventDetails]};
  },

  textSelectionChanged: function AndroidPresenter_textSelectionChanged(aText, aStart,
                                                                       aEnd, aOldStart,
                                                                       aOldEnd, aIsFromUser) {
    let androidEvents = [];

    if (Utils.AndroidSdkVersion >= 14 && !aIsFromUser) {
      if (!this._braillePresenter) {
        this._braillePresenter = new BraillePresenter();
      }
      let brailleOutput = this._braillePresenter.textSelectionChanged(aText, aStart, aEnd,
                                                                      aOldStart, aOldEnd,
                                                                      aIsFromUser).details;

      androidEvents.push({
        eventType: this.ANDROID_VIEW_TEXT_SELECTION_CHANGED,
        text: [aText],
        fromIndex: aStart,
        toIndex: aEnd,
        itemCount: aText.length,
        brailleOutput: brailleOutput
      });
    }

    if (Utils.AndroidSdkVersion >= 16 && aIsFromUser) {
      let [from, to] = aOldStart < aStart ? [aOldStart, aStart] : [aStart, aOldStart];
      androidEvents.push({
        eventType: this.ANDROID_VIEW_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY,
        text: [aText],
        fromIndex: from,
        toIndex: to
      });
    }

    return {
      type: this.type,
      details: androidEvents
    };
  },

  viewportChanged: function AndroidPresenter_viewportChanged(aWindow) {
    if (Utils.AndroidSdkVersion < 14)
      return null;

    return {
      type: this.type,
      details: [{
        eventType: this.ANDROID_VIEW_SCROLLED,
        text: [],
        scrollX: aWindow.scrollX,
        scrollY: aWindow.scrollY,
        maxScrollX: aWindow.scrollMaxX,
        maxScrollY: aWindow.scrollMaxY
      }]
    };
  },

  editingModeChanged: function AndroidPresenter_editingModeChanged(aIsEditing) {
    return this.announce(UtteranceGenerator.genForEditingMode(aIsEditing));
  },

  announce: function AndroidPresenter_announce(aAnnouncement) {
    let localizedAnnouncement = Utils.localize(aAnnouncement).join(' ');
    return {
      type: this.type,
      details: [{
        eventType: (Utils.AndroidSdkVersion >= 16) ?
          this.ANDROID_ANNOUNCEMENT : this.ANDROID_VIEW_TEXT_CHANGED,
        text: [localizedAnnouncement],
        addedCount: localizedAnnouncement.length,
        removedCount: 0,
        fromIndex: 0
      }]
    };
  },

  liveRegion: function AndroidPresenter_liveRegion(aContext, aIsPolite,
    aIsHide, aModifiedText) {
    return this.announce(
      UtteranceGenerator.genForLiveRegion(aContext, aIsHide, aModifiedText));
  }
};




this.B2GPresenter = function B2GPresenter() {};

B2GPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'B2G',

  



  PIVOT_CHANGE_HAPTIC_PATTERN: [40],

  



  pivotChangedReasons: ['none', 'next', 'prev', 'first', 'last', 'text',
    'point'],

  pivotChanged: function B2GPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible) {
      return null;
    }

    return {
      type: this.type,
      details: {
        eventType: 'vc-change',
        data: UtteranceGenerator.genForContext(aContext),
        options: {
          pattern: this.PIVOT_CHANGE_HAPTIC_PATTERN,
          isKey: aContext.accessible.role === Roles.KEY,
          reason: this.pivotChangedReasons[aReason]
        }
      }
    };
  },

  valueChanged: function B2GPresenter_valueChanged(aAccessible) {
    return {
      type: this.type,
      details: {
        eventType: 'value-change',
        data: aAccessible.value
      }
    };
  },

  actionInvoked: function B2GPresenter_actionInvoked(aObject, aActionName) {
    return {
      type: this.type,
      details: {
        eventType: 'action',
        data: UtteranceGenerator.genForAction(aObject, aActionName)
      }
    };
  },

  liveRegion: function
    B2GPresenter_liveRegion(aContext, aIsPolite, aIsHide, aModifiedText) {
      return {
        type: this.type,
        details: {
          eventType: 'liveregion-change',
          data: UtteranceGenerator.genForLiveRegion(aContext, aIsHide,
            aModifiedText),
          options: {enqueue: aIsPolite}
        }
      };
    },

  announce: function B2GPresenter_announce(aAnnouncement) {
    return {
      type: this.type,
      details: {
        eventType: 'announcement',
        data: aAnnouncement
      }
    };
  }
};




this.BraillePresenter = function BraillePresenter() {};

BraillePresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Braille',

  pivotChanged: function BraillePresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible) {
      return null;
    }

    return {
      type: this.type,
      details: {
        output: Utils.localize(BrailleGenerator.genForContext(aContext)).join(
          ' '),
        selectionStart: 0,
        selectionEnd: 0
      }
    };
  },

  textSelectionChanged: function BraillePresenter_textSelectionChanged(aText, aStart,
                                                                       aEnd, aOldStart,
                                                                       aOldEnd, aIsFromUser) {
    return {
      type: this.type,
      details: {
        selectionStart: aStart,
        selectionEnd: aEnd
      }
    };
  }
};

this.Presentation = {
  get presenters() {
    delete this.presenters;
    let presenterMap = {
      'mobile/android': [VisualPresenter, AndroidPresenter],
      'b2g': [VisualPresenter, B2GPresenter],
      'browser': [VisualPresenter, B2GPresenter, AndroidPresenter]
    };
    this.presenters = [new P() for (P of presenterMap[Utils.MozBuildApp])];
    return this.presenters;
  },

  pivotChanged: function Presentation_pivotChanged(aPosition, aOldPosition, aReason,
                                                   aStartOffset, aEndOffset) {
    let context = new PivotContext(aPosition, aOldPosition, aStartOffset, aEndOffset);
    return [p.pivotChanged(context, aReason)
              for each (p in this.presenters)];
  },

  actionInvoked: function Presentation_actionInvoked(aObject, aActionName) {
    return [p.actionInvoked(aObject, aActionName)
              for each (p in this.presenters)];
  },

  textChanged: function Presentation_textChanged(aIsInserted, aStartOffset,
                                    aLength, aText,
                                    aModifiedText) {
    return [p.textChanged(aIsInserted, aStartOffset, aLength,
                          aText, aModifiedText)
              for each (p in this.presenters)];
  },

  textSelectionChanged: function textSelectionChanged(aText, aStart, aEnd,
                                                      aOldStart, aOldEnd,
                                                      aIsFromUser) {
    return [p.textSelectionChanged(aText, aStart, aEnd,
                                   aOldStart, aOldEnd, aIsFromUser)
              for each (p in this.presenters)];
  },

  valueChanged: function valueChanged(aAccessible) {
    return [ p.valueChanged(aAccessible) for (p of this.presenters) ];
  },

  tabStateChanged: function Presentation_tabStateChanged(aDocObj, aPageState) {
    return [p.tabStateChanged(aDocObj, aPageState)
              for each (p in this.presenters)];
  },

  viewportChanged: function Presentation_viewportChanged(aWindow) {
    return [p.viewportChanged(aWindow)
              for each (p in this.presenters)];
  },

  editingModeChanged: function Presentation_editingModeChanged(aIsEditing) {
    return [p.editingModeChanged(aIsEditing)
              for each (p in this.presenters)];
  },

  announce: function Presentation_announce(aAnnouncement) {
    
    
    return [p.announce(UtteranceGenerator.genForAnnouncement(aAnnouncement))
              for each (p in this.presenters)];
  },

  liveRegion: function Presentation_liveRegion(aAccessible, aIsPolite, aIsHide,
    aModifiedText) {
    let context;
    if (!aModifiedText) {
      context = new PivotContext(aAccessible, null, -1, -1, true,
        aIsHide ? true : false);
    }
    return [p.liveRegion(context, aIsPolite, aIsHide, aModifiedText) for (
      p of this.presenters)];
  }
};
