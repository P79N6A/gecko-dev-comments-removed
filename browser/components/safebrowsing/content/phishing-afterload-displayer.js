





































































function PROT_PhishMsgDisplayer(msgDesc, browser, doc, url) {

  
  

  return new PROT_PhishMsgDisplayerCanvas(msgDesc, browser, doc, url);
}















function PROT_PhishMsgDisplayerBase(msgDesc, browser, doc, url) {
  this.debugZone = "phishdisplayer";
  this.msgDesc_ = msgDesc;                                
  this.browser_ = browser;
  this.doc_ = doc;
  this.url_ = url;

  
  this.messageId_ = "safebrowsing-palm-message";
  this.messageTailId_ = "safebrowsing-palm-message-tail-container";
  this.messageContentId_ = "safebrowsing-palm-message-content";
  this.extendedMessageId_ = "safebrowsing-palm-extended-message";
  this.showmoreLinkId_ = "safebrowsing-palm-showmore-link";
  this.faqLinkId_ = "safebrowsing-palm-faq-link";
  this.urlbarIconId_ = "safebrowsing-urlbar-icon";
  this.refElementId_ = this.urlbarIconId_;

  
  this.reporter_ = new PROT_Reporter();

  
  
  
  this.commandHandlers_ = {
    "safebrowsing-palm-showmore":
      BindToObject(this.showMore_, this),
  };

  this.windowWatcher_ = 
    Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Components.interfaces.nsIWindowWatcher);
}




PROT_PhishMsgDisplayerBase.prototype.getBackgroundColor_ = function() {
  var pref = Components.classes["@mozilla.org/preferences-service;1"].
             getService(Components.interfaces.nsIPrefBranch);
  return pref.getCharPref("browser.display.background_color");
}




PROT_PhishMsgDisplayerBase.prototype.declineAction = function() {
  G_Debug(this, "User declined warning.");
  G_Assert(this, this.started_, "Decline on a non-active displayer?");
  this.reporter_.report("phishdecline", this.url_);

  this.messageShouldShow_ = false;
  if (this.messageShowing_)
    this.hideMessage_();
}




PROT_PhishMsgDisplayerBase.prototype.acceptAction = function() {
  G_Assert(this, this.started_, "Accept on an unstarted displayer?");
  G_Assert(this, this.done_, "Accept on a finished displayer?");
  G_Debug(this, "User accepted warning.");
  this.reporter_.report("phishaccept", this.url_);

  var url = this.getMeOutOfHereUrl_();
  this.browser_.loadURI(url);
}






PROT_PhishMsgDisplayerBase.prototype.getMeOutOfHereUrl_ = function() {
  
  var prefs = Cc["@mozilla.org/preferences-service;1"]
              .getService(Ci.nsIPrefService).getDefaultBranch(null);

  var url = "about:blank";
  try {
    url = prefs.getComplexValue("browser.startup.homepage",
                                Ci.nsIPrefLocalizedString).data;
    
    
    if (url.indexOf("|") != -1)
      url = url.split("|")[0];
  } catch(e) {
    G_Debug(this, "Couldn't get homepage pref: " + e);
  }
  
  return url;
}




PROT_PhishMsgDisplayerBase.prototype.onBrowserResized_ = function(event) {
  G_Debug(this, "Got resize for " + event.target);

  if (this.messageShowing_) {
    this.hideMessage_(); 
    this.showMessage_();
  }
}




PROT_PhishMsgDisplayerBase.prototype.browserSelected = function() {
  G_Assert(this, this.started_, "Displayer selected before being started???");

  
  
  
  if (this.messageShowing_ === undefined) {
    this.messageShouldShow_ = true;
  }

  this.hideLockIcon_();        
  this.addWarningInUrlbar_();  

  
  
  
  
  if (this.messageShouldShow_)
    this.showMessage_();
}





PROT_PhishMsgDisplayerBase.prototype.explicitShow = function() {
  this.messageShouldShow_ = true;
  if (!this.messageShowing_)
    this.showMessage_();
}




PROT_PhishMsgDisplayerBase.prototype.browserUnselected = function() {
  this.removeWarningInUrlbar_();
  this.unhideLockIcon_();
  if (this.messageShowing_)
    this.hideMessage_();
}













