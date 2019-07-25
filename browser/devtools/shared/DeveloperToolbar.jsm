





































"use strict";

const EXPORTED_SYMBOLS = [ "DeveloperToolbar" ];

const NS_XHTML = "http://www.w3.org/1999/xhtml";
const URI_GCLIBLANK = "chrome://browser/content/devtools/gcliblank.xhtml";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "gcli", function() {
  let obj = {};
  Components.utils.import("resource:///modules/devtools/gcli.jsm", obj);
  Components.utils.import("resource:///modules/devtools/GcliCommands.jsm", {});
  return obj.gcli;
});








function DeveloperToolbar(aChromeWindow, aToolbarElement)
{
  this._chromeWindow = aChromeWindow;

  this._element = aToolbarElement;
  this._element.hidden = true;
  this._doc = this._element.ownerDocument;

  this._lastState = NOTIFICATIONS.HIDE;
  this._pendingShowCallback = undefined;
  this._pendingHide = false;
}




const NOTIFICATIONS = {
  
  LOAD: "developer-toolbar-load",

  
  SHOW: "developer-toolbar-show",

  
  HIDE: "developer-toolbar-hide"
};





DeveloperToolbar.prototype.NOTIFICATIONS = NOTIFICATIONS;




Object.defineProperty(DeveloperToolbar.prototype, 'visible', {
  get: function DT_visible() {
    return !this._element.hidden;
  },
  enumerable: true
});





DeveloperToolbar.prototype.toggle = function DT_toggle()
{
  if (this.visible) {
    this.hide();
  } else {
    this.show();
    this._input.focus();
  }
};







DeveloperToolbar.introShownThisSession = false;






DeveloperToolbar.prototype.show = function DT_show(aCallback)
{
  if (this._lastState != NOTIFICATIONS.HIDE) {
    return;
  }

  this._notify(NOTIFICATIONS.LOAD);
  this._pendingShowCallback = aCallback;
  this._pendingHide = false;

  let checkLoad = function() {
    if (this.tooltipPanel && this.tooltipPanel.loaded &&
        this.outputPanel && this.outputPanel.loaded) {
      this._onload();
    }
  }.bind(this);

  this._input = this._doc.querySelector(".gclitoolbar-input-node");
  this.tooltipPanel = new TooltipPanel(this._doc, this._input, checkLoad);
  this.outputPanel = new OutputPanel(this._doc, this._input, checkLoad);
};





