



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/accessibility/UtteranceGenerator.jsm');

var EXPORTED_SYMBOLS = ['VisualPresenter',
                        'AndroidPresenter',
                        'DummyAndroidPresenter',
                        'SpeechPresenter',
                        'PresenterContext'];





function Presenter() {}

Presenter.prototype = {
  



  attach: function attach(aWindow) {},

  


  detach: function detach() {},

  






  pivotChanged: function pivotChanged(aContext, aReason) {},

  




  actionInvoked: function actionInvoked(aObject, aActionName) {},

  


  textChanged: function textChanged(aIsInserted, aStartOffset,
                                    aLength, aText,
                                    aModifiedText) {},

  


  textSelectionChanged: function textSelectionChanged() {},

  



  selectionChanged: function selectionChanged(aObject) {},

  






  tabStateChanged: function tabStateChanged(aDocObj, aPageState) {},

  






  tabSelected: function tabSelected(aDocContext, aVCContext) {},

  



  viewportChanged: function viewportChanged() {},

  


  editingModeChanged: function editingModeChanged(aIsEditing) {}
};





function VisualPresenter() {}

VisualPresenter.prototype = {
  __proto__: Presenter.prototype,

  


  BORDER_PADDING: 2,

  attach: function VisualPresenter_attach(aWindow) {
    this.chromeWin = aWindow;

    
    let stylesheetURL = 'chrome://global/content/accessibility/AccessFu.css';
    this.stylesheet = aWindow.document.createProcessingInstruction(
      'xml-stylesheet', 'href="' + stylesheetURL + '" type="text/css"');
    aWindow.document.insertBefore(this.stylesheet, aWindow.document.firstChild);

    
    this.highlightBox = this.chromeWin.document.
      createElementNS('http://www.w3.org/1999/xhtml', 'div');
    this.chromeWin.document.documentElement.appendChild(this.highlightBox);
    this.highlightBox.id = 'virtual-cursor-box';

    
    let inset = this.chromeWin.document.
      createElementNS('http://www.w3.org/1999/xhtml', 'div');
    inset.id = 'virtual-cursor-inset';

    this.highlightBox.appendChild(inset);
  },

  detach: function VisualPresenter_detach() {
    this.chromeWin.document.removeChild(this.stylesheet);
    this.highlightBox.parentNode.removeChild(this.highlightBox);
    this.highlightBox = this.stylesheet = null;
  },

  viewportChanged: function VisualPresenter_viewportChanged() {
    if (this._currentObject)
      this._highlight(this._currentObject);
  },

  pivotChanged: function VisualPresenter_pivotChanged(aContext, aReason) {
    this._currentObject = aContext.accessible;

    if (!aContext.accessible) {
      this._hide();
      return;
    }

    try {
      aContext.accessible.scrollTo(
        Ci.nsIAccessibleScrollType.SCROLL_TYPE_ANYWHERE);
      this._highlight(aContext.accessible);
    } catch (e) {
      Logger.error('Failed to get bounds: ' + e);
      return;
    }
  },

  tabSelected: function VisualPresenter_tabSelected(aDocContext, aVCContext) {
    this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function VisualPresenter_tabStateChanged(aDocObj,
                                                            aPageState) {
    if (aPageState == 'newdoc')
      this._hide();
  },

  

  _hide: function _hide() {
    this.highlightBox.style.display = 'none';
  },

  _highlight: function _highlight(aObject) {
    let vp = Utils.getViewport(this.chromeWin) || { zoom: 1.0, offsetY: 0 };
    let bounds = this._getBounds(aObject, vp.zoom);

    
    this.highlightBox.style.display = 'none';
    this.highlightBox.style.top = bounds.top + 'px';
    this.highlightBox.style.left = bounds.left + 'px';
    this.highlightBox.style.width = bounds.width + 'px';
    this.highlightBox.style.height = bounds.height + 'px';
    this.highlightBox.style.display = 'block';
  },

  _getBounds: function _getBounds(aObject, aZoom, aStart, aEnd) {
    let objX = {}, objY = {}, objW = {}, objH = {};

    if (aEnd >= 0 && aStart >= 0 && aEnd != aStart) {
      
      
    }

    aObject.getBounds(objX, objY, objW, objH);

    
    let docX = {}, docY = {};
    let docRoot = aObject.rootDocument.QueryInterface(Ci.nsIAccessible);
    docRoot.getBounds(docX, docY, {}, {});

    let rv = {
      left: Math.round((objX.value - docX.value - this.BORDER_PADDING) * aZoom),
      top: Math.round((objY.value - docY.value - this.BORDER_PADDING) * aZoom),
      width: Math.round((objW.value + (this.BORDER_PADDING * 2)) * aZoom),
      height: Math.round((objH.value + (this.BORDER_PADDING * 2)) * aZoom)
    };

    return rv;
  }
};





function AndroidPresenter() {}

AndroidPresenter.prototype = {
  __proto__: Presenter.prototype,

  
  ANDROID_VIEW_CLICKED: 0x01,
  ANDROID_VIEW_LONG_CLICKED: 0x02,
  ANDROID_VIEW_SELECTED: 0x04,
  ANDROID_VIEW_FOCUSED: 0x08,
  ANDROID_VIEW_TEXT_CHANGED: 0x10,
  ANDROID_WINDOW_STATE_CHANGED: 0x20,
  ANDROID_VIEW_HOVER_ENTER: 0x80,
  ANDROID_VIEW_HOVER_EXIT: 0x100,
  ANDROID_VIEW_SCROLLED: 0x1000,

  attach: function AndroidPresenter_attach(aWindow) {
    this.chromeWin = aWindow;
  },

  pivotChanged: function AndroidPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible)
      return;

    let isExploreByTouch = (aReason == Ci.nsIAccessiblePivot.REASON_POINT &&
                            Utils.AndroidSdkVersion >= 14);

    if (isExploreByTouch) {
      
      
      this.sendMessageToJava({
         gecko: {
           type: 'Accessibility:Event',
           eventType: this.ANDROID_VIEW_HOVER_EXIT,
           text: []
         }
      });
    }

    let output = [];

    aContext.newAncestry.forEach(
      function(acc) {
        output.push.apply(output, UtteranceGenerator.genForObject(acc));
      }
    );

    output.push.apply(output,
                      UtteranceGenerator.genForObject(aContext.accessible));

    aContext.subtreePreorder.forEach(
      function(acc) {
        output.push.apply(output, UtteranceGenerator.genForObject(acc));
      }
    );

    this.sendMessageToJava({
      gecko: {
        type: 'Accessibility:Event',
        eventType: isExploreByTouch ?
          this.ANDROID_VIEW_HOVER_ENTER :
          this.ANDROID_VIEW_FOCUSED,
        text: output
      }
    });
  },

  actionInvoked: function AndroidPresenter_actionInvoked(aObject, aActionName) {
    this.sendMessageToJava({
      gecko: {
        type: 'Accessibility:Event',
        eventType: this.ANDROID_VIEW_CLICKED,
        text: UtteranceGenerator.genForAction(aObject, aActionName)
      }
    });
  },

  tabSelected: function AndroidPresenter_tabSelected(aDocContext, aVCContext) {
    
    this.pivotChanged(aVCContext, Ci.nsIAccessiblePivot.REASON_NONE);
  },

  tabStateChanged: function AndroidPresenter_tabStateChanged(aDocObj,
                                                             aPageState) {
    this._appAnnounce(
      UtteranceGenerator.genForTabStateChange(aDocObj, aPageState));
  },

  textChanged: function AndroidPresenter_textChanged(aIsInserted, aStart,
                                                     aLength, aText,
                                                     aModifiedText) {
    let androidEvent = {
      type: 'Accessibility:Event',
      eventType: this.ANDROID_VIEW_TEXT_CHANGED,
      text: [aText],
      fromIndex: aStart,
      removedCount: 0,
      addedCount: 0
    };

    if (aIsInserted) {
      androidEvent.addedCount = aLength;
      androidEvent.beforeText =
        aText.substring(0, aStart) + aText.substring(aStart + aLength);
    } else {
      androidEvent.removedCount = aLength;
      androidEvent.beforeText =
        aText.substring(0, aStart) + aModifiedText + aText.substring(aStart);
    }

    this.sendMessageToJava({gecko: androidEvent});
  },

  viewportChanged: function AndroidPresenter_viewportChanged() {
    if (Utils.AndroidSdkVersion < 14)
      return;

    let win = Utils.getBrowserApp(this.chromeWin).selectedBrowser.contentWindow;
    this.sendMessageToJava({
      gecko: {
        type: 'Accessibility:Event',
        eventType: this.ANDROID_VIEW_SCROLLED,
        text: [],
        scrollX: win.scrollX,
        scrollY: win.scrollY,
        maxScrollX: win.scrollMaxX,
        maxScrollY: win.scrollMaxY
      }
    });
  },

  editingModeChanged: function AndroidPresenter_editingModeChanged(aIsEditing) {
    this._appAnnounce(UtteranceGenerator.genForEditingMode(aIsEditing));
  },

  _appAnnounce: function _appAnnounce(aUtterance) {
    if (!aUtterance.length)
      return;

    this.sendMessageToJava({
      gecko: {
        type: 'Accessibility:Event',
        eventType: this.ANDROID_VIEW_TEXT_CHANGED,
        text: aUtterance,
        addedCount: aUtterance.join(' ').length,
        removedCount: 0,
        fromIndex: 0
      }
    });
  },

  sendMessageToJava: function AndroidPresenter_sendMessageTojava(aMessage) {
    return Cc['@mozilla.org/android/bridge;1'].
      getService(Ci.nsIAndroidBridge).
      handleGeckoMessage(JSON.stringify(aMessage));
  }
};





