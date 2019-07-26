



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/UtteranceGenerator.jsm');

this.EXPORTED_SYMBOLS = ['Presentation'];





function Presenter() {}

Presenter.prototype = {
  


  type: 'Base',

  






  pivotChanged: function pivotChanged(aContext, aReason) {},

  




  actionInvoked: function actionInvoked(aObject, aActionName) {},

  


  textChanged: function textChanged(aIsInserted, aStartOffset,
                                    aLength, aText,
                                    aModifiedText) {},

  


  textSelectionChanged: function textSelectionChanged() {},

  



  selectionChanged: function selectionChanged(aObject) {},

  






  tabStateChanged: function tabStateChanged(aDocObj, aPageState) {},

  






  tabSelected: function tabSelected(aDocContext, aVCContext) {},

  




  viewportChanged: function viewportChanged(aWindow) {},

  


  editingModeChanged: function editingModeChanged(aIsEditing) {},

  


  announce: function announce(aAnnouncement) {}
};





this.VisualPresenter = function VisualPresenter() {};

VisualPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Visual',

  


  BORDER_PADDING: 2,

  viewportChanged: function VisualPresenter_viewportChanged(aWindow) {
    if (this._currentAccessible) {
      let context = new PivotContext(this._currentAccessible);
      return {
        type: this.type,
        details: {
          method: 'showBounds',
          bounds: context.bounds,
          padding: this.BORDER_PADDING
        }
      };
    }

    return null;
  },

  pivotChanged: function VisualPresenter_pivotChanged(aContext, aReason) {
    this._currentAccessible = aContext.accessible;

    if (!aContext.accessible)
      return {type: this.type, details: {method: 'hideBounds'}};

    try {
      aContext.accessible.scrollTo(
        Ci.nsIAccessibleScrollType.SCROLL_TYPE_ANYWHERE);
      return {
        type: this.type,
        details: {
          method: 'showBounds',
          bounds: aContext.bounds,
          padding: this.BORDER_PADDING
        }
      };
    } catch (e) {
      Logger.error('Failed to get bounds: ' + e);
      return null;
    }
  },

  tabSelected: function VisualPresenter_tabSelected(aDocContext, aVCContext) {
    return this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function VisualPresenter_tabStateChanged(aDocObj,
                                                            aPageState) {
    if (aPageState == 'newdoc')
      return {type: this.type, details: {method: 'hideBounds'}};

    return null;
  },

  announce: function VisualPresenter_announce(aAnnouncement) {
    return {
      type: this.type,
      details: {
        method: 'showAnnouncement',
        text: aAnnouncement,
        duration: 1000
      }
    };
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
  ANDROID_ANNOUNCEMENT: 0x4000,
  ANDROID_VIEW_ACCESSIBILITY_FOCUSED: 0x8000,

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

    let state = Utils.getStates(aContext.accessible)[0];

    androidEvents.push({eventType: (isExploreByTouch) ?
                          this.ANDROID_VIEW_HOVER_ENTER : focusEventType,
                        text: UtteranceGenerator.genForContext(aContext),
                        bounds: aContext.bounds,
                        clickable: aContext.accessible.actionCount > 0,
                        checkable: !!(state &
                                      Ci.nsIAccessibleStates.STATE_CHECKABLE),
                        checked: !!(state &
                                    Ci.nsIAccessibleStates.STATE_CHECKED)});


    return {
      type: this.type,
      details: androidEvents
    };
  },

  actionInvoked: function AndroidPresenter_actionInvoked(aObject, aActionName) {
    let state = Utils.getStates(aObject)[0];
    return {
      type: this.type,
      details: [{
        eventType: this.ANDROID_VIEW_CLICKED,
        text: UtteranceGenerator.genForAction(aObject, aActionName),
        checked: !!(state & Ci.nsIAccessibleStates.STATE_CHECKED)
      }]
    };
  },

  tabSelected: function AndroidPresenter_tabSelected(aDocContext, aVCContext) {
    
    return this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function AndroidPresenter_tabStateChanged(aDocObj,
                                                             aPageState) {
    return this.announce(
      UtteranceGenerator.genForTabStateChange(aDocObj, aPageState).join(' '));
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
    return this.announce(
      UtteranceGenerator.genForEditingMode(aIsEditing).join(' '));
  },

  announce: function AndroidPresenter_announce(aAnnouncement) {
    return {
      type: this.type,
      details: [{
        eventType: (Utils.AndroidSdkVersion >= 16) ?
          this.ANDROID_ANNOUNCEMENT : this.ANDROID_VIEW_TEXT_CHANGED,
        text: [aAnnouncement],
        addedCount: aAnnouncement.length,
        removedCount: 0,
        fromIndex: 0
      }]
    };
  }
};





this.SpeechPresenter = function SpeechPresenter() {};

SpeechPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Speech',

  pivotChanged: function SpeechPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible)
      return null;

    return {
      type: this.type,
      details: {
        actions: [
          {method: 'playEarcon', data: 'tick', options: {}},
          {method: 'speak',
            data: UtteranceGenerator.genForContext(aContext).join(' '),
            options: {enqueue: true}}
        ]
      }
    };
  }
};





this.HapticPresenter = function HapticPresenter() {};

HapticPresenter.prototype = {
  __proto__: Presenter.prototype,

  type: 'Haptic',

  PIVOT_CHANGE_PATTHERN: [20],

  pivotChanged: function HapticPresenter_pivotChanged(aContext, aReason) {
    return { type: this.type, details: { pattern: this.PIVOT_CHANGE_PATTHERN } };
  }
};

this.Presentation = {
  get presenters() {
    delete this.presenters;
    this.presenters = [new VisualPresenter()];

    if (Utils.MozBuildApp == 'mobile/android') {
      this.presenters.push(new AndroidPresenter());
    } else {
      this.presenters.push(new SpeechPresenter());
      this.presenters.push(new HapticPresenter());
    }

    return this.presenters;
  },

  pivotChanged: function Presentation_pivotChanged(aPosition,
                                                   aOldPosition,
                                                   aReason) {
    let context = new PivotContext(aPosition, aOldPosition);
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
    
    
    return [p.announce(UtteranceGenerator.genForAnnouncement(aAnnouncement)[0])
              for each (p in this.presenters)];
  }
};
