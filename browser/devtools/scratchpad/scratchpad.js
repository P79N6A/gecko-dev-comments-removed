













"use strict";

let require = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;

let { Cc, Ci, Cu } = require("chrome");
let promise = require("sdk/core/promise");
let Telemetry = require("devtools/shared/telemetry");
let DevtoolsHelpers = require("devtools/shared/helpers");
let TargetFactory = require("devtools/framework/target").TargetFactory;
const escodegen = require("escodegen/escodegen");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/scratchpad-manager.jsm");
Cu.import("resource://gre/modules/jsdebugger.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/reflect.jsm");
Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "VariablesView",
  "resource:///modules/devtools/VariablesView.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "VariablesViewController",
  "resource:///modules/devtools/VariablesViewController.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ObjectClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyGetter(this, "REMOTE_TIMEOUT", () =>
  Services.prefs.getIntPref("devtools.debugger.remote-timeout")
);
 
const SCRATCHPAD_CONTEXT_CONTENT = 1;
const SCRATCHPAD_CONTEXT_BROWSER = 2;
const SCRATCHPAD_L10N = "chrome://browser/locale/devtools/scratchpad.properties";
const DEVTOOLS_CHROME_ENABLED = "devtools.chrome.enabled";
const PREF_RECENT_FILES_MAX = "devtools.scratchpad.recentFilesMax";
const BUTTON_POSITION_SAVE = 0;
const BUTTON_POSITION_CANCEL = 1;
const BUTTON_POSITION_DONT_SAVE = 2;
const BUTTON_POSITION_REVERT = 0;
const VARIABLES_VIEW_URL = "chrome://browser/content/devtools/widgets/VariablesView.xul";



let telemetry = new Telemetry();
telemetry.toolOpened("scratchpad");




