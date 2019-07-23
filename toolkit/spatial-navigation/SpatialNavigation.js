




















































var EXPORTED_SYMBOLS = ["SpatialNavigation"];

var SpatialNavigation = {

  init: function(browser, callback) {
    browser.addEventListener("keypress", function (event) { _onInputKeyPress(event, callback) }, true);
  },
  
  uninit: function() {
  }
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


const kAlt   = "alt";
const kShift = "shift";
const kCtrl  = "ctrl";
const kNone  = "none";

function _onInputKeyPress (event, callback) {

  
  if (!PrefObserver['enabled'])
    return;

  
  
  
  var key = event.which || event.keyCode;

  if (key != PrefObserver['keyCodeDown']  &&
      key != PrefObserver['keyCodeRight'] &&
      key != PrefObserver['keyCodeUp'] &&
      key != PrefObserver['keyCodeLeft'])
    return;

  
  if (!event.altKey && PrefObserver['modifierAlt'])
    return;

  if (!event.shiftKey && PrefObserver['modifierShift'])
    return;

  if (!event.crtlKey && PrefObserver['modifierCtrl'])
    return;

  
  
  if (!event.keyCode && 
      (key == Ci.nsIDOMKeyEvent.DOM_VK_LEFT  || key == Ci.nsIDOMKeyEvent.DOM_VK_DOWN ||
       key == Ci.nsIDOMKeyEvent.DOM_VK_RIGHT || key == Ci.nsIDOMKeyEvent.DOM_VK_UP))
    return;

  var target = event.target;

  var doc = target.ownerDocument;

  
  if (!PrefObserver['xulContentEnabled'] && doc instanceof Ci.nsIDOMXULDocument)
    return ;

  
  
  if (target instanceof Ci.nsIDOMHTMLHtmlElement) {
      _focusNextUsingCmdDispatcher(key, callback);
      return;
  }

  if ((target instanceof Ci.nsIDOMHTMLInputElement && (target.type == "text" || target.type == "password")) ||
      target instanceof Ci.nsIDOMHTMLTextAreaElement ) {
    
    
    if (target.selectionEnd - target.selectionStart > 0)
      return;
    
    
    if (target.textLength > 0) {
      if (key == PrefObserver['keyCodeRight'] ||
          key == PrefObserver['keyCodeDown'] ) {
        
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
    if (key == PrefObserver['keyCodeDown']) {
      if (target.selectedIndex + 1 < target.length)
        return;
    }

    if (key == PrefObserver['keyCodeUp']) {
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

  var treeWalker = doc.createTreeWalker(doc, Ci.nsIDOMNodeFilter.SHOW_ELEMENT, snavfilter, false);
  var nextNode;
  
  while ((nextNode = treeWalker.nextNode())) {

    if (nextNode == target)
      continue;

    var nextRect = _inflateRect(nextNode.getBoundingClientRect(),
                                - gRectFudge);

    if (! _isRectInDirection(key, focusedRect, nextRect))
      continue;

    var distance = _spatialDistance(key, focusedRect, nextRect);

    
    
    if (distance <= distanceToBestElement && distance > 0) {
      distanceToBestElement = distance;
      bestElementToFocus = nextNode;
    }
  }
  
  if (bestElementToFocus != null) {
    

    
    doc.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).focus(bestElementToFocus);

    
    if((bestElementToFocus instanceof Ci.nsIDOMHTMLInputElement && (bestElementToFocus.type == "text" || bestElementToFocus.type == "password")) ||
       bestElementToFocus instanceof Ci.nsIDOMHTMLTextAreaElement ) {
      bestElementToFocus.selectionStart = 0;
      bestElementToFocus.selectionEnd = bestElementToFocus.textLength;
    }

    if (callback != undefined)
      callback(bestElementToFocus);
    
  } else {
    
    _focusNextUsingCmdDispatcher(key, callback);
  }

  event.preventDefault();
  event.stopPropagation();
}

function _focusNextUsingCmdDispatcher(key, callback) {

    var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].getService(Ci.nsIWindowMediator);
    var window = windowMediator.getMostRecentWindow("navigator:browser");

    if (key == PrefObserver['keyCodeRight'] || key == PrefObserver['keyCodeDown']) {
      window.document.commandDispatcher.advanceFocus();
    } else {
      window.document.commandDispatcher.rewindFocus();
    }

    if (callback != undefined)
      callback(null);
}

function _isRectInDirection(key, focusedRect, anotherRect)
{
  if (key == PrefObserver['keyCodeLeft']) {
    return (anotherRect.left < focusedRect.left);
  }

  if (key == PrefObserver['keyCodeRight']) {
    return (anotherRect.right > focusedRect.right);
  }

  if (key == PrefObserver['keyCodeUp']) {
    return (anotherRect.top < focusedRect.top);
  }

  if (key == PrefObserver['keyCodeDown']) {
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

function _spatialDistance(key, a, b)
{
  var inlineNavigation = false;
  var mx, my, nx, ny;

  if (key == PrefObserver['keyCodeLeft']) {

    
    
    
    
    
    
    
    
    
    
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
  } else if (key == PrefObserver['keyCodeRight']) {

    
    
    
    
    
    
    
    
    
    
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
  } else if (key == PrefObserver['keyCodeUp']) {

    
    
    
    
    
    
    
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
  } else if (key == PrefObserver['keyCodeDown']) {

    
    
    
    
    
    
    
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

  if (key == PrefObserver['keyCodeLeft'] ||
      key == PrefObserver['keyCodeRight']) {
    scopedRect.left = 0;
    scopedRect.right = Infinity;
    inlineNavigation = _containsRect(scopedRect, b);
  }
  else if (key == PrefObserver['keyCodeUp'] ||
           key == PrefObserver['keyCodeDown']) {
    scopedRect.top = 0;
    scopedRect.bottom = Infinity;
    inlineNavigation = _containsRect(scopedRect, b);
  }
  
  var d = Math.pow((mx-nx), 2) + Math.pow((my-ny), 2);
  
  
  if (inlineNavigation)
    d /= gDirectionalBias;
  
  return d;
}



PrefObserver = {

  register: function()
  {
    this.prefService = Cc["@mozilla.org/preferences-service;1"]
                       .getService(Ci.nsIPrefService);

    this._branch = this.prefService.getBranch("snav.");
    this._branch.QueryInterface(Ci.nsIPrefBranch2);
    this._branch.addObserver("", this, false);

    
    this.observe(null, "nsPref:changed", "enabled");
    this.observe(null, "nsPref:changed", "xulContentEnabled");
    this.observe(null, "nsPref:changed", "keyCode.modifier");
    this.observe(null, "nsPref:changed", "keyCode.right");
    this.observe(null, "nsPref:changed", "keyCode.up");
    this.observe(null, "nsPref:changed", "keyCode.down");
    this.observe(null, "nsPref:changed", "keyCode.left");
  },

  observe: function(aSubject, aTopic, aData)
  {
    if(aTopic != "nsPref:changed")
      return;

    
    
    switch (aData) {
      case "enabled":
        try {
          this.enabled = this._branch.getBoolPref("enabled");
        } catch(e) {
          this.enabled = false;
        }
        break;
      case "xulContentEnabled":
        try {
          this.xulContentEnabled = this._branch.getBoolPref("xulContentEnabled");
        } catch(e) {
          this.xulContentEnabled = false;
        }
        break;

      case "keyCode.modifier":
        try {
          this.keyCodeModifier = this._branch.getCharPref("keyCode.modifier");

          
          this.modifierAlt = false;
          this.modifierShift = false;
          this.modifierCtrl = false;

          if (this.keyCodeModifier != this.kNone)
          {
            
            var mods = this.keyCodeModifier.split(/\++/);
            for (var i = 0; i < mods.length; i++) {
              var mod = mods[i].toLowerCase();
              if (mod == "")
                continue;
              else if (mod == kAlt)
                this.modifierAlt = true;
              else if (mod == kShift)
                this.modifierShift = true;
              else if (mod == kCtrl)
                this.modifierCtrl = true;
              else {
                this.keyCodeModifier = kNone;
                break;
              }
            }
          }
        } catch(e) {
            this.keyCodeModifier = kNone;
        }
        break;
      case "keyCode.up":
        try {
          this.keyCodeUp = this._branch.getIntPref("keyCode.up");
        } catch(e) {
          this.keyCodeUp = Ci.nsIDOMKeyEvent.DOM_VK_UP;
        }
        break;
      case "keyCode.down":
        try {
          this.keyCodeDown = this._branch.getIntPref("keyCode.down");
        } catch(e) {
          this.keyCodeDown = Ci.nsIDOMKeyEvent.DOM_VK_DOWN;
        }
        break;
      case "keyCode.left":
        try {
          this.keyCodeLeft = this._branch.getIntPref("keyCode.left");
        } catch(e) {
          this.keyCodeLeft = Ci.nsIDOMKeyEvent.DOM_VK_LEFT;
        }
        break;
      case "keyCode.right":
        try {
          this.keyCodeRight = this._branch.getIntPref("keyCode.right");
        } catch(e) {
          this.keyCodeRight = Ci.nsIDOMKeyEvent.DOM_VK_RIGHT;
        }
        break;
    }
  },
}

PrefObserver.register();
