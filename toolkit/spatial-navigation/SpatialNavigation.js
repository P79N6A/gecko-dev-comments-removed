





















































var EXPORTED_SYMBOLS = ["SpatialNavigation"];

function SpatialNavigation (browser, callback)
{
  browser.addEventListener("keypress", function (event) { _onInputKeyPress(event, callback) }, true);
};

SpatialNavigation.prototype = {
};




const Cc = Components.classes;
const Ci = Components.interfaces;

function dump(msg)
{
  var console = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
  console.logStringMessage("*** SNAV: " + msg);
}

var gDirectionalBias = 10;
var gRectFudge = 1;

function _onInputKeyPress (event, callback) {
  
  var target = event.target;

  
  if (event.keyCode != event.DOM_VK_LEFT  &&
      event.keyCode != event.DOM_VK_RIGHT &&
      event.keyCode != event.DOM_VK_UP    &&
      event.keyCode != event.DOM_VK_DOWN  )
    return;
  
  
  

  if ((target instanceof Ci.nsIDOMHTMLInputElement && (target.type == "text" || target.type == "password")) ||
      target instanceof Ci.nsIDOMHTMLTextAreaElement ) {
    
    
    if (target.selectionEnd - target.selectionStart > 0)
      return;
    
    
    
    if (target.textLength > 0) {

      if (event.keyCode == event.DOM_VK_RIGHT || event.keyCode == event.DOM_VK_DOWN  ) {
        
        if (target.textLength != target.selectionEnd)
          return;
      }
      else
      {
        
        if (target.selectionStart != 0)
          return;
      }
    }
  }

  
  if (target instanceof Ci.nsIDOMHTMLSelectElement)
  {
    if (event.keyCode == event.DOM_VK_DOWN  ) {
      if (target.selectedIndex + 1 < target.length)
        return;
    }
    
    if (event.keyCode == event.DOM_VK_UP) {
      if (target.selectedIndex > 0)
        return;
    }
  }

  function snavfilter(node) {
    
    if (node instanceof Ci.nsIDOMHTMLLinkElement ||
        node instanceof Ci.nsIDOMHTMLAnchorElement) {
      
      if (node.href == "")
        return Ci.nsIDOMNodeFilter.FILTER_SKIP;
      return  Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
    }
    
    if ((node instanceof Ci.nsIDOMHTMLButtonElement ||
         node instanceof Ci.nsIDOMHTMLInputElement ||
         node instanceof Ci.nsIDOMHTMLLinkElement ||
         node instanceof Ci.nsIDOMHTMLOptGroupElement ||
         node instanceof Ci.nsIDOMHTMLSelectElement ||
         node instanceof Ci.nsIDOMHTMLTextAreaElement) &&
        node.disabled == false)
      return Ci.nsIDOMNodeFilter.FILTER_ACCEPT;
    
    return Ci.nsIDOMNodeFilter.FILTER_SKIP;
  }
  var bestElementToFocus = null;
  var distanceToBestElement = Infinity;
  var focusedRect = _inflateRect(target.getBoundingClientRect(),
                                 - gRectFudge);
  var doc = target.ownerDocument;
  
  var treeWalker = doc.createTreeWalker(doc, Ci.nsIDOMNodeFilter.SHOW_ELEMENT, snavfilter, false);
  var nextNode;
  
  while ((nextNode = treeWalker.nextNode())) {
    
    if (nextNode == target)
      continue;

    var nextRect = _inflateRect(nextNode.getBoundingClientRect(),
                                - gRectFudge);
    
    if (! _isRectInDirection(event, focusedRect, nextRect))
      continue;
    
    var distance = _spatialDistance(event, focusedRect, nextRect);
    
    if (distance <= distanceToBestElement && distance > 0) {
      distanceToBestElement = distance;
      bestElementToFocus = nextNode;
    }
  }
  
  if (bestElementToFocus != null) {
    dump("focusing element  " + bestElementToFocus.nodeName + " " + bestElementToFocus) + "id=" + bestElementToFocus.getAttribute("id");
    
    doc.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).focus(bestElementToFocus);

    
    if((bestElementToFocus instanceof Ci.nsIDOMHTMLInputElement && (bestElementToFocus.type == "text" || bestElementToFocus.type == "password")) ||
       bestElementToFocus instanceof Ci.nsIDOMHTMLTextAreaElement ) {
      bestElementToFocus.selectionStart = 0;
      bestElementToFocus.selectionEnd = bestElementToFocus.textLength;
    }

    if (callback != undefined)
      callback(bestElementToFocus);
    
  } else {
    
    var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].getService(Ci.nsIWindowMediator);
    var window = windowMediator.getMostRecentWindow("navigator:browser");
    
    if (event.keyCode == event.DOM_VK_RIGHT || event.keyCode != event.DOM_VK_DOWN  )
      window.document.commandDispatcher.advanceFocus();
    else
      window.document.commandDispatcher.rewindFocus();
    
    if (callback != undefined)
      callback(null);
  }
  
  event.preventDefault();
  event.stopPropagation();
}

