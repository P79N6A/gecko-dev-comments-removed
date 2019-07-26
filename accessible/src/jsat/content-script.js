



let Ci = Components.interfaces;
let Cu = Components.utils;

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

Logger.debug('content-script.js');

let eventManager = null;

function virtualCursorControl(aMessage) {
  if (Logger.logLevel >= Logger.DEBUG)
    Logger.debug(aMessage.name, JSON.stringify(aMessage.json));

  try {
    let vc = Utils.getVirtualCursor(content.document);
    let origin = aMessage.json.origin;
    if (origin != 'child') {
      if (forwardMessage(vc, aMessage))
        return;
    }

    let details = aMessage.json;
    let rule = TraversalRules[details.rule];
    let moved = 0;
    switch (details.action) {
    case 'moveFirst':
    case 'moveLast':
      moved = vc[details.action](rule);
      break;
    case 'moveNext':
    case 'movePrevious':
      try {
        if (origin == 'parent' && vc.position == null) {
          if (details.action == 'moveNext')
            moved = vc.moveFirst(rule);
          else
            moved = vc.moveLast(rule);
        } else {
          moved = vc[details.action](rule);
        }
      } catch (x) {
        let acc = Utils.AccRetrieval.
          getAccessibleFor(content.document.activeElement);
        moved = vc.moveNext(rule, acc, true);
      }
      break;
    case 'moveToPoint':
      if (!this._ppcp) {
        this._ppcp = Utils.getPixelsPerCSSPixel(content);
      }
      moved = vc.moveToPoint(rule,
                             details.x * this._ppcp, details.y * this._ppcp,
                             true);
      break;
    case 'whereIsIt':
      if (!forwardMessage(vc, aMessage)) {
        if (!vc.position && aMessage.json.move)
          vc.moveFirst(TraversalRules.Simple);
        else {
          sendAsyncMessage('AccessFu:Present', Presentation.pivotChanged(
            vc.position, null, Ci.nsIAccessiblePivot.REASON_NONE));
        }
      }

      break;
    default:
      break;
    }

    if (moved == true) {
      forwardMessage(vc, aMessage);
    } else if (moved == false && details.action != 'moveToPoint') {
      if (origin == 'parent') {
        vc.position = null;
      }
      aMessage.json.origin = 'child';
      sendAsyncMessage('AccessFu:VirtualCursor', aMessage.json);
    }
  } catch (x) {
    Logger.error(x);
  }
}

function forwardMessage(aVirtualCursor, aMessage) {
  try {
    let acc = aVirtualCursor.position;
    if (acc && acc.role == Ci.nsIAccessibleRole.ROLE_INTERNAL_FRAME) {
      let mm = Utils.getMessageManager(acc.DOMNode);
      mm.addMessageListener(aMessage.name, virtualCursorControl);
      aMessage.json.origin = 'parent';
      if (Utils.isContentProcess) {
        
        
        aMessage.json.x -= content.mozInnerScreenX;
        aMessage.json.y -= content.mozInnerScreenY;
      }
      mm.sendAsyncMessage(aMessage.name, aMessage.json);
      return true;
    }
  } catch (x) {
    
  }
  return false;
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

  let vc = Utils.getVirtualCursor(content.document);
  if (!forwardMessage(vc, aMessage))
    activateAccessible(vc.position);
}

function scroll(aMessage) {
  let vc = Utils.getVirtualCursor(content.document);

  function tryToScroll() {
    let horiz = aMessage.json.horizontal;
    let page = aMessage.json.page;

    
    let acc = vc.position;
    while (acc) {
      let elem = acc.DOMNode;

      
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

        let controllers = acc.
          getRelationByType(
            Ci.nsIAccessibleRelation.RELATION_CONTROLLED_BY);
        for (let i = 0; controllers.targetsCount > i; i++) {
          let controller = controllers.getTarget(i);
          
          
          if (controller.role == Ci.nsIAccessibleRole.ROLE_SLIDER) {
            
            let evt = content.document.createEvent('KeyboardEvent');
            evt.initKeyEvent(
              'keypress', true, true, null,
              true, false, false, false,
              (page > 0) ? evt.DOM_VK_RIGHT : evt.DOM_VK_LEFT, 0);
            controller.DOMNode.dispatchEvent(evt);
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

  if (aMessage.json.origin != 'child') {
    if (forwardMessage(vc, aMessage))
      return;
  }

  if (!tryToScroll()) {
    
    aMessage.json.origin = 'child';
    sendAsyncMessage('AccessFu:Scroll', aMessage.json);
  }
}

addMessageListener(
  'AccessFu:Start',
  function(m) {
    Logger.debug('AccessFu:Start');
    if (m.json.buildApp)
      Utils.MozBuildApp = m.json.buildApp;

    addMessageListener('AccessFu:VirtualCursor', virtualCursorControl);
    addMessageListener('AccessFu:Activate', activateCurrent);
    addMessageListener('AccessFu:Scroll', scroll);

    if (!eventManager) {
      eventManager = new EventManager(this);
    }
    eventManager.start();
  });

addMessageListener(
  'AccessFu:Stop',
  function(m) {
    Logger.debug('AccessFu:Stop');

    removeMessageListener('AccessFu:VirtualCursor', virtualCursorControl);
    removeMessageListener('AccessFu:Activate', activateCurrent);
    removeMessageListener('AccessFu:Scroll', scroll);

    eventManager.stop();
  });

sendAsyncMessage('AccessFu:Ready');