function DummyAndroidPresenter() {}

DummyAndroidPresenter.prototype = {
  __proto__: AndroidPresenter.prototype,

  sendMessageToJava: function DummyAndroidPresenter_sendMessageToJava(aMsg) {
    Logger.debug('Android event:\n' + JSON.stringify(aMsg, null, 2));
  }
};





function SpeechPresenter() {}

SpeechPresenter.prototype = {
  __proto__: Presenter.prototype,


  pivotChanged: function SpeechPresenter_pivotChanged(aContext, aReason) {
    if (!aContext.accessible)
      return;

    let output = [];

    aContext.newAncestry.forEach(
      function(acc) {
        output.push.apply(output, UtteranceGenerator.genForObject(acc));
      }
    );

    output.push.apply(output,
                      UtteranceGenerator.genForObject(aContext.accessible));

    aContext.subtreePreorder.forEach(
      function(acc) {
        output.push.apply(output, UtteranceGenerator.genForObject(acc));
      }
    );

    Logger.info('SPEAK', '"' + output.join(' ') + '"');
  }
}





function PresenterContext(aAccessible, aOldAccessible) {
  this._accessible = aAccessible;
  this._oldAccessible =
    this._isDefunct(aOldAccessible) ? null : aOldAccessible;
}

PresenterContext.prototype = {
  get accessible() {
    return this._accessible;
  },

  get oldAccessible() {
    return this._oldAccessible;
  },

  




  get newAncestry() {
    if (!this._newAncestry) {
      let newLineage = [];
      let oldLineage = [];

      let parent = this._accessible;
      while (parent && (parent = parent.parent))
        newLineage.push(parent);

      parent = this._oldAccessible;
      while (parent && (parent = parent.parent))
        oldLineage.push(parent);

      this._newAncestry = [];

      while (true) {
        let newAncestor = newLineage.pop();
        let oldAncestor = oldLineage.pop();

        if (newAncestor == undefined)
          break;

        if (newAncestor != oldAncestor)
          this._newAncestry.push(newAncestor);
      }

    }

    return this._newAncestry;
  },

  



  get subtreePreorder() {
    function traversePreorder(aAccessible) {
      let list = [];
      let child = aAccessible.firstChild;
      while (child) {
        let state = {};
        child.getState(state, {});

        if (!(state.value & Ci.nsIAccessibleStates.STATE_INVISIBLE)) {
          list.push(child);
          list.push.apply(list, traversePreorder(child));
        }

        child = child.nextSibling;
      }
      return list;
    }

    if (!this._subtreePreOrder)
      this._subtreePreOrder = traversePreorder(this._accessible);

    return this._subtreePreOrder;
  },

  _isDefunct: function _isDefunct(aAccessible) {
    try {
      let extstate = {};
      aAccessible.getState({}, extstate);
      return !!(aAccessible.value & Ci.nsIAccessibleStates.EXT_STATE_DEFUNCT);
    } catch (x) {
      return true;
    }
  }
};