var Scratchpad = {
  _instanceId: null,
  _initialWindowTitle: document.title,

  






  _scanModeLine: function SP__scanModeLine(aLine="")
  {
    aLine = aLine.trim();

    let obj = {};
    let ch1 = aLine.charAt(0);
    let ch2 = aLine.charAt(1);

    if (ch1 !== "/" || (ch2 !== "*" && ch2 !== "/")) {
      return obj;
    }

    aLine = aLine
      .replace(/^\/\//, "")
      .replace(/^\/\*/, "")
      .replace(/\*\/$/, "");

    aLine.split(",").forEach(pair => {
      let [key, val] = pair.split(":");

      if (key && val) {
        obj[key.trim()] = val.trim();
      }
    });

    return obj;
  },

  









  executionContext: SCRATCHPAD_CONTEXT_CONTENT,

  




  initialized: false,

  



  get notificationBox()
  {
    return document.getElementById("scratchpad-notificationbox");
  },

  





  get selectedText()
  {
    return this.editor.getSelectedText();
  },

  












  getText: function SP_getText(aStart, aEnd)
  {
    return this.editor.getText(aStart, aEnd);
  },

  











  setText: function SP_setText(aText, aStart, aEnd)
  {
    this.editor.setText(aText, aStart, aEnd);
  },

  





  setFilename: function SP_setFilename(aFilename)
  {
    this.filename = aFilename;
    this._updateTitle();
  },

  



  _updateTitle: function SP__updateTitle()
  {
    let title = this.filename || this._initialWindowTitle;

    if (this.editor && this.editor.dirty) {
      title = "*" + title;
    }

    document.title = title;
  },

  







  getState: function SP_getState()
  {
    return {
      filename: this.filename,
      text: this.getText(),
      executionContext: this.executionContext,
      saved: !this.editor.dirty,
    };
  },

  






  setState: function SP_setState(aState)
  {
    if (aState.filename) {
      this.setFilename(aState.filename);
    }
    if (this.editor) {
      this.editor.dirty = !aState.saved;
    }

    if (aState.executionContext == SCRATCHPAD_CONTEXT_BROWSER) {
      this.setBrowserContext();
    }
    else {
      this.setContentContext();
    }
  },

  


  get browserWindow()
  {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  


  get gBrowser()
  {
    let recentWin = this.browserWindow;
    return recentWin ? recentWin.gBrowser : null;
  },

  



  get uniqueName()
  {
    return "Scratchpad/" + this._instanceId;
  },


  


  get sidebar()
  {
    if (!this._sidebar) {
      this._sidebar = new ScratchpadSidebar(this);
    }
    return this._sidebar;
  },

  


  deselect: function SP_deselect()
  {
    this.editor.dropSelection();
  },

  







  selectRange: function SP_selectRange(aStart, aEnd)
  {
    this.editor.setSelection(aStart, aEnd);
  },

  






  getSelectionRange: function SP_getSelection()
  {
    return this.editor.getSelection();
  },

  








  evaluate: function SP_evaluate(aString)
  {
    let connection;
    if (this.executionContext == SCRATCHPAD_CONTEXT_CONTENT) {
      connection = ScratchpadTab.consoleFor(this.gBrowser.selectedTab);
    }
    else {
      connection = ScratchpadWindow.consoleFor(this.browserWindow);
    }

    let evalOptions = { url: this.uniqueName };

    return connection.then(({ debuggerClient, webConsoleClient }) => {
      let deferred = promise.defer();

      webConsoleClient.evaluateJS(aString, aResponse => {
        this.debuggerClient = debuggerClient;
        this.webConsoleClient = webConsoleClient;
        if (aResponse.error) {
          deferred.reject(aResponse);
        }
        else if (aResponse.exception) {
          deferred.resolve([aString, aResponse]);
        }
        else {
          deferred.resolve([aString, undefined, aResponse.result]);
        }
      }, evalOptions);

      return deferred.promise;
    });
   },

  






  execute: function SP_execute()
  {
    let selection = this.selectedText || this.getText();
    return this.evaluate(selection);
  },

  






  run: function SP_run()
  {
    let deferred = promise.defer();
    let reject = aReason => deferred.reject(aReason);

    this.execute().then(([aString, aError, aResult]) => {
      let resolve = () => deferred.resolve([aString, aError, aResult]);

      if (aError) {
        this.writeAsErrorComment(aError.exception).then(resolve, reject);
      }
      else {
        this.deselect();
        resolve();
      }
    }, reject);

    return deferred.promise;
  },

  







  inspect: function SP_inspect()
  {
    let deferred = promise.defer();
    let reject = aReason => deferred.reject(aReason);

    this.execute().then(([aString, aError, aResult]) => {
      let resolve = () => deferred.resolve([aString, aError, aResult]);

      if (aError) {
        this.writeAsErrorComment(aError.exception).then(resolve, reject);
      }
      else if (VariablesView.isPrimitive({ value: aResult })) {
        this._writePrimitiveAsComment(aResult).then(resolve, reject);
      }
      else {
        this.deselect();
        this.sidebar.open(aString, aResult).then(resolve, reject);
      }
    }, reject);

    return deferred.promise;
  },

  







  reloadAndRun: function SP_reloadAndRun()
  {
    let deferred = promise.defer();

    if (this.executionContext !== SCRATCHPAD_CONTEXT_CONTENT) {
      Cu.reportError(this.strings.
          GetStringFromName("scratchpadContext.invalid"));
      return;
    }

    let browser = this.gBrowser.selectedBrowser;

    this._reloadAndRunEvent = evt => {
      if (evt.target !== browser.contentDocument) {
        return;
      }

      browser.removeEventListener("load", this._reloadAndRunEvent, true);

      this.run().then(aResults => deferred.resolve(aResults));
    };

    browser.addEventListener("load", this._reloadAndRunEvent, true);
    browser.contentWindow.location.reload();

    return deferred.promise;
  },

  








  display: function SP_display()
  {
    let deferred = promise.defer();
    let reject = aReason => deferred.reject(aReason);

    this.execute().then(([aString, aError, aResult]) => {
      let resolve = () => deferred.resolve([aString, aError, aResult]);

      if (aError) {
        this.writeAsErrorComment(aError.exception).then(resolve, reject);
      }
      else if (VariablesView.isPrimitive({ value: aResult })) {
        this._writePrimitiveAsComment(aResult).then(resolve, reject);
      }
      else {
        let objectClient = new ObjectClient(this.debuggerClient, aResult);
        objectClient.getDisplayString(aResponse => {
          if (aResponse.error) {
            reportError("display", aResponse);
            reject(aResponse);
          }
          else {
            let string = aResponse.displayString;
            if (string && string.type == "null") {
              string = "Exception: " +
                       this.strings.GetStringFromName("stringConversionFailed");
            }
            this.writeAsComment(string);
            resolve();
          }
        });
      }
    }, reject);

    return deferred.promise;
  },

  


  prettyPrint: function SP_prettyPrint() {
    const uglyText = this.getText();
    const tabsize = Services.prefs.getIntPref("devtools.editor.tabsize");
    try {
      const ast = Reflect.parse(uglyText);
      const prettyText = escodegen.generate(ast, {
        format: {
          indent: {
            style: " ".repeat(tabsize)
          }
        }
      });
      this.setText(prettyText);
    } catch (e) {
      this.writeAsErrorComment(DevToolsUtils.safeErrorString(e));
    }
  },

  









  _writePrimitiveAsComment: function SP__writePrimitiveAsComment(aValue)
  {
    let deferred = promise.defer();

    if (aValue.type == "longString") {
      let client = this.webConsoleClient;
      client.longString(aValue).substring(0, aValue.length, aResponse => {
        if (aResponse.error) {
          reportError("display", aResponse);
          deferred.reject(aResponse);
        }
        else {
          deferred.resolve(aResponse.substring);
        }
      });
    }
    else {
      deferred.resolve(aValue.type || aValue);
    }

    return deferred.promise.then(aComment => {
      this.writeAsComment(aComment);
    });
  },

  





  writeAsComment: function SP_writeAsComment(aValue)
  {
    let selection = this.getSelectionRange();
    let insertionPoint = selection.start != selection.end ?
                         selection.end : 
                         this.editor.getCharCount(); 

    let newComment = "\n/*\n" + aValue + "\n*/";

    this.setText(newComment, insertionPoint, insertionPoint);

    
    this.selectRange(insertionPoint, insertionPoint + newComment.length);
  },

  






  writeAsErrorComment: function SP_writeAsErrorComment(aError)
  {
    let deferred = promise.defer();

    if (VariablesView.isPrimitive({ value: aError })) {
      deferred.resolve(aError);
    }
    else {
      let reject = aReason => deferred.reject(aReason);
      let objectClient = new ObjectClient(this.debuggerClient, aError);

      
      
      
      let names = ["message", "stack", "fileName", "lineNumber"];
      let promises = names.map(aName => {
        let deferred = promise.defer();

        objectClient.getProperty(aName, aResponse => {
          if (aResponse.error) {
            deferred.reject(aResponse);
          }
          else {
            deferred.resolve({
              name: aName,
              descriptor: aResponse.descriptor
            });
          }
        });

        return deferred.promise;
      });

      {
        
        
        let deferred = promise.defer();
        objectClient.getPrototypeAndProperties(aResponse => {
          if (aResponse.error) {
            deferred.reject(aResponse);
          }
          else {
            deferred.resolve(aResponse);
          }
        });
        promises.push(deferred.promise);
      }

      promise.all(promises).then(aProperties => {
        let error = {};
        let safeGetters;

        
        for (let property of aProperties) {
          if (property.descriptor) {
            error[property.name] = property.descriptor.value;
          }
          else if (property.safeGetterValues) {
            safeGetters = property.safeGetterValues;
          }
        }

        if (safeGetters) {
          for (let key of Object.keys(safeGetters)) {
            if (!error.hasOwnProperty(key)) {
              error[key] = safeGetters[key].getterValue;
            }
          }
        }

        
        let stack;
        if (typeof error.stack == "string") {
          stack = error.stack;
        }
        else if (typeof error.fileName == "number") {
          stack = "@" + error.fileName;
          if (typeof error.lineNumber == "number") {
            stack += ":" + error.lineNumber;
          }
        }
        else if (typeof error.lineNumber == "number") {
          stack = "@" + error.lineNumber;
        }

        stack = stack ? "\n" + stack.replace(/\n$/, "") : "";

        if (typeof error.message == "string") {
          deferred.resolve(error.message + stack);
        }
        else {
          objectClient.getDisplayString(aResult => {
            if (aResult.error) {
              deferred.reject(aResult);
            }
            else if (aResult.displayString.type == "null") {
              deferred.resolve(stack);
            }
            else {
              deferred.resolve(aResult.displayString + stack);
            }
          }, reject);
        }
      }, reject);
    }

    return deferred.promise.then(aMessage => {
      console.log(aMessage);
      this.writeAsComment("Exception: " + aMessage);
    });
  },

  

  




  openScratchpad: function SP_openScratchpad()
  {
    return ScratchpadManager.openScratchpad();
  },

  














  exportToFile: function SP_exportToFile(aFile, aNoConfirmation, aSilentError,
                                         aCallback)
  {
    if (!aNoConfirmation && aFile.exists() &&
        !window.confirm(this.strings.
                        GetStringFromName("export.fileOverwriteConfirmation"))) {
      return;
    }

    let encoder = new TextEncoder();
    let buffer = encoder.encode(this.getText());
    let writePromise = OS.File.writeAtomic(aFile.path, buffer,{tmpPath: aFile.path + ".tmp"});
    writePromise.then(value => {
      if (aCallback) {
        aCallback.call(this, Components.results.NS_OK);
      }
    }, reason => {
      if (!aSilentError) {
        window.alert(this.strings.GetStringFromName("saveFile.failed"));
      }
      if (aCallback) {
        aCallback.call(this, Components.results.NS_ERROR_UNEXPECTED);
      }
    });

  },

  













  importFromFile: function SP_importFromFile(aFile, aSilentError, aCallback)
  {
    
    let channel = NetUtil.newChannel(aFile);
    channel.contentType = "application/javascript";

    NetUtil.asyncFetch(channel, (aInputStream, aStatus) => {
      let content = null;

      if (Components.isSuccessCode(aStatus)) {
        let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
        converter.charset = "UTF-8";
        content = NetUtil.readInputStreamToString(aInputStream,
                                                  aInputStream.available());
        content = converter.ConvertToUnicode(content);

        
        let line = content.split("\n")[0];
        let modeline = this._scanModeLine(line);
        let chrome = Services.prefs.getBoolPref(DEVTOOLS_CHROME_ENABLED);

        if (chrome && modeline["-sp-context"] === "browser") {
          this.setBrowserContext();
        }

        this.setText(content);
        this.editor.resetUndo();
      }
      else if (!aSilentError) {
        window.alert(this.strings.GetStringFromName("openFile.failed"));
      }

      if (aCallback) {
        aCallback.call(this, aStatus, content);
      }
    });
  },

  





  openFile: function SP_openFile(aIndex)
  {
    let promptCallback = aFile => {
      this.promptSave((aCloseFile, aSaved, aStatus) => {
        let shouldOpen = aCloseFile;
        if (aSaved && !Components.isSuccessCode(aStatus)) {
          shouldOpen = false;
        }

        if (shouldOpen) {
          let file;
          if (aFile) {
            file = aFile;
          } else {
            file = Components.classes["@mozilla.org/file/local;1"].
                   createInstance(Components.interfaces.nsILocalFile);
            let filePath = this.getRecentFiles()[aIndex];
            file.initWithPath(filePath);
          }

          if (!file.exists()) {
            this.notificationBox.appendNotification(
              this.strings.GetStringFromName("fileNoLongerExists.notification"),
              "file-no-longer-exists",
              null,
              this.notificationBox.PRIORITY_WARNING_HIGH,
              null);

            this.clearFiles(aIndex, 1);
            return;
          }

          this.setFilename(file.path);
          this.importFromFile(file, false);
          this.setRecentFile(file);
        }
      });
    };

    if (aIndex > -1) {
      promptCallback();
    } else {
      let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
      fp.init(window, this.strings.GetStringFromName("openFile.title"),
              Ci.nsIFilePicker.modeOpen);
      fp.defaultString = "";
      fp.open(aResult => {
        if (aResult != Ci.nsIFilePicker.returnCancel) {
          promptCallback(fp.file);
        }
      });
    }
  },

  





  getRecentFiles: function SP_getRecentFiles()
  {
    let branch = Services.prefs.getBranch("devtools.scratchpad.");
    let filePaths = [];

    
    

    if (branch.prefHasUserValue("recentFilePaths")) {
      let data = branch.getComplexValue("recentFilePaths",
        Ci.nsISupportsString).data;
      filePaths = JSON.parse(data);
    }

    return filePaths;
  },

  





  setRecentFile: function SP_setRecentFile(aFile)
  {
    let maxRecent = Services.prefs.getIntPref(PREF_RECENT_FILES_MAX);
    if (maxRecent < 1) {
      return;
    }

    let filePaths = this.getRecentFiles();
    let filesCount = filePaths.length;
    let pathIndex = filePaths.indexOf(aFile.path);

    
    if (pathIndex > -1) {
      
      if (pathIndex === (filesCount - 1)) {
        
        
        
        this.populateRecentFilesMenu();
        return;
      }

      
      
      filePaths.splice(pathIndex, 1);
    }
    
    
    else if (filesCount === maxRecent) {
      filePaths.shift();
    }

    filePaths.push(aFile.path);

    
    

    let str = Cc["@mozilla.org/supports-string;1"]
      .createInstance(Ci.nsISupportsString);
    str.data = JSON.stringify(filePaths);

    let branch = Services.prefs.getBranch("devtools.scratchpad.");
    branch.setComplexValue("recentFilePaths",
      Ci.nsISupportsString, str);
  },

  


  populateRecentFilesMenu: function SP_populateRecentFilesMenu()
  {
    let maxRecent = Services.prefs.getIntPref(PREF_RECENT_FILES_MAX);
    let recentFilesMenu = document.getElementById("sp-open_recent-menu");

    if (maxRecent < 1) {
      recentFilesMenu.setAttribute("hidden", true);
      return;
    }

    let recentFilesPopup = recentFilesMenu.firstChild;
    let filePaths = this.getRecentFiles();
    let filename = this.getState().filename;

    recentFilesMenu.setAttribute("disabled", true);
    while (recentFilesPopup.hasChildNodes()) {
      recentFilesPopup.removeChild(recentFilesPopup.firstChild);
    }

    if (filePaths.length > 0) {
      recentFilesMenu.removeAttribute("disabled");

      
      for (let i = filePaths.length - 1; i >= 0; --i) {
        let menuitem = document.createElement("menuitem");
        menuitem.setAttribute("type", "radio");
        menuitem.setAttribute("label", filePaths[i]);

        if (filePaths[i] === filename) {
          menuitem.setAttribute("checked", true);
          menuitem.setAttribute("disabled", true);
        }

        menuitem.setAttribute("oncommand", "Scratchpad.openFile(" + i + ");");
        recentFilesPopup.appendChild(menuitem);
      }

      recentFilesPopup.appendChild(document.createElement("menuseparator"));
      let clearItems = document.createElement("menuitem");
      clearItems.setAttribute("id", "sp-menu-clear_recent");
      clearItems.setAttribute("label",
                              this.strings.
                              GetStringFromName("clearRecentMenuItems.label"));
      clearItems.setAttribute("command", "sp-cmd-clearRecentFiles");
      recentFilesPopup.appendChild(clearItems);
    }
  },

  







  clearFiles: function SP_clearFile(aIndex, aLength)
  {
    let filePaths = this.getRecentFiles();
    filePaths.splice(aIndex, aLength);

    
    

    let str = Cc["@mozilla.org/supports-string;1"]
      .createInstance(Ci.nsISupportsString);
    str.data = JSON.stringify(filePaths);

    let branch = Services.prefs.getBranch("devtools.scratchpad.");
    branch.setComplexValue("recentFilePaths",
      Ci.nsISupportsString, str);
  },

  


  clearRecentFiles: function SP_clearRecentFiles()
  {
    Services.prefs.clearUserPref("devtools.scratchpad.recentFilePaths");
  },

  


  handleRecentFileMaxChange: function SP_handleRecentFileMaxChange()
  {
    let maxRecent = Services.prefs.getIntPref(PREF_RECENT_FILES_MAX);
    let menu = document.getElementById("sp-open_recent-menu");

    
    if (maxRecent < 1) {
      menu.setAttribute("hidden", true);
    } else {
      if (menu.hasAttribute("hidden")) {
        if (!menu.firstChild.hasChildNodes()) {
          this.populateRecentFilesMenu();
        }

        menu.removeAttribute("hidden");
      }

      let filePaths = this.getRecentFiles();
      if (maxRecent < filePaths.length) {
        let diff = filePaths.length - maxRecent;
        this.clearFiles(0, diff);
      }
    }
  },
  





  saveFile: function SP_saveFile(aCallback)
  {
    if (!this.filename) {
      return this.saveFileAs(aCallback);
    }

    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.filename);

    this.exportToFile(file, true, false, aStatus => {
      if (Components.isSuccessCode(aStatus)) {
        this.editor.dirty = false;
        this.setRecentFile(file);
      }
      if (aCallback) {
        aCallback(aStatus);
      }
    });
  },

  





  saveFileAs: function SP_saveFileAs(aCallback)
  {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let fpCallback = aResult => {
      if (aResult != Ci.nsIFilePicker.returnCancel) {
        this.setFilename(fp.file.path);
        this.exportToFile(fp.file, true, false, aStatus => {
          if (Components.isSuccessCode(aStatus)) {
            this.editor.dirty = false;
            this.setRecentFile(fp.file);
          }
          if (aCallback) {
            aCallback(aStatus);
          }
        });
      }
    };

    fp.init(window, this.strings.GetStringFromName("saveFileAs"),
            Ci.nsIFilePicker.modeSave);
    fp.defaultString = "scratchpad.js";
    fp.open(fpCallback);
  },

  





  revertFile: function SP_revertFile(aCallback)
  {
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.filename);

    if (!file.exists()) {
      return;
    }

    this.importFromFile(file, false, (aStatus, aContent) => {
      if (aCallback) {
        aCallback(aStatus);
      }
    });
  },

  









  promptRevert: function SP_promptRervert(aCallback)
  {
    if (this.filename) {
      let ps = Services.prompt;
      let flags = ps.BUTTON_POS_0 * ps.BUTTON_TITLE_REVERT +
                  ps.BUTTON_POS_1 * ps.BUTTON_TITLE_CANCEL;

      let button = ps.confirmEx(window,
                          this.strings.GetStringFromName("confirmRevert.title"),
                          this.strings.GetStringFromName("confirmRevert"),
                          flags, null, null, null, null, {});
      if (button == BUTTON_POSITION_CANCEL) {
        if (aCallback) {
          aCallback(false);
        }

        return;
      }
      if (button == BUTTON_POSITION_REVERT) {
        this.revertFile(aStatus => {
          if (aCallback) {
            aCallback(true, aStatus);
          }
        });

        return;
      }
    }
    if (aCallback) {
      aCallback(false);
    }
  },

  


  openErrorConsole: function SP_openErrorConsole()
  {
    this.browserWindow.HUDService.toggleBrowserConsole();
  },

  


  openWebConsole: function SP_openWebConsole()
  {
    let target = TargetFactory.forTab(this.gBrowser.selectedTab);
    gDevTools.showToolbox(target, "webconsole");
    this.browserWindow.focus();
  },

  


  setContentContext: function SP_setContentContext()
  {
    if (this.executionContext == SCRATCHPAD_CONTEXT_CONTENT) {
      return;
    }

    let content = document.getElementById("sp-menu-content");
    document.getElementById("sp-menu-browser").removeAttribute("checked");
    document.getElementById("sp-cmd-reloadAndRun").removeAttribute("disabled");
    content.setAttribute("checked", true);
    this.executionContext = SCRATCHPAD_CONTEXT_CONTENT;
    this.notificationBox.removeAllNotifications(false);
  },

  


  setBrowserContext: function SP_setBrowserContext()
  {
    if (this.executionContext == SCRATCHPAD_CONTEXT_BROWSER) {
      return;
    }

    let browser = document.getElementById("sp-menu-browser");
    let reloadAndRun = document.getElementById("sp-cmd-reloadAndRun");

    document.getElementById("sp-menu-content").removeAttribute("checked");
    reloadAndRun.setAttribute("disabled", true);
    browser.setAttribute("checked", true);

    this.executionContext = SCRATCHPAD_CONTEXT_BROWSER;
    this.notificationBox.appendNotification(
      this.strings.GetStringFromName("browserContext.notification"),
      SCRATCHPAD_CONTEXT_BROWSER,
      null,
      this.notificationBox.PRIORITY_WARNING_HIGH,
      null);
  },

  






  getInnerWindowId: function SP_getInnerWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
  },

  





  onLoad: function SP_onLoad(aEvent)
  {
    if (aEvent.target != document) {
      return;
    }

    let chrome = Services.prefs.getBoolPref(DEVTOOLS_CHROME_ENABLED);
    if (chrome) {
      let environmentMenu = document.getElementById("sp-environment-menu");
      let errorConsoleCommand = document.getElementById("sp-cmd-errorConsole");
      let chromeContextCommand = document.getElementById("sp-cmd-browserContext");
      environmentMenu.removeAttribute("hidden");
      chromeContextCommand.removeAttribute("disabled");
      errorConsoleCommand.removeAttribute("disabled");
    }

    let initialText = this.strings.formatStringFromName(
      "scratchpadIntro1",
      [DevtoolsHelpers.prettyKey(document.getElementById("sp-key-run")),
       DevtoolsHelpers.prettyKey(document.getElementById("sp-key-inspect")),
       DevtoolsHelpers.prettyKey(document.getElementById("sp-key-display"))],
      3);

    let args = window.arguments;

    if (args && args[0] instanceof Ci.nsIDialogParamBlock) {
      args = args[0];
    } else {
      
      
      Cu.reportError(this.strings. GetStringFromName("scratchpad.noargs"));
    }

    this._instanceId = args.GetString(0);

    let state = args.GetString(1) || null;
    if (state) {
      state = JSON.parse(state);
      this.setState(state);
      initialText = state.text;
    }

    this.editor = new SourceEditor();

    let config = {
      mode: SourceEditor.MODES.JAVASCRIPT,
      showLineNumbers: true,
      initialText: initialText,
      contextMenu: "scratchpad-text-popup",
    };

    let editorPlaceholder = document.getElementById("scratchpad-editor");
    this.editor.init(editorPlaceholder, config,
                     this._onEditorLoad.bind(this, state));
  },

  







  _onEditorLoad: function SP__onEditorLoad(aState)
  {
    this.editor.addEventListener(SourceEditor.EVENTS.DIRTY_CHANGED,
                                 this._onDirtyChanged);
    this.editor.focus();
    this.editor.setCaretOffset(this.editor.getCharCount());
    if (aState) {
      this.editor.dirty = !aState.saved;
    }

    this.initialized = true;
    this._triggerObservers("Ready");
    this.populateRecentFilesMenu();
    PreferenceObserver.init();
  },

  





  insertTextAtCaret: function SP_insertTextAtCaret(aText)
  {
    let caretOffset = this.editor.getCaretOffset();
    this.setText(aText, caretOffset, caretOffset);
    this.editor.setCaretOffset(caretOffset + aText.length);
  },

  








  _onDirtyChanged: function SP__onDirtyChanged(aEvent)
  {
    Scratchpad._updateTitle();
    if (Scratchpad.filename) {
      if (Scratchpad.editor.dirty) {
        document.getElementById("sp-cmd-revert").removeAttribute("disabled");
      }
      else {
        document.getElementById("sp-cmd-revert").setAttribute("disabled", true);
      }
    }
  },

  


  undo: function SP_undo()
  {
    this.editor.undo();
  },

  


  redo: function SP_redo()
  {
    this.editor.redo();
  },

  





  onUnload: function SP_onUnload(aEvent)
  {
    if (aEvent.target != document) {
      return;
    }

    
    if (this._reloadAndRunEvent) {
      this.gBrowser.selectedBrowser.removeEventListener("load",
          this._reloadAndRunEvent, true);
    }

    this.editor.removeEventListener(SourceEditor.EVENTS.DIRTY_CHANGED,
                                    this._onDirtyChanged);
    PreferenceObserver.uninit();

    this.editor.destroy();
    this.editor = null;
    if (this._sidebar) {
      this._sidebar.destroy();
      this._sidebar = null;
    }
    this.webConsoleClient = null;
    this.debuggerClient = null;
    this.initialized = false;
  },

  












  promptSave: function SP_promptSave(aCallback)
  {
    if (this.editor.dirty) {
      let ps = Services.prompt;
      let flags = ps.BUTTON_POS_0 * ps.BUTTON_TITLE_SAVE +
                  ps.BUTTON_POS_1 * ps.BUTTON_TITLE_CANCEL +
                  ps.BUTTON_POS_2 * ps.BUTTON_TITLE_DONT_SAVE;

      let button = ps.confirmEx(window,
                          this.strings.GetStringFromName("confirmClose.title"),
                          this.strings.GetStringFromName("confirmClose"),
                          flags, null, null, null, null, {});

      if (button == BUTTON_POSITION_CANCEL) {
        if (aCallback) {
          aCallback(false, false);
        }
        return false;
      }

      if (button == BUTTON_POSITION_SAVE) {
        this.saveFile(aStatus => {
          if (aCallback) {
            aCallback(true, true, aStatus);
          }
        });
        return true;
      }
    }

    if (aCallback) {
      aCallback(true, false);
    }
    return true;
  },

  








  onClose: function SP_onClose(aEvent, aCallback)
  {
    aEvent.preventDefault();
    this.close(aCallback);
  },

  






  close: function SP_close(aCallback)
  {
    this.promptSave((aShouldClose, aSaved, aStatus) => {
      let shouldClose = aShouldClose;
      if (aSaved && !Components.isSuccessCode(aStatus)) {
        shouldClose = false;
      }

      if (shouldClose) {
        telemetry.toolClosed("scratchpad");
        window.close();
      }

      if (aCallback) {
        aCallback();
      }
    });
  },

  _observers: [],

  












  addObserver: function SP_addObserver(aObserver)
  {
    this._observers.push(aObserver);
  },

  





  removeObserver: function SP_removeObserver(aObserver)
  {
    let index = this._observers.indexOf(aObserver);
    if (index != -1) {
      this._observers.splice(index, 1);
    }
  },

  








  _triggerObservers: function SP_triggerObservers(aName, aArgs)
  {
    
    if (!aArgs) {
      aArgs = [this];
    } else {
      aArgs.unshift(this);
    }

    
    for (let i = 0; i < this._observers.length; ++i) {
      let observer = this._observers[i];
      let handler = observer["on" + aName];
      if (handler) {
        handler.apply(observer, aArgs);
      }
    }
  },

  


  openDocumentationPage: function SP_openDocumentationPage()
  {
    let url = this.strings.GetStringFromName("help.openDocumentationPage");
    let newTab = this.gBrowser.addTab(url);
    this.browserWindow.focus();
    this.gBrowser.selectedTab = newTab;
  },
};