PROT_PhishMsgDisplayerBase.prototype.start = function() {
  G_Assert(this, this.started_ == undefined, "Displayer started twice?");
  this.started_ = true;

  this.commandController_ = new PROT_CommandController(this.commandHandlers_);
  this.doc_.defaultView.controllers.appendController(this.commandController_);

  
  
  this.resizeHandler_ = BindToObject(this.onBrowserResized_, this);
  this.browser_.addEventListener("resize",
                                 this.resizeHandler_, 
                                 false);
}





PROT_PhishMsgDisplayerBase.prototype.isActive = function() {
  return !!this.started_;
}






PROT_PhishMsgDisplayerBase.prototype.done = function() {
  G_Assert(this, !this.done_, "Called done more than once?");
  this.done_ = true;

  
  
  if (this.started_) {

    
    
    this.removeWarningInUrlbar_();
    this.unhideLockIcon_();

    
    if (this.messageShowing_)
      this.hideMessage_();

    if (this.resizeHandler_) {
      this.browser_.removeEventListener("resize", 
                                        this.resizeHandler_, 
                                        false);
      this.resizeHandler_ = null;
    }
    
    var win = this.doc_.defaultView;
    win.controllers.removeController(this.commandController_);
    this.commandController_ = null;
  }
}










PROT_PhishMsgDisplayerBase.prototype.removeIfExists_ = function(orig,
                                                                toRemove) {
  var pos = orig.indexOf(toRemove);
  if (pos != -1)
    orig = orig.substring(0, pos) + orig.substring(pos + toRemove.length);

  return orig;
}






PROT_PhishMsgDisplayerBase.prototype.hideLockIcon_ = function() {
  var lockIcon = this.doc_.getElementById("lock-icon");
  if (!lockIcon)
    return;
  lockIcon.hidden = true;
}




PROT_PhishMsgDisplayerBase.prototype.unhideLockIcon_ = function() {
  var lockIcon = this.doc_.getElementById("lock-icon");
  if (!lockIcon)
    return;
  lockIcon.hidden = false;
}






PROT_PhishMsgDisplayerBase.prototype.addWarningInUrlbar_ = function() {
  var urlbarIcon = this.doc_.getElementById(this.urlbarIconId_);
  if (!urlbarIcon)
    return;
  urlbarIcon.setAttribute('level', 'warn');
}




PROT_PhishMsgDisplayerBase.prototype.removeWarningInUrlbar_ = function() {
  var urlbarIcon = this.doc_.getElementById(this.urlbarIconId_);
  if (!urlbarIcon)
    return;
  urlbarIcon.setAttribute('level', 'safe');
}




PROT_PhishMsgDisplayerBase.prototype.showMessage_ = function() { };




PROT_PhishMsgDisplayerBase.prototype.hideMessage_ = function() { };









PROT_PhishMsgDisplayerBase.prototype.adjustLocation_ = function(message,
                                                                tail,
                                                                refElement) {
  var refX = refElement.boxObject.x;
  var refY = refElement.boxObject.y;
  var refHeight = refElement.boxObject.height;
  var refWidth = refElement.boxObject.width;
  G_Debug(this, "Ref element is at [window-relative] (" + refX + ", " + 
          refY + ")");

  var pixelsIntoRefY = -2;
  var tailY = refY + refHeight - pixelsIntoRefY;
  var tailPixelsLeftOfRefX = tail.boxObject.width;
  var tailPixelsIntoRefX = Math.round(refWidth / 2);
  var tailX = refX - tailPixelsLeftOfRefX + tailPixelsIntoRefX;

  
  var messageY = tailY + tail.boxObject.height - 2;
  var messagePixelsLeftOfRefX = 375;
  var messageX = refX - messagePixelsLeftOfRefX;
  G_Debug(this, "Message is at [window-relative] (" + messageX + ", " + 
          messageY + ")");
  G_Debug(this, "Tail is at [window-relative] (" + tailX + ", " + 
          tailY + ")");

  if (messageX < 0) {
    
    tail.style.display = "none";
    this.adjustLocationFloating_(message);
    return;
  }

  tail.style.top = tailY + "px";
  tail.style.left = tailX + "px";
  message.style.top = messageY + "px";
  message.style.left = messageX + "px";
  
  this.maybeAddScrollbars_();
}






PROT_PhishMsgDisplayerBase.prototype.adjustLocationFloating_ = function(message) {
  
  var browserX = this.browser_.boxObject.x;
  var browserXCenter = browserX + this.browser_.boxObject.width / 2;
  var messageX = browserXCenter - (message.boxObject.width / 2);

  
  var messageY = this.browser_.boxObject.y;

  
  message.style.top = messageY + "px";
  message.style.left = messageX + "px";

  this.maybeAddScrollbars_();
}




