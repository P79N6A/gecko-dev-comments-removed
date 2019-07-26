



let Ci = Components.interfaces;
let Cu = Components.utils;

const ROLE_ENTRY = Ci.nsIAccessibleRole.ROLE_ENTRY;
const ROLE_INTERNAL_FRAME = Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Presentation',
  'resource://gre/modules/accessibility/Presentation.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'TraversalRules',
  'resource://gre/modules/accessibility/TraversalRules.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Utils',
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'EventManager',
  'resource://gre/modules/accessibility/EventManager.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'ObjectWrapper',
  'resource://gre/modules/ObjectWrapper.jsm');

Logger.debug('content-script.js');

let eventManager = null;

function moveCursor(aMessage) {
  if (Logger.logLevel >= Logger.DEBUG) {
    Logger.debug(aMessage.name, JSON.stringify(aMessage.json, null, ' '));
  }

  let vc = Utils.getVirtualCursor(content.document);
  let origin = aMessage.json.origin;
  let action = aMessage.json.action;
  let rule = TraversalRules[aMessage.json.rule];

  function moveCursorInner() {
    try {
      if (origin == 'parent' &&
          !Utils.isAliveAndVisible(vc.position)) {
        
        if (action == 'moveNext') {
          return vc.moveFirst(rule);
        } else if (action == 'movePrevious') {
          return vc.moveLast(rule);
        }
      }

      return vc[action](rule);
    } catch (x) {
      if (action == 'moveNext' || action == 'movePrevious') {
        
        let acc = Utils.AccRetrieval.
          getAccessibleFor(content.document.activeElement);
        return vc.moveNext(rule, acc, true);
      } else {
        throw x;
      }
    }

    return false;
  }

  try {
    if (origin != 'child' &&
        forwardToChild(aMessage, moveCursor, vc.position)) {
      
      return;
    }

    if (moveCursorInner()) {
      
      
      forwardToChild(aMessage, moveCursor, vc.position);
    } else {
      
      
      if (origin == 'parent') {
        vc.position = null;
      }
      forwardToParent(aMessage);
    }
  } catch (x) {
    Logger.logException(x, 'Cursor move failed');
  }
}

function moveToPoint(aMessage) {
  if (Logger.logLevel >= Logger.DEBUG) {
    Logger.debug(aMessage.name, JSON.stringify(aMessage.json, null, ' '));
  }

  let vc = Utils.getVirtualCursor(content.document);
  let details = aMessage.json;
  let rule = TraversalRules[details.rule];

  try {
    let dpr = content.devicePixelRatio;
    vc.moveToPoint(rule, details.x * dpr, details.y * dpr, true);
    forwardToChild(aMessage, moveToPoint, vc.position);
  } catch (x) {
    Logger.logException(x, 'Failed move to point');
  }
}

function showCurrent(aMessage) {
  if (Logger.logLevel >= Logger.DEBUG) {
    Logger.debug(aMessage.name, JSON.stringify(aMessage.json, null, ' '));
  }

  let vc = Utils.getVirtualCursor(content.document);

  if (!forwardToChild(vc, showCurrent, aMessage)) {
    if (!vc.position && aMessage.json.move) {
      vc.moveFirst(TraversalRules.Simple);
    } else {
      sendAsyncMessage('AccessFu:Present', Presentation.pivotChanged(
                         vc.position, null, Ci.nsIAccessiblePivot.REASON_NONE));
    }
  }
}

function forwardToParent(aMessage) {
  
  let newJSON = JSON.parse(JSON.stringify(aMessage.json));
  newJSON.origin = 'child';
  sendAsyncMessage(aMessage.name, newJSON);
}