function ScratchpadTab(aTab)
{
  this._tab = aTab;
}

let scratchpadTargets = new WeakMap();










ScratchpadTab.consoleFor = function consoleFor(aSubject)
{
  if (!scratchpadTargets.has(aSubject)) {
    scratchpadTargets.set(aSubject, new this(aSubject));
  }
  return scratchpadTargets.get(aSubject).connect();
};


ScratchpadTab.prototype = {
  


  _connector: null,

  





  connect: function ST_connect()
  {
    if (this._connector) {
      return this._connector;
    }

    let deferred = promise.defer();
    this._connector = deferred.promise;

    let connectTimer = setTimeout(() => {
      deferred.reject({
        error: "timeout",
        message: Scratchpad.strings.GetStringFromName("connectionTimeout"),
      });
    }, REMOTE_TIMEOUT);

    deferred.promise.then(() => clearTimeout(connectTimer));

    this._attach().then(aTarget => {
      let consoleActor = aTarget.form.consoleActor;
      let client = aTarget.client;
      client.attachConsole(consoleActor, [], (aResponse, aWebConsoleClient) => {
        if (aResponse.error) {
          reportError("attachConsole", aResponse);
          deferred.reject(aResponse);
        }
        else {
          deferred.resolve({
            webConsoleClient: aWebConsoleClient,
            debuggerClient: client
          });
        }
      });
    });

    return deferred.promise;
  },

  





  _attach: function ST__attach()
  {
    let target = TargetFactory.forTab(this._tab);
    return target.makeRemote().then(() => target);
  },
};