PROT_PhishMsgDisplayerBase.prototype.maybeAddScrollbars_ = function() {
  var message = this.doc_.getElementById(this.messageId_);
  
  var content = this.doc_.getElementById(this.messageContentId_);
  var bottom = content.boxObject.y + content.boxObject.height;
  var maxY = this.doc_.defaultView.innerHeight;
  G_Debug(this, "bottom: " + bottom + ", maxY: " + maxY
                + ", new height: " + (maxY - content.boxObject.y));
  if (bottom > maxY) {
    var newHeight = maxY - content.boxObject.y;
    if (newHeight < 1)
      newHeight = 1;

    content.style.height = newHeight + "px";
    content.style.overflow = "auto";
  }
}




PROT_PhishMsgDisplayerBase.prototype.showMore_ = function() {
  this.doc_.getElementById(this.extendedMessageId_).hidden = false;
  this.doc_.getElementById(this.showmoreLinkId_).style.display = "none";

  
  var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                            .getService(Components.interfaces.nsIURLFormatter);
  var faqURL = formatter.formatURLPref("browser.safebrowsing.warning.infoURL");
  var labelEl = this.doc_.getElementById(this.faqLinkId_);
  labelEl.setAttribute("href", faqURL);
  
  this.maybeAddScrollbars_();
}







PROT_PhishMsgDisplayerBase.prototype.showURL_ = function(url) {
  this.windowWatcher_.openWindow(this.windowWatcher_.activeWindow,
                                 url,
                                 "_blank",
                                 null,
                                 null);
}






PROT_PhishMsgDisplayerBase.prototype.getReportErrorURL_ = function() {
  var badUrl = this.url_;

  var url = gDataProvider.getReportErrorURL();
  url += "&url=" + encodeURIComponent(badUrl);
  return url;
}





PROT_PhishMsgDisplayerBase.prototype.getReportGenericURL_ = function() {
  var badUrl = this.url_;

  var url = gDataProvider.getReportGenericURL();
  url += "&url=" + encodeURIComponent(badUrl);
  return url;
}

















function PROT_PhishMsgDisplayerCanvas(msgDesc, browser, doc, url) {
  PROT_PhishMsgDisplayerBase.call(this, msgDesc, browser, doc, url);

  this.dimAreaId_ = "safebrowsing-dim-area-canvas";
  this.pageCanvasId_ = "safebrowsing-page-canvas";
  this.xhtmlNS_ = "http://www.w3.org/1999/xhtml";     
}

PROT_PhishMsgDisplayerCanvas.inherits(PROT_PhishMsgDisplayerBase);





PROT_PhishMsgDisplayerCanvas.prototype.showMessage_ = function() {
  G_Debug(this, "Showing message.");

  
  var dimmer = this.doc_.getElementById('safebrowsing-dim-area-canvas');
  if (!dimmer) {
    var onOverlayMerged = BindToObject(this.showMessageAfterOverlay_,
                                       this);
    var observer = new G_ObserverWrapper("xul-overlay-merged",
                                         onOverlayMerged);

    this.doc_.loadOverlay(
        "chrome://browser/content/safebrowsing/warning-overlay.xul",
        observer);
  } else {
    
    
    this.showMessageAfterOverlay_();
  }
}




