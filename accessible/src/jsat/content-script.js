



let Ci = Components.interfaces;
let Cu = Components.utils;

const MOVEMENT_GRANULARITY_CHARACTER = 1;
const MOVEMENT_GRANULARITY_WORD = 2;
const MOVEMENT_GRANULARITY_PARAGRAPH = 8;

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
XPCOMUtils.defineLazyModuleGetter(this, 'ContentControl',
  'resource://gre/modules/accessibility/ContentControl.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Roles',
  'resource://gre/modules/accessibility/Constants.jsm');

Logger.debug('content-script.js');

let eventManager = null;
let contentControl = null;

function forwardToParent(aMessage) {
  
  let newJSON = JSON.parse(JSON.stringify(aMessage.json));
  newJSON.origin = 'child';
  sendAsyncMessage(aMessage.name, newJSON);
}

function forwardToChild(aMessage, aListener, aVCPosition) {
  let acc = aVCPosition || Utils.getVirtualCursor(content.document).position;

  if (!Utils.isAliveAndVisible(acc) || acc.role != Roles.INTERNAL_FRAME) {
    return false;
  }

  Logger.debug(() => {
    return ['forwardToChild', Logger.accessibleToString(acc),
            aMessage.name, JSON.stringify(aMessage.json, null, '  ')];
  });

  let mm = Utils.getMessageManager(acc.DOMNode);

  if (aListener) {
    mm.addMessageListener(aMessage.name, aListener);
  }

  
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
    try {
      if (aMessage.json.activateIfKey &&
          aAccessible.role != Roles.KEY) {
        
        return;
      }
    } catch (e) {
      
      return;
    }

    if (aAccessible.actionCount > 0) {
      aAccessible.doAction(0);
    } else {
      let control = Utils.getEmbeddedControl(aAccessible);
      if (control && control.actionCount > 0) {
        control.doAction(0);
      }

      
      
      
      
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

    if (aAccessible.role !== Roles.KEY) {
      
      sendAsyncMessage('AccessFu:Present',
                       Presentation.actionInvoked(aAccessible, 'click'));
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
  if (focusedAcc && focusedAcc.role === Roles.ENTRY) {
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

function moveByGranularity(aMessage) {
  let direction = aMessage.json.direction;
  let vc = Utils.getVirtualCursor(content.document);
  let granularity;

  switch(aMessage.json.granularity) {
    case MOVEMENT_GRANULARITY_CHARACTER:
      granularity = Ci.nsIAccessiblePivot.CHAR_BOUNDARY;
      break;
    case MOVEMENT_GRANULARITY_WORD:
      granularity = Ci.nsIAccessiblePivot.WORD_BOUNDARY;
      break;
    default:
      return;
  }

  if (direction === 'Previous') {
    vc.movePreviousByText(granularity);
  } else if (direction === 'Next') {
    vc.moveNextByText(granularity);
  }
}

function moveCaret(aMessage) {
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
  function sendScrollCoordinates(aAccessible) {
    let bounds = Utils.getBounds(aAccessible);
    sendAsyncMessage('AccessFu:DoScroll',
                     { bounds: bounds,
                       page: aMessage.json.page,
                       horizontal: aMessage.json.horizontal });
  }

  let position = Utils.getVirtualCursor(content.document).position;
  if (!forwardToChild(aMessage, scroll, position)) {
    sendScrollCoordinates(position);
  }
}

function adjustRange(aMessage) {
  function sendUpDownKey(aAccessible) {
    let acc = Utils.getEmbeddedControl(aAccessible) || aAccessible;
    let elem = acc.DOMNode;
    if (elem) {
      if (elem.tagName === 'INPUT' && elem.type === 'range') {
        elem[aMessage.json.direction === 'forward' ? 'stepDown' : 'stepUp']();
        let changeEvent = content.document.createEvent('UIEvent');
        changeEvent.initEvent('change', true, true);
        elem.dispatchEvent(changeEvent);
      } else {
        let evt = content.document.createEvent('KeyboardEvent');
        let keycode = aMessage.json.direction == 'forward' ?
              content.KeyEvent.DOM_VK_DOWN : content.KeyEvent.DOM_VK_UP;
        evt.initKeyEvent(
          "keypress", false, true, null, false, false, false, false, keycode, 0);
        elem.dispatchEvent(evt);
      }
    }
  }

  let position = Utils.getVirtualCursor(content.document).position;
  if (!forwardToChild(aMessage, adjustRange, position)) {
    sendUpDownKey(position);
  }
}
addMessageListener(
  'AccessFu:Start',
  function(m) {
    Logger.debug('AccessFu:Start');
    if (m.json.buildApp)
      Utils.MozBuildApp = m.json.buildApp;

    addMessageListener('AccessFu:Activate', activateCurrent);
    addMessageListener('AccessFu:ContextMenu', activateContextMenu);
    addMessageListener('AccessFu:Scroll', scroll);
    addMessageListener('AccessFu:AdjustRange', adjustRange);
    addMessageListener('AccessFu:MoveCaret', moveCaret);
    addMessageListener('AccessFu:MoveByGranularity', moveByGranularity);

    if (!eventManager) {
      eventManager = new EventManager(this);
    }
    eventManager.start();

    if (!contentControl) {
      contentControl = new ContentControl(this);
    }
    contentControl.start();

    sendAsyncMessage('AccessFu:ContentStarted');
  });

addMessageListener(
  'AccessFu:Stop',
  function(m) {
    Logger.debug('AccessFu:Stop');

    removeMessageListener('AccessFu:Activate', activateCurrent);
    removeMessageListener('AccessFu:ContextMenu', activateContextMenu);
    removeMessageListener('AccessFu:Scroll', scroll);
    removeMessageListener('AccessFu:MoveCaret', moveCaret);
    removeMessageListener('AccessFu:MoveByGranularity', moveByGranularity);

    eventManager.stop();
    contentControl.stop();
  });

sendAsyncMessage('AccessFu:Ready');