function ScratchpadWindow() {}

ScratchpadWindow.consoleFor = ScratchpadTab.consoleFor;

ScratchpadWindow.prototype = Heritage.extend(ScratchpadTab.prototype, {
  





  _attach: function SW__attach()
  {
    let deferred = promise.defer();

    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }

    let client = new DebuggerClient(DebuggerServer.connectPipe());
    client.connect(() => {
      client.listTabs(aResponse => {
        if (aResponse.error) {
          reportError("listTabs", aResponse);
          deferred.reject(aResponse);
        }
        else {
          deferred.resolve({ form: aResponse, client: client });
        }
      });
    });

    return deferred.promise;
  }
});






function ScratchpadSidebar(aScratchpad)
{
  let ToolSidebar = require("devtools/framework/sidebar").ToolSidebar;
  let tabbox = document.querySelector("#scratchpad-sidebar");
  this._sidebar = new ToolSidebar(tabbox, this, "scratchpad");
  this._scratchpad = aScratchpad;
}

ScratchpadSidebar.prototype = {
  


  _sidebar: null,

  


  variablesView: null,

  


  visible: false,

  










  open: function SS_open(aEvalString, aObject)
  {
    this.show();

    let deferred = promise.defer();

    let onTabReady = () => {
      if (this.variablesView) {
        this.variablesView.controller.releaseActors();
      }
      else {
        let window = this._sidebar.getWindowForTab("variablesview");
        let container = window.document.querySelector("#variables");

        this.variablesView = new VariablesView(container, {
          searchEnabled: true,
          searchPlaceholder: this._scratchpad.strings
                             .GetStringFromName("propertiesFilterPlaceholder")
        });

        VariablesViewController.attach(this.variablesView, {
          getObjectClient: aGrip => {
            return new ObjectClient(this._scratchpad.debuggerClient, aGrip);
          },
          getLongStringClient: aActor => {
            return this._scratchpad.webConsoleClient.longString(aActor);
          },
          releaseActor: aActor => {
            this._scratchpad.debuggerClient.release(aActor);
          }
        });
      }
      this._update(aObject).then(() => deferred.resolve());
    };

    if (this._sidebar.getCurrentTabID() == "variablesview") {
      onTabReady();
    }
    else {
      this._sidebar.once("variablesview-ready", onTabReady);
      this._sidebar.addTab("variablesview", VARIABLES_VIEW_URL, true);
    }

    return deferred.promise;
  },

  


  show: function SS_show()
  {
    if (!this.visible) {
      this.visible = true;
      this._sidebar.show();
    }
  },

  


  hide: function SS_hide()
  {
    if (this.visible) {
      this.visible = false;
      this._sidebar.hide();
    }
  },

  





  destroy: function SS_destroy()
  {
    if (this.variablesView) {
      this.variablesView.controller.releaseActors();
      this.variablesView = null;
    }
    return this._sidebar.destroy();
  },

  







  _update: function SS__update(aObject)
  {
    let view = this.variablesView;
    view.empty();

    let scope = view.addScope();
    scope.expanded = true;
    scope.locked = true;

    let container = scope.addItem();
    return view.controller.expand(container, aObject);
  }
};










