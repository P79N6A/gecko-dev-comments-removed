





































"use strict";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

let EXPORTED_SYMBOLS = [ "DeveloperToolbar", "loadCommands" ];

XPCOMUtils.defineLazyGetter(this, "gcli", function () {
  let obj = {};
  Components.utils.import("resource:///modules/gcli.jsm", obj);
  return obj.gcli;
});

let console = gcli._internal.console;





function loadCommands()
{
  Components.utils.import("resource:///modules/GcliCommands.jsm", {});
  Components.utils.import("resource:///modules/GcliTiltCommands.jsm", {});
}



let commandsLoaded = false;







function DeveloperToolbar(aChromeWindow, aToolbarElement)
{
  if (!commandsLoaded) {
    loadCommands();
    commandsLoaded = true;
  }

  this._chromeWindow = aChromeWindow;

  this._element = aToolbarElement;
  this._element.hidden = true;
  this._doc = this._element.ownerDocument;

  this._command = this._doc.getElementById("Tools:DevToolbar");

  aChromeWindow.getBrowser().tabContainer.addEventListener("TabSelect", this, false);
}




const NOTIFICATIONS = {
  
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




DeveloperToolbar.prototype.show = function DT_show()
{
  this._command.setAttribute("checked", "true");

  this._input = this._doc.querySelector(".gclitoolbar-input-node");

  this.tooltipPanel = new TooltipPanel(this._doc, this._input);
  this.outputPanel = new OutputPanel(this._doc, this._input);

  let contentDocument = this._chromeWindow.getBrowser().contentDocument;

  this.display = gcli._internal.createDisplay({
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

  this._element.hidden = false;
  this._notify(NOTIFICATIONS.SHOW);

  if (!DeveloperToolbar.introShownThisSession) {
    this.display.maybeShowIntro();
    DeveloperToolbar.introShownThisSession = true;
  }
};




DeveloperToolbar.prototype.hide = function DT_hide()
{
  this._command.setAttribute("checked", "false");

  this.display.onVisibilityChange.remove(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.remove(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.remove(this.outputPanel._outputChanged, this.outputPanel);
  this.display.destroy();

  
  
  
  

  this.outputPanel.remove();
  delete this.outputPanel;

  this.tooltipPanel.remove();
  delete this.tooltipPanel;

  this._element.hidden = true;
  this._notify(NOTIFICATIONS.HIDE);
};





DeveloperToolbar.prototype._notify = function DT_notify(aTopic)
{
  let data = { toolbar: this };
  data.wrappedJSObject = data;
  Services.obs.notifyObservers(data, aTopic, null);
};





DeveloperToolbar.prototype.handleEvent = function DT_handleEvent(aEvent)
{
  if (aEvent.type == "TabSelect") {
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






function OutputPanel(aChromeDoc, aInput)
{
  this._input = aInput;
  this._panel = aChromeDoc.getElementById("gcli-output");
  this._frame = aChromeDoc.getElementById("gcli-output-frame");
  this._anchor = aChromeDoc.getElementById("developer-toolbar");

  this._content = getContentBox(this._panel);
  this._content.classList.add("gcli-panel-inner-arrowcontent");

  this.document = this._frame.contentDocument;
  this.document.body.classList.add("gclichrome-output");

  this._div = this.document.querySelector("div");
  this._div.classList.add('gcli-row-out');
  this._div.setAttribute('aria-live', 'assertive');

  this.displayedOutput = undefined;
}




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





OutputPanel.prototype._visibilityChanged = function OP_visibilityChanged(aEvent)
{
  if (aEvent.outputVisible === true) {
    
  } else {
    this._panel.hidePopup();
  }
};







function TooltipPanel(aChromeDoc, aInput)
{
  this._input = aInput;
  this._panel = aChromeDoc.getElementById("gcli-tooltip");
  this._frame = aChromeDoc.getElementById("gcli-tooltip-frame");
  this._anchor = aChromeDoc.getElementById("developer-toolbar");

  this._content = getContentBox(this._panel);
  this._content.classList.add("gcli-panel-inner-arrowcontent");

  this.document = this._frame.contentDocument;
  this.document.body.classList.add("gclichrome-tooltip");

  this.hintElement = this.document.querySelector("div");
}




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





TooltipPanel.prototype._visibilityChanged = function TP_visibilityChanged(aEvent)
{
  if (aEvent.tooltipVisible === true) {
    this.show();
  } else {
    this._panel.hidePopup();
  }
};