function forwardToChild(aMessage, aListener, aVCPosition) {
  let acc = aVCPosition || Utils.getVirtualCursor(content.document).position;

  if (!Utils.isAliveAndVisible(acc) || acc.role != ROLE_INTERNAL_FRAME) {
    return false;
  }

  if (Logger.logLevel >= Logger.DEBUG) {
    Logger.debug('forwardToChild', Logger.accessibleToString(acc),
                 aMessage.name, JSON.stringify(aMessage.json, null, '  '));
  }

  let mm = Utils.getMessageManager(acc.DOMNode);
  mm.addMessageListener(aMessage.name, aListener);
  
  let newJSON = JSON.parse(JSON.stringify(aMessage.json));
  newJSON.origin = 'parent';
  if (Utils.isContentProcess) {
    
    
    newJSON.x -= content.mozInnerScreenX;
    newJSON.y -= content.mozInnerScreenY;
  }
  mm.sendAsyncMessage(aMessage.name, newJSON);
  return true;
}

function activateCurrent(aMessage) {
  Logger.debug('activateCurrent');
  function activateAccessible(aAccessible) {
    if (aAccessible.actionCount > 0) {
      aAccessible.doAction(0);
    } else {
      
      
      
      
      let docAcc = Utils.AccRetrieval.getAccessibleFor(content.document);
      let docX = {}, docY = {}, docW = {}, docH = {};
      docAcc.getBounds(docX, docY, docW, docH);

      let objX = {}, objY = {}, objW = {}, objH = {};
      aAccessible.getBounds(objX, objY, objW, objH);

      let x = Math.round((objX.value - docX.value) + objW.value / 2);
      let y = Math.round((objY.value - docY.value) + objH.value / 2);

      let node = aAccessible.DOMNode || aAccessible.parent.DOMNode;

      function dispatchMouseEvent(aEventType) {
        let evt = content.document.createEvent('MouseEvents');
        evt.initMouseEvent(aEventType, true, true, content,
                           x, y, 0, 0, 0, false, false, false, false, 0, null);
        node.dispatchEvent(evt);
      }

      dispatchMouseEvent('mousedown');
      dispatchMouseEvent('mouseup');
    }
  }

  function moveCaretTo(aAccessible, aOffset) {
    let accText = aAccessible.QueryInterface(Ci.nsIAccessibleText);
    let oldOffset = accText.caretOffset;
    let text = accText.getText(0, accText.characterCount);

    if (aOffset >= 0 && aOffset <= accText.characterCount) {
      accText.caretOffset = aOffset;
    }

    presentCaretChange(text, oldOffset, accText.caretOffset);
  }

  let focusedAcc = Utils.AccRetrieval.getAccessibleFor(content.document.activeElement);
  if (focusedAcc && focusedAcc.role === ROLE_ENTRY) {
    moveCaretTo(focusedAcc, aMessage.json.offset);
    return;
  }

  let position = Utils.getVirtualCursor(content.document).position;
  if (!forwardToChild(aMessage, activateCurrent, position)) {
    activateAccessible(position);
  }
}

function activateContextMenu(aMessage) {
  function sendContextMenuCoordinates(aAccessible) {
    let bounds = Utils.getBounds(aAccessible);
    sendAsyncMessage('AccessFu:ActivateContextMenu', {bounds: bounds});
  }

  let position = Utils.getVirtualCursor(content.document).position;
  if (!forwardToChild(aMessage, activateContextMenu, position)) {
    sendContextMenuCoordinates(position);
  }
}

function moveCaret(aMessage) {
  const MOVEMENT_GRANULARITY_CHARACTER = 1;
  const MOVEMENT_GRANULARITY_WORD = 2;
  const MOVEMENT_GRANULARITY_PARAGRAPH = 8;

  let direction = aMessage.json.direction;
  let granularity = aMessage.json.granularity;
  let accessible = Utils.getVirtualCursor(content.document).position;
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
        accText.caretOffset = end.value === accText.caretOffset ? start.value : end.value;
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

  presentCaretChange(text, oldOffset, accText.caretOffset);
}

function presentCaretChange(aText, aOldOffset, aNewOffset) {
  if (aOldOffset !== aNewOffset) {
    let msg = Presentation.textSelectionChanged(aText, aNewOffset, aNewOffset,
                                                aOldOffset, aOldOffset, true);
    sendAsyncMessage('AccessFu:Present', msg);
  }
}