function reportError(aAction, aResponse)
{
  Cu.reportError(aAction + " failed: " + aResponse.error + " " +
                 aResponse.message);
}






var PreferenceObserver = {
  _initialized: false,

  init: function PO_init()
  {
    if (this._initialized) {
      return;
    }

    this.branch = Services.prefs.getBranch("devtools.scratchpad.");
    this.branch.addObserver("", this, false);
    this._initialized = true;
  },

  observe: function PO_observe(aMessage, aTopic, aData)
  {
    if (aTopic != "nsPref:changed") {
      return;
    }

    if (aData == "recentFilesMax") {
      Scratchpad.handleRecentFileMaxChange();
    }
    else if (aData == "recentFilePaths") {
      Scratchpad.populateRecentFilesMenu();
    }
  },

  uninit: function PO_uninit () {
    if (!this.branch) {
      return;
    }

    this.branch.removeObserver("", this);
    this.branch = null;
  }
};

XPCOMUtils.defineLazyGetter(Scratchpad, "strings", function () {
  return Services.strings.createBundle(SCRATCHPAD_L10N);
});

addEventListener("load", Scratchpad.onLoad.bind(Scratchpad), false);
addEventListener("unload", Scratchpad.onUnload.bind(Scratchpad), false);
addEventListener("close", Scratchpad.onClose.bind(Scratchpad), false);
