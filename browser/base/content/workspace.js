








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/PropertyPanel.jsm");

const WORKSPACE_CONTEXT_CONTENT = 1;
const WORKSPACE_CONTEXT_CHROME = 2;
const WORKSPACE_WINDOW_URL = "chrome://browser/content/workspace.xul";
const WORKSPACE_L10N = "chrome://browser/locale/workspace.properties";
const WORKSPACE_WINDOW_FEATURES = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";




var Workspace = {
  









  executionContext: WORKSPACE_CONTEXT_CONTENT,

  



  get textbox() document.getElementById("workspace-textbox"),

  



  get statusbarStatus() document.getElementById("workspace-status"),

  


  get selectedText()
  {
    return this.textbox.value.substring(this.textbox.selectionStart,
                                        this.textbox.selectionEnd);
  },

  


  get browserWindow() Services.wm.getMostRecentWindow("navigator:browser"),

  


  get gBrowser()
  {
    let recentWin = this.browserWindow;
    return recentWin ? recentWin.gBrowser : null;
  },

  


  get contentSandbox()
  {
    if (!this.browserWindow) {
      Cu.reportError(this.strings.
                     GetStringFromName("browserWindow.unavailable"));
      return;
    }

    
    
    let contentWindow = this.gBrowser.selectedBrowser.contentWindow;
    return new Cu.Sandbox(contentWindow,
                          { sandboxPrototype: contentWindow,
                            wantXrays: false });
  },

  



  get chromeSandbox()
  {
    if (!this.browserWindow) {
      Cu.reportError(this.strings.
                     GetStringFromName("browserWindow.unavailable"));
      return;
    }

    return new Cu.Sandbox(this.browserWindow,
                          { sandboxPrototype: this.browserWindow,
                            wantXrays: false });
  },

  


  deselect: function WS_deselect()
  {
    this.textbox.selectionEnd = this.textbox.selectionStart;
  },

  







  selectRange: function WS_selectRange(aStart, aEnd)
  {
    this.textbox.selectionStart = aStart;
    this.textbox.selectionEnd = aEnd;
  },

  







  evalInContentSandbox: function WS_evalInContentSandbox(aString)
  {
    let result;
    try {
      result = Cu.evalInSandbox(aString, this.contentSandbox, "1.8",
                                "Workspace", 1);
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

  







  evalInChromeSandbox: function WS_evalInChromeSandbox(aString)
  {
    let result;
    try {
      result = Cu.evalInSandbox(aString, this.chromeSandbox, "1.8",
                                "Workspace", 1);
    }
    catch (ex) {
      Cu.reportError(ex);
      Cu.reportError(ex.stack);
      this.openErrorConsole();
    }

    return result;
  },

  








  evalForContext: function WS_evaluateForContext(aString)
  {
    return this.executionContext == WORKSPACE_CONTEXT_CONTENT ?
           this.evalInContentSandbox(aString) :
           this.evalInChromeSandbox(aString);
  },

  



  execute: function WS_execute()
  {
    let selection = this.selectedText || this.textbox.value;
    let result = this.evalForContext(selection);
    this.deselect();
    return [selection, result];
  },

  




  inspect: function WS_inspect()
  {
    let [selection, result] = this.execute();

    if (result) {
      this.openPropertyPanel(selection, result);
    }
  },

  





  print: function WS_print()
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

  










  openPropertyPanel: function WS_openPropertyPanel(aEvalString, aOutputObject)
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
    panel.setAttribute("class", "workspace_propertyPanel");
    panel.openPopup(null, "after_pointer", 0, 0, false, false);
    panel.sizeTo(200, 400);

    return propPanel;
  },

  

  


  openWorkspace: function WS_openWorkspace()
  {
    Services.ww.openWindow(null, WORKSPACE_WINDOW_URL, "_blank",
                           WORKSPACE_WINDOW_FEATURES, null);
  },

  














  exportToFile: function WS_exportToFile(aFile, aNoConfirmation, aSilentError,
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

  













  importFromFile: function WS_importFromFile(aFile, aSilentError, aCallback)
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

  


  openFile: function WS_openFile()
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

  


  saveFile: function WS_saveFile()
  {
    if (!this.filename) {
      return this.saveFileAs();
    }

    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.filename);
    this.exportToFile(file, true);
  },

  


  saveFileAs: function WS_saveFileAs()
  {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, this.strings.GetStringFromName("saveFileAs"),
            Ci.nsIFilePicker.modeSave);
    fp.defaultString = "workspace.js";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      document.title = this.filename = fp.file.path;
      this.exportToFile(fp.file);
    }
  },

  


  openErrorConsole: function WS_openErrorConsole()
  {
    this.browserWindow.toJavaScriptConsole();
  },

  


  openWebConsole: function WS_openWebConsole()
  {
    if (!this.browserWindow.HUDConsoleUI.getOpenHUD()) {
      this.browserWindow.HUDConsoleUI.toggleHUD();
    }
    this.browserWindow.focus();
  },

  


  setContentContext: function WS_setContentContext()
  {
    let content = document.getElementById("ws-menu-content");
    document.getElementById("ws-menu-chrome").removeAttribute("checked");
    content.setAttribute("checked", true);
    this.statusbarStatus.label = content.getAttribute("label");
    this.executionContext = WORKSPACE_CONTEXT_CONTENT;
  },

  


  setChromeContext: function WS_setChromeContext()
  {
    let chrome = document.getElementById("ws-menu-chrome");
    document.getElementById("ws-menu-content").removeAttribute("checked");
    chrome.setAttribute("checked", true);
    this.statusbarStatus.label = chrome.getAttribute("label");
    this.executionContext = WORKSPACE_CONTEXT_CHROME;
  },

  






  getWindowId: function HS_getWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },
};

XPCOMUtils.defineLazyGetter(Workspace, "strings", function () {
  return Services.strings.createBundle(WORKSPACE_L10N);
});