DeveloperToolbar.prototype._onload = function DT_onload()
{
  this._doc.getElementById("Tools:DevToolbar").setAttribute("checked", "true");

  let contentDocument = this._chromeWindow.getBrowser().contentDocument;

  this.display = gcli.createDisplay({
    contentDocument: contentDocument,
    chromeDocument: this._doc,
    chromeWindow: this._chromeWindow,

    hintElement: this.tooltipPanel.hintElement,
    inputElement: this._input,
    completeElement: this._doc.querySelector(".gclitoolbar-complete-node"),
    backgroundElement: this._doc.querySelector(".gclitoolbar-stack-node"),
    outputDocument: this.outputPanel.document,

    environment: {
      chromeDocument: this._doc,
      contentDocument: contentDocument
    },

    tooltipClass: 'gcliterm-tooltip',
    eval: null,
    scratchpad: null
  });

  this.display.onVisibilityChange.add(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.add(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.add(this.outputPanel._outputChanged, this.outputPanel);

  this._chromeWindow.getBrowser().tabContainer.addEventListener("TabSelect", this, false);
  this._chromeWindow.getBrowser().addEventListener("load", this, true); 

  this._element.hidden = false;

  this._notify(NOTIFICATIONS.SHOW);
  if (this._pendingShowCallback) {
    this._pendingShowCallback.call();
    this._pendingShowCallback = undefined;
  }

  
  
  
  if (this._pendingHide) {
    this.hide();
    return;
  }

  if (!DeveloperToolbar.introShownThisSession) {
    this.display.maybeShowIntro();
    DeveloperToolbar.introShownThisSession = true;
  }
};




DeveloperToolbar.prototype.hide = function DT_hide()
{
  if (this._lastState == NOTIFICATIONS.HIDE) {
    return;
  }

  if (this._lastState == NOTIFICATIONS.LOAD) {
    this._pendingHide = true;
    return;
  }

  this._element.hidden = true;

  this._doc.getElementById("Tools:DevToolbar").setAttribute("checked", "false");
  this.destroy();

  this._notify(NOTIFICATIONS.HIDE);
};




DeveloperToolbar.prototype.destroy = function DT_destroy()
{
  this._chromeWindow.getBrowser().tabContainer.removeEventListener("TabSelect", this, false);
  this._chromeWindow.getBrowser().removeEventListener("load", this, true); 

  this.display.onVisibilityChange.remove(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.remove(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.remove(this.outputPanel._outputChanged, this.outputPanel);
  this.display.destroy();
  this.outputPanel.destroy();
  this.tooltipPanel.destroy();
  delete this._input;

  
  
  
  
  




};





DeveloperToolbar.prototype._notify = function DT_notify(aTopic)
{
  this._lastState = aTopic;

  let data = { toolbar: this };
  data.wrappedJSObject = data;
  Services.obs.notifyObservers(data, aTopic, null);
};





DeveloperToolbar.prototype.handleEvent = function DT_handleEvent(aEvent)
{
  if (aEvent.type == "TabSelect" || aEvent.type == "load") {
    this._chromeWindow.HUDConsoleUI.refreshCommand();
    this._chromeWindow.DebuggerUI.refreshCommand();

    if (this.visible) {
      let contentDocument = this._chromeWindow.getBrowser().contentDocument;

      this.display.reattach({
        contentDocument: contentDocument,
        chromeWindow: this._chromeWindow,
        environment: {
          chromeDocument: this._doc,
          contentDocument: contentDocument
        },
      });
    }
  }
};







function getContentBox(aPanel)
{
  let container = aPanel.ownerDocument.getAnonymousElementByAttribute(
          aPanel, "anonid", "container");
  return container.querySelector(".panel-inner-arrowcontent");
}










function getVerticalSpacing(aNode, aRoot)
{
  let win = aNode.ownerDocument.defaultView;

  function pxToNum(styles, property) {
    return parseInt(styles.getPropertyValue(property).replace(/px$/, ''), 10);
  }

  let vertSpacing = 0;
  do {
    let styles = win.getComputedStyle(aNode);
    vertSpacing += pxToNum(styles, "padding-top");
    vertSpacing += pxToNum(styles, "padding-bottom");
    vertSpacing += pxToNum(styles, "margin-top");
    vertSpacing += pxToNum(styles, "margin-bottom");
    vertSpacing += pxToNum(styles, "border-top-width");
    vertSpacing += pxToNum(styles, "border-bottom-width");

    let prev = aNode.previousSibling;
    while (prev != null) {
      vertSpacing += prev.clientHeight;
      prev = prev.previousSibling;
    }

    let next = aNode.nextSibling;
    while (next != null) {
      vertSpacing += next.clientHeight;
      next = next.nextSibling;
    }

    aNode = aNode.parentNode;
  } while (aNode !== aRoot);

  return vertSpacing + 9;
}







function OutputPanel(aChromeDoc, aInput, aLoadCallback)
{
  this._input = aInput;
  this._anchor = aChromeDoc.getElementById("developer-toolbar");

  this._loadCallback = aLoadCallback;

  










  this._panel = aChromeDoc.createElement("panel");
  this._panel.id = "gcli-output";
  this._panel.classList.add("gcli-panel");
  this._panel.setAttribute("type", "arrow");
  this._panel.setAttribute("noautofocus", "true");
  this._panel.setAttribute("noautohide", "true");
  this._anchor.parentElement.insertBefore(this._panel, this._anchor);

  this._frame = aChromeDoc.createElement("iframe");
  this._frame.id = "gcli-output-frame";
  this._frame.setAttribute("src", URI_GCLIBLANK);
  this._frame.setAttribute("flex", "1");
  this._panel.appendChild(this._frame);

  this.displayedOutput = undefined;

  this._onload = this._onload.bind(this);
  this._frame.addEventListener("load", this._onload, true);

  this.loaded = false;
}




OutputPanel.prototype._onload = function OP_onload()
{
  this._frame.removeEventListener("load", this._onload, true);
  delete this._onload;

  this._content = getContentBox(this._panel);
  this._content.classList.add("gcli-panel-inner-arrowcontent");

  this.document = this._frame.contentDocument;
  this.document.body.classList.add("gclichrome-output");

  this._div = this.document.querySelector("div");
  this._div.classList.add('gcli-row-out');
  this._div.setAttribute('aria-live', 'assertive');

  this.loaded = true;
  if (this._loadCallback) {
    this._loadCallback();
    delete this._loadCallback;
  }
};




OutputPanel.prototype.show = function OP_show()
{
  this._panel.ownerDocument.defaultView.setTimeout(function() {
    this._resize();
  }.bind(this), 0);

  this._resize();
  this._panel.openPopup(this._anchor, "before_end", -300, 0, false, false, null);

  this._input.focus();
};





OutputPanel.prototype._resize = function CLP_resize()
{
  let vertSpacing = getVerticalSpacing(this._content, this._panel);
  let idealHeight = this.document.body.scrollHeight + vertSpacing;
  this._panel.sizeTo(400, Math.min(idealHeight, 500));
};




OutputPanel.prototype._outputChanged = function OP_outputChanged(aEvent)
{
  if (aEvent.output.hidden) {
    return;
  }

  this.remove();

  this.displayedOutput = aEvent.output;
  this.update();

  this.displayedOutput.onChange.add(this.update, this);
  this.displayedOutput.onClose.add(this.remove, this);
};





OutputPanel.prototype.update = function OP_update()
{
  if (this.displayedOutput.data == null) {
    while (this._div.hasChildNodes()) {
      this._div.removeChild(this._div.firstChild);
    }
  } else {
    this.displayedOutput.toDom(this._div);
    this.show();
  }
};




OutputPanel.prototype.remove = function OP_remove()
{
  this._panel.hidePopup();

  if (this.displayedOutput) {
    this.displayedOutput.onChange.remove(this.update, this);
    this.displayedOutput.onClose.remove(this.remove, this);
    delete this.displayedOutput;
  }
};




OutputPanel.prototype.destroy = function OP_destroy()
{
  this.remove();

  this._panel.removeChild(this._frame);
  this._anchor.parentElement.removeChild(this._panel);

  delete this._input;
  delete this._anchor;
  delete this._panel;
  delete this._frame;
  delete this._content;
  delete this._div;
  delete this.document;
};





OutputPanel.prototype._visibilityChanged = function OP_visibilityChanged(aEvent)
{
  if (aEvent.outputVisible === true) {
    
  } else {
    this._panel.hidePopup();
  }
};








function TooltipPanel(aChromeDoc, aInput, aLoadCallback)
{
  this._input = aInput;
  this._anchor = aChromeDoc.getElementById("developer-toolbar");

  this._onload = this._onload.bind(this);
  this._loadCallback = aLoadCallback;
  










  this._panel = aChromeDoc.createElement("panel");
  this._panel.id = "gcli-tooltip";
  this._panel.classList.add("gcli-panel");
  this._panel.setAttribute("type", "arrow");
  this._panel.setAttribute("noautofocus", "true");
  this._panel.setAttribute("noautohide", "true");
  this._anchor.parentElement.insertBefore(this._panel, this._anchor);

  this._frame = aChromeDoc.createElement("iframe");
  this._frame.id = "gcli-tooltip-frame";
  this._frame.setAttribute("src", URI_GCLIBLANK);
  this._frame.setAttribute("flex", "1");
  this._panel.appendChild(this._frame);

  this._frame.addEventListener("load", this._onload, true);
  this.loaded = false;
}




TooltipPanel.prototype._onload = function TP_onload()
{
  this._frame.removeEventListener("load", this._onload, true);

  this._content = getContentBox(this._panel);
  this._content.classList.add("gcli-panel-inner-arrowcontent");

  this.document = this._frame.contentDocument;
  this.document.body.classList.add("gclichrome-tooltip");

  this.hintElement = this.document.querySelector("div");

  this.loaded = true;

  if (this._loadCallback) {
    this._loadCallback();
    delete this._loadCallback;
  }
};




TooltipPanel.prototype.show = function TP_show()
{
  let vertSpacing = getVerticalSpacing(this._content, this._panel);
  let idealHeight = this.document.body.scrollHeight + vertSpacing;
  this._panel.sizeTo(350, Math.min(idealHeight, 500));
  this._panel.openPopup(this._anchor, "before_start", 0, 0, false, false, null);

  this._input.focus();
};




TooltipPanel.prototype.remove = function TP_remove()
{
  this._panel.hidePopup();
};




TooltipPanel.prototype.destroy = function TP_destroy()
{
  this.remove();

  this._panel.removeChild(this._frame);
  this._anchor.parentElement.removeChild(this._panel);

  delete this._input;
  delete this._onload;
  delete this._panel;
  delete this._frame;
  delete this._anchor;
  delete this._content;
  delete this.document;
  delete this.hintElement;
};





TooltipPanel.prototype._visibilityChanged = function TP_visibilityChanged(aEvent)
{
  if (aEvent.tooltipVisible === true) {
    this.show();
  } else {
    this._panel.hidePopup();
  }
};