PROT_PhishMsgDisplayerCanvas.prototype.showMessageAfterOverlay_ = function() {
  this.messageShowing_ = true;

  
  
  
  
  
  
  
  
  

  
  var w = this.browser_.boxObject.width;
  G_Debug(this, "browser w=" + w);
  var h = this.browser_.boxObject.height;
  G_Debug(this, "browser h=" + h);
  var x = this.browser_.boxObject.x;
  G_Debug(this, "browser x=" + w);
  var y = this.browser_.boxObject.y;
  G_Debug(this, "browser y=" + h);

  var win = this.browser_.contentWindow;
  var scrollX = win.scrollX;
  G_Debug(this, "win scrollx=" + scrollX);
  var scrollY = win.scrollY;
  G_Debug(this, "win scrolly=" + scrollY);

  
  
  
  var pageCanvas = this.doc_.createElementNS(this.xhtmlNS_, "html:canvas");
  pageCanvas.id = this.pageCanvasId_;
  pageCanvas.style.left = x + 'px';
  pageCanvas.style.top = y + 'px';

  var dimarea = this.doc_.getElementById(this.dimAreaId_);
  this.doc_.getElementById('main-window').insertBefore(pageCanvas,
                                                       dimarea);

  
  dimarea.style.left = x + 'px';
  dimarea.style.top = y + 'px';
  dimarea.style.width = w + 'px';
  dimarea.style.height = h + 'px';
  dimarea.hidden = false;
  
  
  pageCanvas.setAttribute("width", w);
  pageCanvas.setAttribute("height", h);

  var bgcolor = this.getBackgroundColor_();

  var cx = pageCanvas.getContext("2d");
  cx.drawWindow(win, scrollX, scrollY, w, h, bgcolor);

  
  
  var debZone = this.debugZone;
  function repaint() {
    G_Debug(debZone, "Repainting canvas...");
    cx.drawWindow(win, scrollX, scrollY, w, h, bgcolor);
  };
  this.repainter_ = new PROT_PhishMsgCanvasRepainter(repaint);

  
  this.showAndPositionWarning_();

  
  var link = this.doc_.getElementById('safebrowsing-palm-falsepositive-link');
  link.href = this.getReportErrorURL_();

  
  this.doc_.getElementById(this.messageContentId_).focus();
}







PROT_PhishMsgDisplayerCanvas.prototype.showAndPositionWarning_ = function() {
  var refElement = this.doc_.getElementById(this.refElementId_);
  var message = this.doc_.getElementById(this.messageId_);
  var tail = this.doc_.getElementById(this.messageTailId_);

  message.hidden = false;
  message.style.display = "block";

  
  if (this.isVisibleElement_(refElement)) {
    
    tail.hidden = false;
    tail.style.display = "block";
    this.adjustLocation_(message, tail, refElement);
  } else {
    
    tail.hidden = true;
    tail.style.display = "none";
    this.adjustLocationFloating_(message);
  }
}




PROT_PhishMsgDisplayerCanvas.prototype.isVisibleElement_ = function(elt) {
  if (!elt)
    return false;
  
  
  if (elt.boxObject.x == 0)
    return false;

  return true;
}




PROT_PhishMsgDisplayerCanvas.prototype.hideMessage_ = function() {
  G_Debug(this, "Hiding phishing warning.");
  G_Assert(this, this.messageShowing_, "Hide message called but not showing?");

  this.messageShowing_ = false;
  this.repainter_.cancel();
  this.repainter_ = null;

  
  var message = this.doc_.getElementById(this.messageId_);
  message.hidden = true;
  message.style.display = "none";
  var content = this.doc_.getElementById(this.messageContentId_);
  content.style.height = "";
  content.style.overflow = "";

  var tail = this.doc_.getElementById(this.messageTailId_);
  tail.hidden = true;
  tail.style.display = "none";

  
  var pageCanvas = this.doc_.getElementById(this.pageCanvasId_);
  pageCanvas.parentNode.removeChild(pageCanvas);

  
  var dimarea = this.doc_.getElementById(this.dimAreaId_);
  dimarea.hidden = true;
}















function PROT_PhishMsgCanvasRepainter(repaintFunc) {
  this.count_ = 0;
  this.repaintFunc_ = repaintFunc;
  this.initPeriodMS_ = 500;             
  this.steadyStateAtMS_ = 10 * 1000;    
  this.steadyStatePeriodMS_ = 3 * 1000; 
  this.quitAtMS_ = 20 * 1000;           
  this.startMS_ = (new Date).getTime();
  this.alarm_ = new G_Alarm(BindToObject(this.repaint, this), 
                            this.initPeriodMS_);
}




PROT_PhishMsgCanvasRepainter.prototype.repaint = function() {
  this.repaintFunc_();

  var nextRepaint;
  
  if ((new Date).getTime() - this.startMS_ > this.steadyStateAtMS_)
    nextRepaint = this.steadyStatePeriodMS_;
  else 
    nextRepaint = this.initPeriodMS_;

  if (!((new Date).getTime() - this.startMS_ > this.quitAtMS_))
    this.alarm_ = new G_Alarm(BindToObject(this.repaint, this), nextRepaint);
}




PROT_PhishMsgCanvasRepainter.prototype.cancel = function() {
  if (this.alarm_) {
    this.alarm_.cancel();
    this.alarm_ = null;
  }
  this.repaintFunc_ = null;
}
