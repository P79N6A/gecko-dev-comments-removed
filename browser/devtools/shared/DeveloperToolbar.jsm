



"use strict";

this.EXPORTED_SYMBOLS = [ "DeveloperToolbar", "CommandUtils" ];

const NS_XHTML = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const { require, TargetFactory } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;

const Node = Ci.nsIDOMNode;

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
                                  "resource://gre/modules/devtools/event-emitter.js");

XPCOMUtils.defineLazyGetter(this, "prefBranch", function() {
  let prefService = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefService);
  return prefService.getBranch(null)
                    .QueryInterface(Ci.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(this, "toolboxStrings", function () {
  return Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
});

const Telemetry = require("devtools/shared/telemetry");


XPCOMUtils.defineLazyGetter(this, "gcli", () => {
  try {
    require("devtools/commandline/commands-index");
    return require("gcli/index");
  }
  catch (ex) {
    console.error(ex);
  }
});

XPCOMUtils.defineLazyGetter(this, "util", () => {
  return require("gcli/util/util");
});

Object.defineProperty(this, "ConsoleServiceListener", {
  get: function() {
    return require("devtools/toolkit/webconsole/utils").ConsoleServiceListener;
  },
  configurable: true,
  enumerable: true
});

const promise = Cu.import('resource://gre/modules/Promise.jsm', {}).Promise;




let CommandUtils = {
  


  createRequisition: function(environment) {
    return gcli.load().then(() => {
      return gcli.createRequisition({ environment: environment });
    });
  },

  



  getCommandbarSpec: function(pref) {
    let value = prefBranch.getComplexValue(pref, Ci.nsISupportsString).data;
    return JSON.parse(value);
  },

  






  createButtons: function(toolbarSpec, target, document, requisition) {
    return util.promiseEach(toolbarSpec, typed => {
      
      return requisition.update(typed).then(() => {
        let button = document.createElement("toolbarbutton");

        
        let command = requisition.commandAssignment.value;
        if (command == null) {
          throw new Error("No command '" + typed + "'");
        }

        
        if (!target.isLocalTab && !command.isRemoteSafe) {
          requisition.clear();
          return;
        }

        if (command.buttonId != null) {
          button.id = command.buttonId;
          if (command.buttonClass != null) {
            button.className = command.buttonClass;
          }
        }
        else {
          button.setAttribute("text-as-image", "true");
          button.setAttribute("label", command.name);
          button.className = "devtools-toolbarbutton";
        }
        if (command.tooltipText != null) {
          button.setAttribute("tooltiptext", command.tooltipText);
        }
        else if (command.description != null) {
          button.setAttribute("tooltiptext", command.description);
        }

        button.addEventListener("click", () => {
          requisition.updateExec(typed);
        }, false);

        
        if (command.state) {
          button.setAttribute("autocheck", false);

          





          let onChange = (eventName, ev) => {
            if (ev.target == target || ev.tab == target.tab) {

              let updateChecked = (checked) => {
                if (checked) {
                  button.setAttribute("checked", true);
                }
                else if (button.hasAttribute("checked")) {
                  button.removeAttribute("checked");
                }
              };

              
              
              
              
              
              let reply = command.state.isChecked(target);
              if (typeof reply.then == "function") {
                reply.then(updateChecked, console.error);
              }
              else {
                updateChecked(reply);
              }
            }
          };

          command.state.onChange(target, onChange);
          onChange("", { target: target });
          document.defaultView.addEventListener("unload", () => {
            if (command.state.offChange) {
              command.state.offChange(target, onChange);
            }
          }, false);
        }

        requisition.clear();

        return button;
      });
    });
  },

  





  createEnvironment: function(container, targetProperty='target') {
    if (!container[targetProperty].toString ||
        !/TabTarget/.test(container[targetProperty].toString())) {
      throw new Error('Missing target');
    }

    return {
      get target() {
        if (!container[targetProperty].toString ||
            !/TabTarget/.test(container[targetProperty].toString())) {
          throw new Error('Removed target');
        }

        return container[targetProperty];
      },

      get chromeWindow() {
        return this.target.tab.ownerDocument.defaultView;
      },

      get chromeDocument() {
        return this.chromeWindow.document;
      },

      get window() {
        return this.chromeWindow.gBrowser.selectedBrowser.contentWindow;
      },

      get document() {
        return this.window.document;
      }
    };
  },
};

this.CommandUtils = CommandUtils;








XPCOMUtils.defineLazyGetter(this, "isLinux", function() {
  return OS == "Linux";
});

XPCOMUtils.defineLazyGetter(this, "OS", function() {
  let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
  return os;
});







this.DeveloperToolbar = function DeveloperToolbar(aChromeWindow, aToolbarElement)
{
  this._chromeWindow = aChromeWindow;

  this._element = aToolbarElement;
  this._element.hidden = true;
  this._doc = this._element.ownerDocument;

  this._telemetry = new Telemetry();
  this._errorsCount = {};
  this._warningsCount = {};
  this._errorListeners = {};
  this._errorCounterButton = this._doc
                             .getElementById("developer-toolbar-toolbox-button");
  this._errorCounterButton._defaultTooltipText =
      this._errorCounterButton.getAttribute("tooltiptext");

  EventEmitter.decorate(this);
}




const NOTIFICATIONS = {
  
  LOAD: "developer-toolbar-load",

  
  SHOW: "developer-toolbar-show",

  
  HIDE: "developer-toolbar-hide"
};





DeveloperToolbar.prototype.NOTIFICATIONS = NOTIFICATIONS;




Object.defineProperty(DeveloperToolbar.prototype, "target", {
  get: function() {
    return TargetFactory.forTab(this._chromeWindow.gBrowser.selectedTab);
  },
  enumerable: true
});




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





DeveloperToolbar.prototype.toggle = function() {
  if (this.visible) {
    return this.hide().catch(console.error);
  } else {
    return this.show(true).catch(console.error);
  }
};





DeveloperToolbar.prototype.focus = function() {
  if (this.visible) {
    this._input.focus();
    return promise.resolve();
  } else {
    return this.show(true);
  }
};





DeveloperToolbar.prototype.focusToggle = function() {
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




DeveloperToolbar.prototype.show = function(focus) {
  if (this._showPromise != null) {
    return this._showPromise;
  }

  
  var waitPromise = this._hidePromise || promise.resolve();

  this._showPromise = waitPromise.then(() => {
    Services.prefs.setBoolPref("devtools.toolbar.visible", true);

    this._telemetry.toolOpened("developertoolbar");

    this._notify(NOTIFICATIONS.LOAD);

    this._input = this._doc.querySelector(".gclitoolbar-input-node");

    
    
    let panelPromises = [
      TooltipPanel.create(this),
      OutputPanel.create(this)
    ];
    return promise.all(panelPromises).then(panels => {
      [ this.tooltipPanel, this.outputPanel ] = panels;

      this._doc.getElementById("Tools:DevToolbar").setAttribute("checked", "true");

      return gcli.load().then(() => {
        this.display = gcli.createDisplay({
          contentDocument: this._chromeWindow.gBrowser.contentDocumentAsCPOW,
          chromeDocument: this._doc,
          chromeWindow: this._chromeWindow,
          hintElement: this.tooltipPanel.hintElement,
          inputElement: this._input,
          completeElement: this._doc.querySelector(".gclitoolbar-complete-node"),
          backgroundElement: this._doc.querySelector(".gclitoolbar-stack-node"),
          outputDocument: this.outputPanel.document,
          environment: CommandUtils.createEnvironment(this, "target"),
          tooltipClass: "gcliterm-tooltip",
          eval: null,
          scratchpad: null
        });

        this.display.focusManager.addMonitoredElement(this.outputPanel._frame);
        this.display.focusManager.addMonitoredElement(this._element);

        this.display.onVisibilityChange.add(this.outputPanel._visibilityChanged,
                                            this.outputPanel);
        this.display.onVisibilityChange.add(this.tooltipPanel._visibilityChanged,
                                            this.tooltipPanel);
        this.display.onOutput.add(this.outputPanel._outputChanged, this.outputPanel);

        let tabbrowser = this._chromeWindow.gBrowser;
        tabbrowser.tabContainer.addEventListener("TabSelect", this, false);
        tabbrowser.tabContainer.addEventListener("TabClose", this, false);
        tabbrowser.addEventListener("load", this, true);
        tabbrowser.addEventListener("beforeunload", this, true);

        this._initErrorsCount(tabbrowser.selectedTab);
        this._devtoolsUnloaded = this._devtoolsUnloaded.bind(this);
        this._devtoolsLoaded = this._devtoolsLoaded.bind(this);
        Services.obs.addObserver(this._devtoolsUnloaded, "devtools-unloaded", false);
        Services.obs.addObserver(this._devtoolsLoaded, "devtools-loaded", false);

        this._element.hidden = false;

        if (focus) {
          this._input.focus();
        }

        this._notify(NOTIFICATIONS.SHOW);

        if (!DeveloperToolbar.introShownThisSession) {
          this.display.maybeShowIntro();
          DeveloperToolbar.introShownThisSession = true;
        }

        this._showPromise = null;
      });
    });
  });

  return this._showPromise;
};




DeveloperToolbar.prototype.hide = function() {
  
  if (this._hidePromise != null) {
    return this._hidePromise;
  }

  
  var waitPromise = this._showPromise || promise.resolve();

  this._hidePromise = waitPromise.then(() => {
    this._element.hidden = true;

    Services.prefs.setBoolPref("devtools.toolbar.visible", false);

    this._doc.getElementById("Tools:DevToolbar").setAttribute("checked", "false");
    this.destroy();

    this._telemetry.toolClosed("developertoolbar");
    this._notify(NOTIFICATIONS.HIDE);

    this._hidePromise = null;
  });

  return this._hidePromise;
};





DeveloperToolbar.prototype._devtoolsUnloaded = function() {
  let tabbrowser = this._chromeWindow.gBrowser;
  Array.prototype.forEach.call(tabbrowser.tabs, this._stopErrorsCount, this);
};





DeveloperToolbar.prototype._devtoolsLoaded = function() {
  let tabbrowser = this._chromeWindow.gBrowser;
  this._initErrorsCount(tabbrowser.selectedTab);
};









DeveloperToolbar.prototype._initErrorsCount = function(tab) {
  let tabId = tab.linkedPanel;
  if (tabId in this._errorsCount) {
    this._updateErrorsCount();
    return;
  }

  let window = tab.linkedBrowser.contentWindow;
  let listener = new ConsoleServiceListener(window, {
    onConsoleServiceMessage: this._onPageError.bind(this, tabId),
  });
  listener.init();

  this._errorListeners[tabId] = listener;
  this._errorsCount[tabId] = 0;
  this._warningsCount[tabId] = 0;

  let messages = listener.getCachedMessages();
  messages.forEach(this._onPageError.bind(this, tabId));

  this._updateErrorsCount();
};









DeveloperToolbar.prototype._stopErrorsCount = function(tab) {
  let tabId = tab.linkedPanel;
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




DeveloperToolbar.prototype.destroy = function() {
  if (this._input == null) {
    return; 
  }

  let tabbrowser = this._chromeWindow.gBrowser;
  tabbrowser.tabContainer.removeEventListener("TabSelect", this, false);
  tabbrowser.tabContainer.removeEventListener("TabClose", this, false);
  tabbrowser.removeEventListener("load", this, true);
  tabbrowser.removeEventListener("beforeunload", this, true);

  Services.obs.removeObserver(this._devtoolsUnloaded, "devtools-unloaded");
  Services.obs.removeObserver(this._devtoolsLoaded, "devtools-loaded");
  Array.prototype.forEach.call(tabbrowser.tabs, this._stopErrorsCount, this);

  this.display.focusManager.removeMonitoredElement(this.outputPanel._frame);
  this.display.focusManager.removeMonitoredElement(this._element);

  this.display.onVisibilityChange.remove(this.outputPanel._visibilityChanged, this.outputPanel);
  this.display.onVisibilityChange.remove(this.tooltipPanel._visibilityChanged, this.tooltipPanel);
  this.display.onOutput.remove(this.outputPanel._outputChanged, this.outputPanel);
  this.display.destroy();
  this.outputPanel.destroy();
  this.tooltipPanel.destroy();
  delete this._input;

  
  
  
  
  




};





DeveloperToolbar.prototype._notify = function(topic) {
  let data = { toolbar: this };
  data.wrappedJSObject = data;
  Services.obs.notifyObservers(data, topic, null);
};




DeveloperToolbar.prototype.handleEvent = function(ev) {
  if (ev.type == "TabSelect" || ev.type == "load") {
    if (this.visible) {
      this.display.reattach({
        contentDocument: this._chromeWindow.gBrowser.contentDocumentAsCPOW
      });

      if (ev.type == "TabSelect") {
        this._initErrorsCount(ev.target);
      }
    }
  }
  else if (ev.type == "TabClose") {
    this._stopErrorsCount(ev.target);
  }
  else if (ev.type == "beforeunload") {
    this._onPageBeforeUnload(ev);
  }
};










DeveloperToolbar.prototype._onPageError = function(tabId, pageError) {
  if (pageError.category == "CSS Parser" ||
      pageError.category == "CSS Loader") {
    return;
  }
  if ((pageError.flags & pageError.warningFlag) ||
      (pageError.flags & pageError.strictFlag)) {
    this._warningsCount[tabId]++;
  } else {
    this._errorsCount[tabId]++;
  }
  this._updateErrorsCount(tabId);
};








DeveloperToolbar.prototype._onPageBeforeUnload = function(ev) {
  let window = ev.target.defaultView;
  if (window.top !== window) {
    return;
  }

  let tabs = this._chromeWindow.gBrowser.tabs;
  Array.prototype.some.call(tabs, function(tab) {
    if (tab.linkedBrowser.contentWindow === window) {
      let tabId = tab.linkedPanel;
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










DeveloperToolbar.prototype._updateErrorsCount = function(changedTabId) {
  let tabId = this._chromeWindow.gBrowser.selectedTab.linkedPanel;
  if (changedTabId && tabId != changedTabId) {
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

  this.emit("errors-counter-updated");
};







DeveloperToolbar.prototype.resetErrorsCount = function(tab) {
  let tabId = tab.linkedPanel;
  if (tabId in this._errorsCount || tabId in this._warningsCount) {
    this._errorsCount[tabId] = 0;
    this._warningsCount[tabId] = 0;
    this._updateErrorsCount(tabId);
  }
};




function OutputPanel() {
  throw new Error('Use OutputPanel.create()');
}

















OutputPanel.create = function(devtoolbar) {
  var outputPanel = Object.create(OutputPanel.prototype);
  return outputPanel._init(devtoolbar);
};




OutputPanel.prototype._init = function(devtoolbar) {
  this._devtoolbar = devtoolbar;
  this._input = this._devtoolbar._input;
  this._toolbar = this._devtoolbar._doc.getElementById("developer-toolbar");

  











  
  
  this._panel = this._devtoolbar._doc.createElement(isLinux ? "tooltip" : "panel");

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

  this._frame = this._devtoolbar._doc.createElementNS(NS_XHTML, "iframe");
  this._frame.id = "gcli-output-frame";
  this._frame.setAttribute("src", "chrome://browser/content/devtools/commandlineoutput.xhtml");
  this._frame.setAttribute("sandbox", "allow-same-origin");
  this._panel.appendChild(this._frame);

  this.displayedOutput = undefined;

  this._update = this._update.bind(this);

  
  let deferred = promise.defer();
  let onload = () => {
    this._frame.removeEventListener("load", onload, true);

    this.document = this._frame.contentDocument;
    this._copyTheme();

    this._div = this.document.getElementById("gcli-output-root");
    this._div.classList.add('gcli-row-out');
    this._div.setAttribute('aria-live', 'assertive');

    let styles = this._toolbar.ownerDocument.defaultView
                    .getComputedStyle(this._toolbar);
    this._div.setAttribute("dir", styles.direction);

    deferred.resolve(this);
  };
  this._frame.addEventListener("load", onload, true);

  return deferred.promise;
}



OutputPanel.prototype._copyTheme = function() {
  if (this.document) {
    let theme =
      this._devtoolbar._doc.documentElement.getAttribute("devtoolstheme");
    this.document.documentElement.setAttribute("devtoolstheme", theme);
  }
};




OutputPanel.prototype._onpopuphiding = function(ev) {
  
  
  if (isLinux && !this.canHide) {
    ev.preventDefault();
  }
};




OutputPanel.prototype.show = function() {
  if (isLinux) {
    this.canHide = false;
  }

  
  
  this._frame.style.minHeight = this._frame.style.maxHeight = 0;
  this._frame.style.minWidth = 0;

  this._copyTheme();
  this._panel.openPopup(this._input, "before_start", 0, 0, false, false, null);
  this._resize();

  this._input.focus();
};





OutputPanel.prototype._resize = function() {
  if (this._panel == null || this.document == null || !this._panel.state == "closed") {
    return
  }

  
  
  let maxWidth = this._panel.ownerDocument.documentElement.clientWidth;

  
  
  
  
  switch(OS) {
    case "Linux":
      maxWidth -= 5;
      break;
    case "Darwin":
      maxWidth -= 25;
      break;
    case "WINNT":
      maxWidth -= 5;
      break;
  }

  this.document.body.style.width = "-moz-max-content";
  let style = this._frame.contentWindow.getComputedStyle(this.document.body);
  let frameWidth = parseInt(style.width, 10);
  let width = Math.min(maxWidth, frameWidth);
  this.document.body.style.width = width + "px";

  
  this._frame.style.minWidth = width + "px";
  this._panel.style.maxWidth = maxWidth + "px";

  
  
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




OutputPanel.prototype._outputChanged = function(ev) {
  if (ev.output.hidden) {
    return;
  }

  this.remove();

  this.displayedOutput = ev.output;

  if (this.displayedOutput.completed) {
    this._update();
  }
  else {
    this.displayedOutput.promise.then(this._update, this._update)
                                .then(null, console.error);
  }
};





OutputPanel.prototype._update = function() {
  
  if (this._div == null) {
    return;
  }

  
  while (this._div.hasChildNodes()) {
    this._div.removeChild(this._div.firstChild);
  }

  if (this.displayedOutput.data != null) {
    let context = this._devtoolbar.display.requisition.conversionContext;
    this.displayedOutput.convert('dom', context).then(node => {
      if (node == null) {
        return;
      }

      while (this._div.hasChildNodes()) {
        this._div.removeChild(this._div.firstChild);
      }

      var links = node.querySelectorAll('*[href]');
      for (var i = 0; i < links.length; i++) {
        links[i].setAttribute('target', '_blank');
      }

      this._div.appendChild(node);
      this.show();
    });
  }
};




OutputPanel.prototype.remove = function() {
  if (isLinux) {
    this.canHide = true;
  }

  if (this._panel && this._panel.hidePopup) {
    this._panel.hidePopup();
  }

  if (this.displayedOutput) {
    delete this.displayedOutput;
  }
};




OutputPanel.prototype.destroy = function() {
  this.remove();

  this._panel.removeEventListener("popuphiding", this._onpopuphiding, true);

  this._panel.removeChild(this._frame);
  this._toolbar.parentElement.removeChild(this._panel);

  delete this._devtoolbar;
  delete this._input;
  delete this._toolbar;
  delete this._onpopuphiding;
  delete this._panel;
  delete this._frame;
  delete this._content;
  delete this._div;
  delete this.document;
};





OutputPanel.prototype._visibilityChanged = function(ev) {
  if (ev.outputVisible === true) {
    
  } else {
    if (isLinux) {
      this.canHide = true;
    }
    this._panel.hidePopup();
  }
};




function TooltipPanel() {
  throw new Error('Use TooltipPanel.create()');
}

















TooltipPanel.create = function(devtoolbar) {
  var tooltipPanel = Object.create(TooltipPanel.prototype);
  return tooltipPanel._init(devtoolbar);
};




TooltipPanel.prototype._init = function(devtoolbar) {
  let deferred = promise.defer();

  let chromeDocument = devtoolbar._doc;
  this._devtoolbar = devtoolbar;
  this._input = devtoolbar._doc.querySelector(".gclitoolbar-input-node");
  this._toolbar = devtoolbar._doc.querySelector("#developer-toolbar");
  this._dimensions = { start: 0, end: 0 };

  













  
  
  this._panel = devtoolbar._doc.createElement(isLinux ? "tooltip" : "panel");

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

  this._frame = devtoolbar._doc.createElementNS(NS_XHTML, "iframe");
  this._frame.id = "gcli-tooltip-frame";
  this._frame.setAttribute("src", "chrome://browser/content/devtools/commandlinetooltip.xhtml");
  this._frame.setAttribute("flex", "1");
  this._frame.setAttribute("sandbox", "allow-same-origin");
  this._panel.appendChild(this._frame);

  


  let onload = () => {
    this._frame.removeEventListener("load", onload, true);

    this.document = this._frame.contentDocument;
    this._copyTheme();
    this.hintElement = this.document.getElementById("gcli-tooltip-root");
    this._connector = this.document.getElementById("gcli-tooltip-connector");

    let styles = this._toolbar.ownerDocument.defaultView
                    .getComputedStyle(this._toolbar);
    this.hintElement.setAttribute("dir", styles.direction);

    deferred.resolve(this);
  };
  this._frame.addEventListener("load", onload, true);

  return deferred.promise;
};



TooltipPanel.prototype._copyTheme = function() {
  if (this.document) {
    let theme =
      this._devtoolbar._doc.documentElement.getAttribute("devtoolstheme");
    this.document.documentElement.setAttribute("devtoolstheme", theme);
  }
};




TooltipPanel.prototype._onpopuphiding = function(ev) {
  
  
  if (isLinux && !this.canHide) {
    ev.preventDefault();
  }
};




TooltipPanel.prototype.show = function(dimensions) {
  if (!dimensions) {
    dimensions = { start: 0, end: 0 };
  }
  this._dimensions = dimensions;

  
  
  
  this._panel.ownerDocument.defaultView.setTimeout(() => {
    this._resize();
  }, 0);

  if (isLinux) {
    this.canHide = false;
  }

  this._copyTheme();
  this._resize();
  this._panel.openPopup(this._input, "before_start", dimensions.start * 10, 0,
                        false, false, null);
  this._input.focus();
};








const AVE_CHAR_WIDTH = 4.5;




TooltipPanel.prototype._resize = function() {
  if (this._panel == null || this.document == null || !this._panel.state == "closed") {
    return
  }

  let offset = 10 + Math.floor(this._dimensions.start * AVE_CHAR_WIDTH);
  this._panel.style.marginLeft = offset + "px";

  








  this._frame.height = this.document.body.scrollHeight;
};




TooltipPanel.prototype.remove = function() {
  if (isLinux) {
    this.canHide = true;
  }
  if (this._panel && this._panel.hidePopup) {
    this._panel.hidePopup();
  }
};




TooltipPanel.prototype.destroy = function() {
  this.remove();

  this._panel.removeEventListener("popuphiding", this._onpopuphiding, true);

  this._panel.removeChild(this._frame);
  this._toolbar.parentElement.removeChild(this._panel);

  delete this._connector;
  delete this._dimensions;
  delete this._input;
  delete this._onpopuphiding;
  delete this._panel;
  delete this._frame;
  delete this._toolbar;
  delete this._content;
  delete this.document;
  delete this.hintElement;
};





TooltipPanel.prototype._visibilityChanged = function(ev) {
  if (ev.tooltipVisible === true) {
    this.show(ev.dimensions);
  } else {
    if (isLinux) {
      this.canHide = true;
    }
    this._panel.hidePopup();
  }
};
