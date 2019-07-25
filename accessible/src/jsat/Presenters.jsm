



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/accessibility/UtteranceGenerator.jsm');
Cu.import('resource://gre/modules/Services.jsm');

var EXPORTED_SYMBOLS = ['VisualPresenter',
                        'AndroidPresenter',
                        'DummyAndroidPresenter'];





function Presenter() {}

Presenter.prototype = {
  



  attach: function attach(aWindow) {},

  


  detach: function detach() {},

  





  pivotChanged: function pivotChanged(aObject, aNewContext) {},

  




  actionInvoked: function actionInvoked(aObject, aActionName) {},

  


  textChanged: function textChanged() {},

  


  textSelectionChanged: function textSelectionChanged() {},

  



  selectionChanged: function selectionChanged(aObject) {},

  


  pageStateChanged: function pageStateChanged() {},

  



  tabSelected: function tabSelected(aObject) {},

  



  viewportChanged: function viewportChanged() {}
};





function VisualPresenter() {}

VisualPresenter.prototype = new Presenter();




VisualPresenter.prototype.BORDER_PADDING = 2;

VisualPresenter.prototype.attach = function(aWindow) {
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
};

VisualPresenter.prototype.detach = function() {
  this.chromeWin.document.removeChild(this.stylesheet);
  this.highlightBox.parentNode.removeChild(this.highlightBox);
  this.highlightBox = this.stylesheet = null;
};

VisualPresenter.prototype.viewportChanged = function() {
  if (this._currentObject)
    this.highlight(this._currentObject);
};

VisualPresenter.prototype.pivotChanged = function(aObject, aNewContext) {
  this._currentObject = aObject;

  if (!aObject) {
    this.hide();
    return;
  }

  try {
    aObject.scrollTo(Ci.nsIAccessibleScrollType.SCROLL_TYPE_ANYWHERE);
    this.highlight(aObject);
  } catch (e) {
    dump('Error getting bounds: ' + e);
    return;
  }
};

VisualPresenter.prototype.tabSelected = function(aObject) {
  let vcDoc = aObject.QueryInterface(Ci.nsIAccessibleCursorable);
  this.pivotChanged(vcDoc.virtualCursor.position);
};



VisualPresenter.prototype.hide = function hide() {
  this.highlightBox.style.display = 'none';
};

VisualPresenter.prototype.highlight = function(aObject) {
  let vp = (Services.appinfo.OS == 'Android') ?
    this.chromeWin.BrowserApp.selectedTab.getViewport() :
    { zoom: 1.0, offsetY: 0 };

  let bounds = this.getBounds(aObject, vp.zoom);

  
  this.highlightBox.style.display = 'none';
  this.highlightBox.style.top = bounds.top + 'px';
  this.highlightBox.style.left = bounds.left + 'px';
  this.highlightBox.style.width = bounds.width + 'px';
  this.highlightBox.style.height = bounds.height + 'px';
  this.highlightBox.style.display = 'block';
};

VisualPresenter.prototype.getBounds = function(aObject, aZoom, aStart, aEnd) {
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
};





const ANDROID_TYPE_VIEW_CLICKED = 0x01;
const ANDROID_TYPE_VIEW_LONG_CLICKED = 0x02;
const ANDROID_TYPE_VIEW_SELECTED = 0x04;
const ANDROID_TYPE_VIEW_FOCUSED = 0x08;
const ANDROID_TYPE_VIEW_TEXT_CHANGED = 0x10;
const ANDROID_TYPE_WINDOW_STATE_CHANGED = 0x20;

function AndroidPresenter() {}

AndroidPresenter.prototype = new Presenter();

AndroidPresenter.prototype.pivotChanged = function(aObject, aNewContext) {
  let output = [];
  for (let i in aNewContext)
    output.push.apply(output,
                      UtteranceGenerator.genForObject(aNewContext[i]));

  output.push.apply(output,
                    UtteranceGenerator.genForObject(aObject, true));

  this.sendMessageToJava({
    gecko: {
      type: 'Accessibility:Event',
      eventType: ANDROID_TYPE_VIEW_FOCUSED,
      text: output
    }
  });
};

AndroidPresenter.prototype.actionInvoked = function(aObject, aActionName) {
  this.sendMessageToJava({
    gecko: {
      type: 'Accessibility:Event',
      eventType: ANDROID_TYPE_VIEW_CLICKED,
      text: UtteranceGenerator.genForAction(aObject, aActionName)
    }
  });
};

AndroidPresenter.prototype.tabSelected = function(aObject) {
  let vcDoc = aObject.QueryInterface(Ci.nsIAccessibleCursorable);
  let context = [];

  let parent = vcDoc.virtualCursor.position || aObject;
  while ((parent = parent.parent))
    context.push(parent);
  context.reverse();

  this.pivotChanged(vcDoc.virtualCursor.position || aObject, context);
};

AndroidPresenter.prototype.sendMessageToJava = function(aMessage) {
  return Cc['@mozilla.org/android/bridge;1'].
    getService(Ci.nsIAndroidBridge).
    handleGeckoMessage(JSON.stringify(aMessage));
};





function DummyAndroidPresenter() {}

DummyAndroidPresenter.prototype = new AndroidPresenter();

DummyAndroidPresenter.prototype.sendMessageToJava = function(aMessage) {
  dump(JSON.stringify(aMessage, null, 2) + '\n');
};