function _isRectInDirection(event, focusedRect, anotherRect)
{
    if (event.keyCode == event.DOM_VK_LEFT) {  
      return (anotherRect.left < focusedRect.left);
    }
    
    if (event.keyCode == event.DOM_VK_RIGHT) {
      return (anotherRect.right > focusedRect.right);
    }
    
    if (event.keyCode == event.DOM_VK_UP) {
      return (anotherRect.top < focusedRect.top);
    }
    
    if (event.keyCode == event.DOM_VK_DOWN) {
      return (anotherRect.bottom > focusedRect.bottom);
    }
    return false;
}

function _inflateRect(rect, value)
{
  var newRect = new Object();
  
  newRect.left   = rect.left - value;
  newRect.top    = rect.top - value;
  newRect.right  = rect.right  + value;
  newRect.bottom = rect.bottom + value;
  return newRect;
}

function _containsRect(a, b)
{
  return ( (b.left  <= a.right) &&
           (b.right >= a.left)  &&
           (b.top  <= a.bottom) &&
           (b.bottom >= a.top) );
}

function _spatialDistance(event, a, b)
{
  var inlineNavigation = false;
  var mx, my, nx, ny;
  
  if (event.keyCode == event.DOM_VK_LEFT) {
    
    
    
    
    
    
    
    
    
    
    
    if (a.top > b.bottom) {
      
      mx = a.left;
      my = a.top;
      nx = b.right;
      ny = b.bottom;
    }
    else if (a.bottom < b.top) {
      
      mx = a.left;
      my = a.bottom;
      nx = b.right;
      ny = b.top;       
    }
    else {
      mx = a.left;
      my = 0;
      nx = b.right; 
      ny = 0;    
    }
  } else if (event.keyCode == event.DOM_VK_RIGHT) {
    
    
    
    
    
    
    
    
    
    
    
    if (a.top > b.bottom) {
      
      mx = a.right;
      my = a.top;
      nx = b.left;
      ny = b.bottom;
    }
    else if (a.bottom < b.top) {
      
      mx = a.right;
      my = a.bottom;
      nx = b.left;
      ny = b.top;       
    } else {
      mx = a.right;
      my = 0;
      nx = b.left; 
      ny = 0;	    
    }
  } else if (event.keyCode == event.DOM_VK_UP) {
    
    
    
    
    
    
    
    
    if (a.left > b.right) {
      
      mx = a.left;
      my = a.top;
      nx = b.right;
      ny = b.bottom;
    } else if (a.right < b.left) {
      
      mx = a.right;
      my = a.top;
      nx = b.left;
      ny = b.bottom;       
    } else {
      
      mx = 0;
      my = a.top;
      nx = 0;
      ny = b.bottom;
    }
  } else if (event.keyCode == event.DOM_VK_DOWN) {
    
    
    
    
    
    
    
    
    if (a.left > b.right) {
      
      mx = a.left;
      my = a.bottom;
      nx = b.right;
      ny = b.top;
    } else if (a.right < b.left) {
      
      mx = a.right;
      my = a.bottom;
      nx = b.left;
      ny = b.top;      
    } else {
      
      mx = 0;
      my = a.bottom;
      nx = 0;
      ny = b.top;
    }
  }
  
  var scopedRect = _inflateRect(a, gRectFudge);
  
  if (event.keyCode == event.DOM_VK_LEFT || 
      event.keyCode == event.DOM_VK_RIGHT) {
    scopedRect.left = 0;
    scopedRect.right = Infinity;
    inlineNavigation = _containsRect(scopedRect, b);
  }
  else if (event.keyCode == event.DOM_VK_UP ||
           event.keyCode == event.DOM_VK_DOWN) {
    scopedRect.top = 0;
    scopedRect.bottom = Infinity;
    inlineNavigation = _containsRect(scopedRect, b);
  }
  
  var d = Math.pow((mx-nx), 2) + Math.pow((my-ny), 2);
  
  
  if (inlineNavigation)
    d /= gDirectionalBias;
  
  return d;
}

