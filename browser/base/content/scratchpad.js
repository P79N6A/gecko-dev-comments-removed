

















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/PropertyPanel.jsm");

const SCRATCHPAD_CONTEXT_CONTENT = 1;
const SCRATCHPAD_CONTEXT_CHROME = 2;
const SCRATCHPAD_WINDOW_URL = "chrome://browser/content/scratchpad.xul";
const SCRATCHPAD_L10N = "chrome://browser/locale/scratchpad.properties";
const SCRATCHPAD_WINDOW_FEATURES = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";
const DEVTOOLS_CHROME_ENABLED = "devtools.chrome.enabled";




var Scratchpad = {
  









  executionContext: SCRATCHPAD_CONTEXT_CONTENT,

  



  get textbox() document.getElementById("scratchpad-textbox"),

  



  get statusbarStatus() document.getElementById("scratchpad-status"),

  


  get selectedText()
  {
    return this.textbox.value.substring(this.textbox.selectionStart,
                                        this.textbox.selectionEnd);
  },

  


  get browserWindow() Services.wm.getMostRecentWindow("navigator:browser"),

  



  _previousWindow: null,

  


  get gBrowser()
  {
    let recentWin = this.browserWindow;
    return recentWin ? recentWin.gBrowser : null;
  },

  


  _contentSandbox: null,

  





  get contentSandbox()
  {
    if (!this.browserWindow) {
      Cu.reportError(this.strings.
                     GetStringFromName("browserWindow.unavailable"));
      return;
    }

    if (!this._contentSandbox ||
        this.browserWindow != this._previousBrowserWindow) {
      let contentWindow = this.gBrowser.selectedBrowser.contentWindow;
      this._contentSandbox = new Cu.Sandbox(contentWindow,
        { sandboxPrototype: contentWindow, wantXrays: false });

      this._previousBrowserWindow = this.browserWindow;
    }

    return this._contentSandbox;
  },

  



  _chromeSandbox: null,

  





  get chromeSandbox()
  {
    if (!this.browserWindow) {
      Cu.reportError(this.strings.
                     GetStringFromName("browserWindow.unavailable"));
      return;
    }

    if (!this._chromeSandbox ||
        this.browserWindow != this._previousBrowserWindow) {
      this._chromeSandbox = new Cu.Sandbox(this.browserWindow,
        { sandboxPrototype: this.browserWindow, wantXrays: false });

      this._previousBrowserWindow = this.browserWindow;
    }

    return this._chromeSandbox;
  },

  


  deselect: function SP_deselect()
  {
    this.textbox.selectionEnd = this.textbox.selectionStart;
  },

  







  selectRange: function SP_selectRange(aStart, aEnd)
  {
    this.textbox.selectionStart = aStart;
    this.textbox.selectionEnd = aEnd;
  },

  







  evalInContentSandbox: function SP_evalInContentSandbox(aString)
  {
    let result;
    try {
      result = Cu.evalInSandbox(aString, this.contentSandbox, "1.8",
                                "Scratchpad", 1);
    }
    catch (ex) {
      this.openWebConsole();

      let contentWindow = this.gBrowser.selectedBrowser.contentWindow;

      let scriptError = Cc["@mozilla.org/scripterror;1"].
                        createInstance(Ci.nsIScriptError2);

      scriptError.initWithWindowID(ex.message + "\n" + ex.stack, ex.fileName,
                                   "", ex.lineNumber, 0, scriptError.errorFlag,
                                   "content javascript",
                                   this.getWindowId(contentWindow));

      Services.console.logMessage(scriptError);
    }

    return result;
  },

  







  evalInChromeSandbox: function SP_evalInChromeSandbox(aString)
  {
    let result;
    try {
      result = Cu.evalInSandbox(aString, this.chromeSandbox, "1.8",
                                "Scratchpad", 1);
    }
    catch (ex) {
      Cu.reportError(ex);
      Cu.reportError(ex.stack);
      this.openErrorConsole();
    }

    return result;
  },

  








  evalForContext: function SP_evaluateForContext(aString)
  {
    return this.executionContext == SCRATCHPAD_CONTEXT_CONTENT ?
           this.evalInContentSandbox(aString) :
           this.evalInChromeSandbox(aString);
  },

  



  execute: function SP_execute()
  {
    let selection = this.selectedText || this.textbox.value;
    let result = this.evalForContext(selection);
    this.deselect();
    return [selection, result];
  },

  




  inspect: function SP_inspect()
  {
    let [selection, result] = this.execute();

    if (result) {
      this.openPropertyPanel(selection, result);
    }
  },

  





  print: function SP_print()
  {
    let selectionStart = this.textbox.selectionStart;
    let selectionEnd = this.textbox.selectionEnd;
    if (selectionStart == selectionEnd) {
      selectionEnd = this.textbox.value.length;
    }

    let [selection, result] = this.execute();
    if (!result) {
      return;
    }

    let firstPiece = this.textbox.value.slice(0, selectionEnd);
    let lastPiece = this.textbox.value.
                    slice(selectionEnd, this.textbox.value.length);

    let newComment = "/*\n" + result.toString() + "\n*/";

    this.textbox.value = firstPiece + newComment + lastPiece;

    
    this.selectRange(firstPiece.length, firstPiece.length + newComment.length);
  },

  










  openPropertyPanel: function SP_openPropertyPanel(aEvalString, aOutputObject)
  {
    let self = this;
    let propPanel;
    
    
    
    let buttons = [];

    
    
    
    if (aEvalString !== null) {
      buttons.push({
        label: this.strings.
               GetStringFromName("propertyPanel.updateButton.label"),
        accesskey: this.strings.
                   GetStringFromName("propertyPanel.updateButton.accesskey"),
        oncommand: function () {
          try {
            let result = self.evalForContext(aEvalString);

            if (result !== undefined) {
              propPanel.treeView.data = result;
            }
          }
          catch (ex) { }
        }
      });
    }

    let doc = this.browserWindow.document;
    let parent = doc.getElementById("mainPopupSet");
    let title = aOutputObject.toString();
    propPanel = new PropertyPanel(parent, doc, title, aOutputObject, buttons);

    let panel = propPanel.panel;
    panel.setAttribute("class", "scratchpad_propertyPanel");
    panel.openPopup(null, "after_pointer", 0, 0, false, false);
    panel.sizeTo(200, 400);

    return propPanel;
  },

  

  


  openScratchpad: function SP_openScratchpad()
  {
    Services.ww.openWindow(null, SCRATCHPAD_WINDOW_URL, "_blank",
                           SCRATCHPAD_WINDOW_FEATURES, null);
  },

  














  exportToFile: function SP_exportToFile(aFile, aNoConfirmation, aSilentError,
                                         aCallback)
  {
    if (!aNoConfirmation && aFile.exists() &&
        !window.confirm(this.strings.
                        GetStringFromName("export.fileOverwriteConfirmation"))) {
      return;
    }

    let fs = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
    let modeFlags = 0x02 | 0x08 | 0x20;
    fs.init(aFile, modeFlags, 0644, fs.DEFER_OPEN);

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let input = converter.convertToInputStream(this.textbox.value);

    let self = this;
    NetUtil.asyncCopy(input, fs, function(aStatus) {
      if (!aSilentError && !Components.isSuccessCode(aStatus)) {
        window.alert(self.strings.GetStringFromName("saveFile.failed"));
      }

      if (aCallback) {
        aCallback.call(self, aStatus);
      }
    });
  },

  













  importFromFile: function SP_importFromFile(aFile, aSilentError, aCallback)
  {
    
    let channel = NetUtil.newChannel(aFile);
    channel.contentType = "application/javascript";

    let self = this;
    NetUtil.asyncFetch(channel, function(aInputStream, aStatus) {
      let content = null;

      if (Components.isSuccessCode(aStatus)) {
        content = NetUtil.readInputStreamToString(aInputStream,
                                                  aInputStream.available());
        self.textbox.value = content;
      }
      else if (!aSilentError) {
        window.alert(self.strings.GetStringFromName("openFile.failed"));
      }

      if (aCallback) {
        aCallback.call(self, aStatus, content);
      }
    });
  },

  


  openFile: function SP_openFile()
  {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, this.strings.GetStringFromName("openFile.title"),
            Ci.nsIFilePicker.modeOpen);
    fp.defaultString = "";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      document.title = this.filename = fp.file.path;
      this.importFromFile(fp.file);
    }
  },

  


  saveFile: function SP_saveFile()
  {
    if (!this.filename) {
      return this.saveFileAs();
    }

    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.filename);
    this.exportToFile(file, true);
  },

  


  saveFileAs: function SP_saveFileAs()
  {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, this.strings.GetStringFromName("saveFileAs"),
            Ci.nsIFilePicker.modeSave);
    fp.defaultString = "scratchpad.js";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      document.title = this.filename = fp.file.path;
      this.exportToFile(fp.file);
    }
  },

  


  openErrorConsole: function SP_openErrorConsole()
  {
    this.browserWindow.toJavaScriptConsole();
  },

  


  openWebConsole: function SP_openWebConsole()
  {
    if (!this.browserWindow.HUDConsoleUI.getOpenHUD()) {
      this.browserWindow.HUDConsoleUI.toggleHUD();
    }
    this.browserWindow.focus();
  },

  


  setContentContext: function SP_setContentContext()
  {
    let content = document.getElementById("sp-menu-content");
    document.getElementById("sp-menu-chrome").removeAttribute("checked");
    content.setAttribute("checked", true);
    this.statusbarStatus.label = content.getAttribute("label");
    this.executionContext = SCRATCHPAD_CONTEXT_CONTENT;
    this.resetContext();
  },

  


  setChromeContext: function SP_setChromeContext()
  {
    let chrome = document.getElementById("sp-menu-chrome");
    document.getElementById("sp-menu-content").removeAttribute("checked");
    chrome.setAttribute("checked", true);
    this.statusbarStatus.label = chrome.getAttribute("label");
    this.executionContext = SCRATCHPAD_CONTEXT_CHROME;
    this.resetContext();
  },

  


  resetContext: function SP_resetContext()
  {
    this._chromeSandbox = null;
    this._contentSandbox = null;
    this._previousWindow = null;
  },

  






  getWindowId: function SP_getWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },

  


  onLoad: function SP_onLoad()
  {
    let chromeContextMenu = document.getElementById("sp-menu-chrome");
    let errorConsoleMenu = document.getElementById("sp-menu-errorConsole");
    let errorConsoleCommand = document.getElementById("sp-cmd-errorConsole");
    let chromeContextCommand = document.getElementById("sp-cmd-chromeContext");

    let chrome = Services.prefs.getBoolPref(DEVTOOLS_CHROME_ENABLED);
    if (chrome) {
      chromeContextMenu.removeAttribute("hidden");
      errorConsoleMenu.removeAttribute("hidden");
      errorConsoleCommand.removeAttribute("disabled");
      chromeContextCommand.removeAttribute("disabled");
    }
  },
};

XPCOMUtils.defineLazyGetter(Scratchpad, "strings", function () {
  return Services.strings.createBundle(SCRATCHPAD_L10N);
});

addEventListener("DOMContentLoaded", Scratchpad.onLoad.bind(Scratchpad), false);

