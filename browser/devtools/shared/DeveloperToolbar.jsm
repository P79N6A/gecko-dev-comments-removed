



"use strict";

this.EXPORTED_SYMBOLS = [ "DeveloperToolbar", "CommandUtils" ];

const NS_XHTML = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource:///modules/devtools/Commands.jsm");

const Node = Components.interfaces.nsIDOMNode;

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gcli",
                                  "resource:///modules/devtools/gcli.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CmdCommands",
                                  "resource:///modules/devtools/BuiltinCommands.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageErrorListener",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyGetter(this, "prefBranch", function() {
  let prefService = Components.classes["@mozilla.org/preferences-service;1"]
          .getService(Components.interfaces.nsIPrefService);
  return prefService.getBranch(null)
          .QueryInterface(Components.interfaces.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(this, "toolboxStrings", function () {
  return Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
});




this.CommandUtils = {
  



  getCommandbarSpec: function CU_getCommandbarSpec(aPref) {
    let value = prefBranch.getComplexValue(aPref,
                               Components.interfaces.nsISupportsString).data;
    return JSON.parse(value);
  },

  







  createButtons: function CU_createButtons(toolbarSpec, target, document, requisition) {
    let reply = [];

    toolbarSpec.forEach(function(buttonSpec) {
      let button = document.createElement("toolbarbutton");
      reply.push(button);

      if (typeof buttonSpec == "string") {
        buttonSpec = { typed: buttonSpec };
      }
      
      requisition.update(buttonSpec.typed);

      
      let command = requisition.commandAssignment.value;
      if (command == null) {
        
        
        button.setAttribute("label", "X");
        button.setAttribute("tooltip", "Unknown command: " + buttonSpec.typed);
        button.setAttribute("disabled", "true");
      }
      else {
        if (command.buttonId != null) {
          button.id = command.buttonId;
        }
        if (command.buttonClass != null) {
          button.className = command.buttonClass;
        }
        if (command.tooltipText != null) {
          button.setAttribute("tooltiptext", command.tooltipText);
        }
        else if (command.description != null) {
          button.setAttribute("tooltiptext", command.description);
        }

        button.addEventListener("click", function() {
          requisition.update(buttonSpec.typed);
          
            requisition.exec();
          





        }, false);

        
        if (command.state) {
          button.setAttribute("autocheck", false);
          let onChange = function(event, eventTab) {
            if (eventTab == target.tab) {
              if (command.state.isChecked(target)) {
                button.setAttribute("checked", true);
              }
              else if (button.hasAttribute("checked")) {
                button.removeAttribute("checked");
              }
            }
          };
          command.state.onChange(target, onChange);
          onChange(null, target.tab);
          document.defaultView.addEventListener("unload", function() {
            command.state.offChange(target, onChange);
          }, false);
        }
      }
    });

    requisition.update('');

    return reply;
  }
};








XPCOMUtils.defineLazyGetter(this, "isLinux", function () {
  let os = Components.classes["@mozilla.org/xre/app-info;1"]
           .getService(Components.interfaces.nsIXULRuntime).OS;
  return os == "Linux";
});







this.DeveloperToolbar = function DeveloperToolbar(aChromeWindow, aToolbarElement)
{
  this._chromeWindow = aChromeWindow;

  this._element = aToolbarElement;
  this._element.hidden = true;
  this._doc = this._element.ownerDocument;

  this._lastState = NOTIFICATIONS.HIDE;
  this._pendingShowCallback = undefined;
  this._pendingHide = false;
  this._errorsCount = {};
  this._warningsCount = {};
  this._errorListeners = {};
  this._errorCounterButton = this._doc
                             .getElementById("developer-toolbar-toolbox-button");
  this._errorCounterButton._defaultTooltipText =
    this._errorCounterButton.getAttribute("tooltiptext");

  try {
    CmdCommands.refreshAutoCommands(aChromeWindow);
  }
  catch (ex) {
    console.error(ex);
  }
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

let _gSequenceId = 0;




Object.defineProperty(DeveloperToolbar.prototype, 'sequenceId', {
  get: function DT_visible() {
    return _gSequenceId++;
  },
  enumerable: true
});





DeveloperToolbar.prototype.toggle = function DT_toggle()
{
  if (this.visible) {
    this.hide();
  } else {
    this.show(true);
  }
};





DeveloperToolbar.prototype.focus = function DT_focus()
{
  if (this.visible) {
    this._input.focus();
  } else {
    this.show(true);
  }
};





DeveloperToolbar.prototype.focusToggle = function DT_focusToggle()
{
  if (this.visible) {
    
    
    let active = this._chromeWindow.document.activeElement;
    let position = this._input.compareDocumentPosition(active);
    if (position & Node.DOCUMENT_POSITION_CONTAINED_BY) {
      this.hide();
    }
    else {
      this._input.focus();
    }
  } else {
    this.show(true);
  }
};







DeveloperToolbar.introShownThisSession = false;






DeveloperToolbar.prototype.show = function DT_show(aFocus, aCallback)
{
  if (this._lastState != NOTIFICATIONS.HIDE) {
    return;
  }

  Services.prefs.setBoolPref("devtools.toolbar.visible", true);

  this._notify(NOTIFICATIONS.LOAD);
  this._pendingShowCallback = aCallback;
  this._pendingHide = false;

  let checkLoad = function() {
    if (this.tooltipPanel && this.tooltipPanel.loaded &&
        this.outputPanel && this.outputPanel.loaded) {
      this._onload(aFocus);
    }
  }.bind(this);

  this._input = this._doc.querySelector(".gclitoolbar-input-node");
  this.tooltipPanel = new TooltipPanel(this._doc, this._input, checkLoad);
  this.outputPanel = new OutputPanel(this._doc, this._input, checkLoad);
};





DeveloperToolbar.prototype._onload = function DT_onload(aFocus)
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

  this.display.focusManager.addMonitoredElement(this.outputPanel._frame);
  this.display.focusManager.addMonitoredElement(this._element);

  this.display.onVisibilityChange.add(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.add(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.add(this.outputPanel._outputChanged, this.outputPanel);

  this._chromeWindow.getBrowser().tabContainer.addEventListener("TabSelect", this, false);
  this._chromeWindow.getBrowser().tabContainer.addEventListener("TabClose", this, false);
  this._chromeWindow.getBrowser().addEventListener("load", this, true);
  this._chromeWindow.getBrowser().addEventListener("beforeunload", this, true);

  this._initErrorsCount(this._chromeWindow.getBrowser().selectedTab);

  this._element.hidden = false;

  if (aFocus) {
    this._input.focus();
  }

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









DeveloperToolbar.prototype._initErrorsCount = function DT__initErrorsCount(aTab)
{
  let tabId = aTab.linkedPanel;
  if (tabId in this._errorsCount) {
    this._updateErrorsCount();
    return;
  }

  let window = aTab.linkedBrowser.contentWindow;
  let listener = new PageErrorListener(window, {
    onPageError: this._onPageError.bind(this, tabId),
  });
  listener.init();

  this._errorListeners[tabId] = listener;
  this._errorsCount[tabId] = 0;
  this._warningsCount[tabId] = 0;

  let messages = listener.getCachedMessages();
  messages.forEach(this._onPageError.bind(this, tabId));

  this._updateErrorsCount();
};









DeveloperToolbar.prototype._stopErrorsCount = function DT__stopErrorsCount(aTab)
{
  let tabId = aTab.linkedPanel;
  if (!(tabId in this._errorsCount) || !(tabId in this._warningsCount)) {
    this._updateErrorsCount();
    return;
  }

  this._errorListeners[tabId].destroy();
  delete this._errorListeners[tabId];
  delete this._errorsCount[tabId];
  delete this._warningsCount[tabId];

  this._updateErrorsCount();
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

  Services.prefs.setBoolPref("devtools.toolbar.visible", false);

  this._doc.getElementById("Tools:DevToolbar").setAttribute("checked", "false");
  this.destroy();

  this._notify(NOTIFICATIONS.HIDE);
};




DeveloperToolbar.prototype.destroy = function DT_destroy()
{
  if (this._lastState == NOTIFICATIONS.HIDE) {
    return;
  }

  this._chromeWindow.getBrowser().tabContainer.removeEventListener("TabSelect", this, false);
  this._chromeWindow.getBrowser().tabContainer.removeEventListener("TabClose", this, false);
  this._chromeWindow.getBrowser().removeEventListener("load", this, true); 
  this._chromeWindow.getBrowser().removeEventListener("beforeunload", this, true);

  let tabs = this._chromeWindow.getBrowser().tabs;
  Array.prototype.forEach.call(tabs, this._stopErrorsCount, this);

  this.display.focusManager.removeMonitoredElement(this.outputPanel._frame);
  this.display.focusManager.removeMonitoredElement(this._element);

  this.display.onVisibilityChange.remove(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.remove(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.remove(this.outputPanel._outputChanged, this.outputPanel);
  this.display.destroy();
  this.outputPanel.destroy();
  this.tooltipPanel.destroy();
  delete this._input;

  
  
  
  
  





  this._lastState = NOTIFICATIONS.HIDE;
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

      if (aEvent.type == "TabSelect") {
        this._initErrorsCount(aEvent.target);
      }
    }
  }
  else if (aEvent.type == "TabClose") {
    this._stopErrorsCount(aEvent.target);
  }
  else if (aEvent.type == "beforeunload") {
    this._onPageBeforeUnload(aEvent);
  }
};










DeveloperToolbar.prototype._onPageError =
function DT__onPageError(aTabId, aPageError)
{
  if (aPageError.category == "CSS Parser" ||
      aPageError.category == "CSS Loader") {
    return;
  }
  if ((aPageError.flags & aPageError.warningFlag) ||
      (aPageError.flags & aPageError.strictFlag)) {
    this._warningsCount[aTabId]++;
  } else {
    this._errorsCount[aTabId]++;
  }
  this._updateErrorsCount(aTabId);
};








DeveloperToolbar.prototype._onPageBeforeUnload =
function DT__onPageBeforeUnload(aEvent)
{
  let window = aEvent.target.defaultView;
  if (window.top !== window) {
    return;
  }

  let tabs = this._chromeWindow.getBrowser().tabs;
  Array.prototype.some.call(tabs, function(aTab) {
    if (aTab.linkedBrowser.contentWindow === window) {
      let tabId = aTab.linkedPanel;
      if (tabId in this._errorsCount || tabId in this._warningsCount) {
        this._errorsCount[tabId] = 0;
        this._warningsCount[tabId] = 0;
        this._updateErrorsCount(tabId);
      }
      return true;
    }
    return false;
  }, this);
};










DeveloperToolbar.prototype._updateErrorsCount =
function DT__updateErrorsCount(aChangedTabId)
{
  let tabId = this._chromeWindow.getBrowser().selectedTab.linkedPanel;
  if (aChangedTabId && tabId != aChangedTabId) {
    return;
  }

  let errors = this._errorsCount[tabId];
  let warnings = this._warningsCount[tabId];
  let btn = this._errorCounterButton;
  if (errors) {
    let errorsText = toolboxStrings
                     .GetStringFromName("toolboxToggleButton.errors");
    errorsText = PluralForm.get(errors, errorsText).replace("#1", errors);

    let warningsText = toolboxStrings
                       .GetStringFromName("toolboxToggleButton.warnings");
    warningsText = PluralForm.get(warnings, warningsText).replace("#1", warnings);

    let tooltiptext = toolboxStrings
                      .formatStringFromName("toolboxToggleButton.tooltip",
                                            [errorsText, warningsText], 2);

    btn.setAttribute("error-count", errors);
    btn.setAttribute("tooltiptext", tooltiptext);
  } else {
    btn.removeAttribute("error-count");
    btn.setAttribute("tooltiptext", btn._defaultTooltipText);
  }
};







DeveloperToolbar.prototype.resetErrorsCount =
function DT_resetErrorsCount(aTab)
{
  let tabId = aTab.linkedPanel;
  if (tabId in this._errorsCount || tabId in this._warningsCount) {
    this._errorsCount[tabId] = 0;
    this._warningsCount[tabId] = 0;
    this._updateErrorsCount(tabId);
  }
};



















function OutputPanel(aChromeDoc, aInput, aLoadCallback)
{
  this._input = aInput;
  this._toolbar = aChromeDoc.getElementById("developer-toolbar");

  this._loadCallback = aLoadCallback;

  











  
  
  this._panel = aChromeDoc.createElement(isLinux ? "tooltip" : "panel");

  this._panel.id = "gcli-output";
  this._panel.classList.add("gcli-panel");

  if (isLinux) {
    this.canHide = false;
    this._onpopuphiding = this._onpopuphiding.bind(this);
    this._panel.addEventListener("popuphiding", this._onpopuphiding, true);
  } else {
    this._panel.setAttribute("noautofocus", "true");
    this._panel.setAttribute("noautohide", "true");

    
    
    
    
    this._panel.setAttribute("height", "1px");
  }

  this._toolbar.parentElement.insertBefore(this._panel, this._toolbar);

  this._frame = aChromeDoc.createElementNS(NS_XHTML, "iframe");
  this._frame.id = "gcli-output-frame";
  this._frame.setAttribute("src", "chrome://browser/content/devtools/commandlineoutput.xhtml");
  this._frame.setAttribute("sandbox", "allow-same-origin");
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

  this.document = this._frame.contentDocument;

  this._div = this.document.getElementById("gcli-output-root");
  this._div.classList.add('gcli-row-out');
  this._div.setAttribute('aria-live', 'assertive');

  let styles = this._toolbar.ownerDocument.defaultView
                  .getComputedStyle(this._toolbar);
  this._div.setAttribute("dir", styles.direction);

  this.loaded = true;
  if (this._loadCallback) {
    this._loadCallback();
    delete this._loadCallback;
  }
};






Object.defineProperty(OutputPanel.prototype, 'scrollbarWidth', {
  get: function() {
    if (this.__scrollbarWidth) {
      return this.__scrollbarWidth;
    }

    let hbox = this.document.createElementNS(XUL_NS, "hbox");
    hbox.setAttribute("style", "height: 0%; overflow: hidden");

    let scrollbar = this.document.createElementNS(XUL_NS, "scrollbar");
    scrollbar.setAttribute("orient", "vertical");
    hbox.appendChild(scrollbar);

    this.document.documentElement.appendChild(hbox);
    this.__scrollbarWidth = scrollbar.clientWidth;
    this.document.documentElement.removeChild(hbox);

    return this.__scrollbarWidth;
  },
  enumerable: true
});




OutputPanel.prototype._onpopuphiding = function OP_onpopuphiding(aEvent)
{
  
  
  if (isLinux && !this.canHide) {
    aEvent.preventDefault();
  }
};




OutputPanel.prototype.show = function OP_show()
{
  if (isLinux) {
    this.canHide = false;
  }

  
  
  this._frame.style.minHeight = this._frame.style.maxHeight = 0;
  this._frame.style.minWidth = 0;

  this._panel.openPopup(this._input, "before_start", 0, 0, false, false, null);
  this._resize();

  this._input.focus();
};





OutputPanel.prototype._resize = function CLP_resize()
{
  if (this._panel == null || this.document == null || !this._panel.state == "closed") {
    return
  }

  
  
  let maxWidth = this._panel.ownerDocument.documentElement.clientWidth;
  let width = Math.min(maxWidth, this.document.documentElement.scrollWidth);

  
  width += this.scrollbarWidth;

  
  this._frame.style.minWidth = width + "px";

  
  
  const browserAdjustment = 15;

  
  
  let maxHeight =
    this._panel.ownerDocument.documentElement.clientHeight - browserAdjustment;
  let height = Math.min(maxHeight, this.document.documentElement.scrollHeight);

  
  this._frame.style.minHeight = this._frame.style.maxHeight = height + "px";

  
  this._panel.sizeTo(width, height);

  
  
  let screenX = this._input.boxObject.screenX;
  let screenY = this._toolbar.boxObject.screenY;
  this._panel.moveTo(screenX, screenY - height);
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
  if (isLinux) {
    this.canHide = true;
  }

  if (this._panel && this._panel.hidePopup) {
    this._panel.hidePopup();
  }

  if (this.displayedOutput) {
    this.displayedOutput.onChange.remove(this.update, this);
    this.displayedOutput.onClose.remove(this.remove, this);
    delete this.displayedOutput;
  }
};




OutputPanel.prototype.destroy = function OP_destroy()
{
  this.remove();

  this._panel.removeEventListener("popuphiding", this._onpopuphiding, true);

  this._panel.removeChild(this._frame);
  this._toolbar.parentElement.removeChild(this._panel);

  delete this._input;
  delete this._toolbar;
  delete this._onload;
  delete this._onpopuphiding;
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
    if (isLinux) {
      this.canHide = true;
    }
    this._panel.hidePopup();
  }
};




















function TooltipPanel(aChromeDoc, aInput, aLoadCallback)
{
  this._input = aInput;
  this._toolbar = aChromeDoc.getElementById("developer-toolbar");
  this._dimensions = { start: 0, end: 0 };

  this._onload = this._onload.bind(this);
  this._loadCallback = aLoadCallback;
  













  
  
  this._panel = aChromeDoc.createElement(isLinux ? "tooltip" : "panel");

  this._panel.id = "gcli-tooltip";
  this._panel.classList.add("gcli-panel");

  if (isLinux) {
    this.canHide = false;
    this._onpopuphiding = this._onpopuphiding.bind(this);
    this._panel.addEventListener("popuphiding", this._onpopuphiding, true);
  } else {
    this._panel.setAttribute("noautofocus", "true");
    this._panel.setAttribute("noautohide", "true");

    
    
    
    
    this._panel.setAttribute("height", "1px");
  }

  this._toolbar.parentElement.insertBefore(this._panel, this._toolbar);

  this._frame = aChromeDoc.createElementNS(NS_XHTML, "iframe");
  this._frame.id = "gcli-tooltip-frame";
  this._frame.setAttribute("src", "chrome://browser/content/devtools/commandlinetooltip.xhtml");
  this._frame.setAttribute("flex", "1");
  this._frame.setAttribute("sandbox", "allow-same-origin");
  this._panel.appendChild(this._frame);

  this._frame.addEventListener("load", this._onload, true);

  this.loaded = false;
}




TooltipPanel.prototype._onload = function TP_onload()
{
  this._frame.removeEventListener("load", this._onload, true);

  this.document = this._frame.contentDocument;
  this.hintElement = this.document.getElementById("gcli-tooltip-root");
  this._connector = this.document.getElementById("gcli-tooltip-connector");

  let styles = this._toolbar.ownerDocument.defaultView
                  .getComputedStyle(this._toolbar);
  this.hintElement.setAttribute("dir", styles.direction);

  this.loaded = true;

  if (this._loadCallback) {
    this._loadCallback();
    delete this._loadCallback;
  }
};




TooltipPanel.prototype._onpopuphiding = function TP_onpopuphiding(aEvent)
{
  
  
  if (isLinux && !this.canHide) {
    aEvent.preventDefault();
  }
};




TooltipPanel.prototype.show = function TP_show(aDimensions)
{
  if (!aDimensions) {
    aDimensions = { start: 0, end: 0 };
  }
  this._dimensions = aDimensions;

  
  
  
  this._panel.ownerDocument.defaultView.setTimeout(function() {
    this._resize();
  }.bind(this), 0);

  if (isLinux) {
    this.canHide = false;
  }

  this._resize();
  this._panel.openPopup(this._input, "before_start", aDimensions.start * 10, 0, false, false, null);
  this._input.focus();
};








const AVE_CHAR_WIDTH = 4.5;




TooltipPanel.prototype._resize = function TP_resize()
{
  if (this._panel == null || this.document == null || !this._panel.state == "closed") {
    return
  }

  let offset = 10 + Math.floor(this._dimensions.start * AVE_CHAR_WIDTH);
  this._panel.style.marginLeft = offset + "px";

  








  this._frame.height = this.document.body.scrollHeight;
};




TooltipPanel.prototype.remove = function TP_remove()
{
  if (isLinux) {
    this.canHide = true;
  }
  if (this._panel && this._panel.hidePopup) {
    this._panel.hidePopup();
  }
};




TooltipPanel.prototype.destroy = function TP_destroy()
{
  this.remove();

  this._panel.removeEventListener("popuphiding", this._onpopuphiding, true);

  this._panel.removeChild(this._frame);
  this._toolbar.parentElement.removeChild(this._panel);

  delete this._connector;
  delete this._dimensions;
  delete this._input;
  delete this._onload;
  delete this._onpopuphiding;
  delete this._panel;
  delete this._frame;
  delete this._toolbar;
  delete this._content;
  delete this.document;
  delete this.hintElement;
};





TooltipPanel.prototype._visibilityChanged = function TP_visibilityChanged(aEvent)
{
  if (aEvent.tooltipVisible === true) {
    this.show(aEvent.dimensions);
  } else {
    if (isLinux) {
      this.canHide = true;
    }
    this._panel.hidePopup();
  }
};