function scroll(aMessage) {
  let vc = Utils.getVirtualCursor(content.document);

  function tryToScroll() {
    let horiz = aMessage.json.horizontal;
    let page = aMessage.json.page;

    
    let acc = vc.position;
    while (acc) {
      let elem = acc.DOMNode;

      
      
      
      let uiactions = elem.getAttribute ? elem.getAttribute('uiactions') : '';
      if (uiactions && uiactions.split(' ').indexOf('scroll') >= 0) {
        let evt = elem.ownerDocument.createEvent('CustomEvent');
        let details = horiz ? { deltaX: page * elem.clientWidth } :
          { deltaY: page * elem.clientHeight };
        evt.initCustomEvent(
          'scrollrequest', true, true,
          ObjectWrapper.wrap(details, elem.ownerDocument.defaultView));
        if (!elem.dispatchEvent(evt))
          return;
      }

      
      if (elem == content.document)
        break;

      if (!horiz && elem.clientHeight < elem.scrollHeight) {
        let s = content.getComputedStyle(elem);
        if (s.overflowY == 'scroll' || s.overflowY == 'auto') {
          elem.scrollTop += page * elem.clientHeight;
          return true;
        }
      }

      if (horiz) {
        if (elem.clientWidth < elem.scrollWidth) {
          let s = content.getComputedStyle(elem);
          if (s.overflowX == 'scroll' || s.overflowX == 'auto') {
            elem.scrollLeft += page * elem.clientWidth;
            return true;
          }
        }
      }
      acc = acc.parent;
    }

    
    if (!horiz && content.scrollMaxY &&
        ((page > 0 && content.scrollY < content.scrollMaxY) ||
         (page < 0 && content.scrollY > 0))) {
      content.scroll(0, content.innerHeight * page + content.scrollY);
      return true;
    } else if (horiz && content.scrollMaxX &&
               ((page > 0 && content.scrollX < content.scrollMaxX) ||
                (page < 0 && content.scrollX > 0))) {
      content.scroll(content.innerWidth * page + content.scrollX);
      return true;
    }

    return false;
  }

  if (aMessage.json.origin != 'child' &&
      forwardToChild(aMessage, scroll, vc.position)) {
    return;
  }

  if (!tryToScroll()) {
    
    forwardToParent(aMessage);
  }
}

addMessageListener(
  'AccessFu:Start',
  function(m) {
    Logger.debug('AccessFu:Start');
    if (m.json.buildApp)
      Utils.MozBuildApp = m.json.buildApp;

    addMessageListener('AccessFu:MoveToPoint', moveToPoint);
    addMessageListener('AccessFu:MoveCursor', moveCursor);
    addMessageListener('AccessFu:ShowCurrent', showCurrent);
    addMessageListener('AccessFu:Activate', activateCurrent);
    addMessageListener('AccessFu:ContextMenu', activateContextMenu);
    addMessageListener('AccessFu:Scroll', scroll);
    addMessageListener('AccessFu:MoveCaret', moveCaret);

    if (!eventManager) {
      eventManager = new EventManager(this);
    }
    eventManager.start();
  });

addMessageListener(
  'AccessFu:Stop',
  function(m) {
    Logger.debug('AccessFu:Stop');

    removeMessageListener('AccessFu:MoveToPoint', moveToPoint);
    removeMessageListener('AccessFu:MoveCursor', moveCursor);
    removeMessageListener('AccessFu:ShowCurrent', showCurrent);
    removeMessageListener('AccessFu:Activate', activateCurrent);
    removeMessageListener('AccessFu:ContextMenu', activateContextMenu);
    removeMessageListener('AccessFu:Scroll', scroll);
    removeMessageListener('AccessFu:MoveCaret', moveCaret);

    eventManager.stop();
  });

sendAsyncMessage('AccessFu:Ready');
