













"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const SCRATCHPAD_CONTEXT_CONTENT = 1;
const SCRATCHPAD_CONTEXT_BROWSER = 2;
const BUTTON_POSITION_SAVE       = 0;
const BUTTON_POSITION_CANCEL     = 1;
const BUTTON_POSITION_DONT_SAVE  = 2;
const BUTTON_POSITION_REVERT     = 0;
const EVAL_FUNCTION_TIMEOUT      = 1000; 

const MAXIMUM_FONT_SIZE = 96;
const MINIMUM_FONT_SIZE = 6;
const NORMAL_FONT_SIZE = 12;

const SCRATCHPAD_L10N = "chrome://browser/locale/devtools/scratchpad.properties";
const DEVTOOLS_CHROME_ENABLED = "devtools.chrome.enabled";
const PREF_RECENT_FILES_MAX = "devtools.scratchpad.recentFilesMax";
const SHOW_TRAILING_SPACE = "devtools.scratchpad.showTrailingSpace";
const ENABLE_AUTOCOMPLETION = "devtools.scratchpad.enableAutocompletion";
const TAB_SIZE = "devtools.editor.tabsize";

const VARIABLES_VIEW_URL = "chrome://browser/content/devtools/widgets/VariablesView.xul";

const require   = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;

const Telemetry = require("devtools/shared/telemetry");
const Editor    = require("devtools/sourceeditor/editor");
const TargetFactory = require("devtools/framework/target").TargetFactory;

const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
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

XPCOMUtils.defineLazyModuleGetter(this, "EnvironmentClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ObjectClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyGetter(this, "REMOTE_TIMEOUT", () =>
  Services.prefs.getIntPref("devtools.debugger.remote-timeout"));

XPCOMUtils.defineLazyModuleGetter(this, "ShortcutUtils",
  "resource://gre/modules/ShortcutUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Reflect",
  "resource://gre/modules/reflect.jsm");



let telemetry = new Telemetry();
telemetry.toolOpened("scratchpad");

let WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;




var Scratchpad = {
  _instanceId: null,
  _initialWindowTitle: document.title,
  _dirty: false,

  






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

  


  _setupPopupShowingListeners: function SP_setupPopupShowing() {
    let elementIDs = ['sp-menu_editpopup', 'scratchpad-text-popup'];

    for (let elementID of elementIDs) {
      let elem = document.getElementById(elementID);
      if (elem) {
        elem.addEventListener("popupshowing", function () {
          goUpdateGlobalEditMenuItems();
          let commands = ['cmd_undo', 'cmd_redo', 'cmd_delete', 'cmd_findAgain'];
          commands.forEach(goUpdateCommand);
        });
      }
    }
  },

  


  _setupCommandListeners: function SP_setupCommands() {
    let commands = {
      "cmd_gotoLine": () => {
        goDoCommand('cmd_gotoLine');
      },
      "sp-cmd-newWindow": () => {
        Scratchpad.openScratchpad();
      },
      "sp-cmd-openFile": () => {
        Scratchpad.openFile();
      },
      "sp-cmd-clearRecentFiles": () => {
        Scratchpad.clearRecentFiles();
      },
      "sp-cmd-save": () => {
        Scratchpad.saveFile();
      },
      "sp-cmd-saveas": () => {
        Scratchpad.saveFileAs();
      },
      "sp-cmd-revert": () => {
        Scratchpad.promptRevert();
      },
      "sp-cmd-close": () => {
        Scratchpad.close();
      },
      "sp-cmd-run": () => {
        Scratchpad.run();
      },
      "sp-cmd-inspect": () => {
        Scratchpad.inspect();
      },
      "sp-cmd-display": () => {
        Scratchpad.display();
      },
      "sp-cmd-pprint": () => {
        Scratchpad.prettyPrint();
      },
      "sp-cmd-contentContext": () => {
        Scratchpad.setContentContext();
      },
      "sp-cmd-browserContext": () => {
        Scratchpad.setBrowserContext();
      },
      "sp-cmd-reloadAndRun": () => {
        Scratchpad.reloadAndRun();
      },
      "sp-cmd-evalFunction": () => {
        Scratchpad.evalTopLevelFunction();
      },
      "sp-cmd-errorConsole": () => {
        Scratchpad.openErrorConsole();
      },
      "sp-cmd-webConsole": () => {
        Scratchpad.openWebConsole();
      },
      "sp-cmd-documentationLink": () => {
        Scratchpad.openDocumentationPage();
      },
      "sp-cmd-hideSidebar": () => {
        Scratchpad.sidebar.hide();
      },
      "sp-cmd-line-numbers": () => {
        Scratchpad.toggleEditorOption('lineNumbers');
      },
      "sp-cmd-wrap-text": () => {
        Scratchpad.toggleEditorOption('lineWrapping');
      },
      "sp-cmd-highlight-trailing-space": () => {
        Scratchpad.toggleEditorOption('showTrailingSpace');
      },
      "sp-cmd-larger-font": () => {
        Scratchpad.increaseFontSize();
      },
      "sp-cmd-smaller-font": () => {
        Scratchpad.decreaseFontSize();
      },
      "sp-cmd-normal-font": () => {
        Scratchpad.normalFontSize();
      },
    }

    for (let command in commands) {
      let elem = document.getElementById(command);
      if (elem) {
        elem.addEventListener("command", commands[command]);
      }
    }
  },

  









  executionContext: SCRATCHPAD_CONTEXT_CONTENT,

  




  initialized: false,

  


  get dirty()
  {
    let clean = this.editor && this.editor.isClean();
    return this._dirty || !clean;
  },

  


  set dirty(aValue)
  {
    this._dirty = aValue;
    if (!aValue && this.editor)
      this.editor.setClean();
    this._updateTitle();
  },

  



  get notificationBox()
  {
    return document.getElementById("scratchpad-notificationbox");
  },

  


  hideMenu: function SP_hideMenu()
  {
    document.getElementById("sp-menubar").style.display = "none";
  },

  


  showMenu: function SP_showMenu()
  {
    document.getElementById("sp-menubar").style.display = "";
  },

  












  getText: function SP_getText(aStart, aEnd)
  {
    var value = this.editor.getText();
    return value.slice(aStart || 0, aEnd || value.length);
  },

  





  setFilename: function SP_setFilename(aFilename)
  {
    this.filename = aFilename;
    this._updateTitle();
  },

  



  _updateTitle: function SP__updateTitle()
  {
    let title = this.filename || this._initialWindowTitle;

    if (this.dirty)
      title = "*" + title;

    document.title = title;
  },

  







  getState: function SP_getState()
  {
    return {
      filename: this.filename,
      text: this.getText(),
      executionContext: this.executionContext,
      saved: !this.dirty
    };
  },

  






  setState: function SP_setState(aState)
  {
    if (aState.filename)
      this.setFilename(aState.filename);

    this.dirty = !aState.saved;

    if (aState.executionContext == SCRATCHPAD_CONTEXT_BROWSER)
      this.setBrowserContext();
    else
      this.setContentContext();
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

  



  setText: function SP_setText(value)
  {
    return this.editor.setText(value);
  },

  








  evaluate: function SP_evaluate(aString)
  {
    let connection;
    if (this.target) {
      connection = ScratchpadTarget.consoleFor(this.target);
    }
    else if (this.executionContext == SCRATCHPAD_CONTEXT_CONTENT) {
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
        else if (aResponse.exception !== null) {
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
    WebConsoleUtils.usageCount++;
    let selection = this.editor.getSelection() || this.getText();
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
        this.editor.dropSelection();
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
      else {
        this.editor.dropSelection();
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
            this.writeAsComment(aResponse.displayString);
            resolve();
          }
        });
      }
    }, reject);

    return deferred.promise;
  },

  _prettyPrintWorker: null,

  


  get prettyPrintWorker() {
    if (!this._prettyPrintWorker) {
      this._prettyPrintWorker = new ChromeWorker(
        "resource://gre/modules/devtools/server/actors/pretty-print-worker.js");

      this._prettyPrintWorker.addEventListener("error", ({ message, filename, lineno }) => {
        DevToolsUtils.reportException(message + " @ " + filename + ":" + lineno);
      }, false);
    }
    return this._prettyPrintWorker;
  },

  






  prettyPrint: function SP_prettyPrint() {
    const uglyText = this.getText();
    const tabsize = Services.prefs.getIntPref(TAB_SIZE);
    const id = Math.random();
    const deferred = promise.defer();

    const onReply = ({ data }) => {
      if (data.id !== id) {
        return;
      }
      this.prettyPrintWorker.removeEventListener("message", onReply, false);

      if (data.error) {
        let errorString = DevToolsUtils.safeErrorString(data.error);
        this.writeAsErrorComment(errorString);
        deferred.reject(errorString);
      } else {
        this.editor.setText(data.code);
        deferred.resolve(data.code);
      }
    };

    this.prettyPrintWorker.addEventListener("message", onReply, false);
    this.prettyPrintWorker.postMessage({
      id: id,
      url: "(scratchpad)",
      indent: tabsize,
      source: uglyText
    });

    return deferred.promise;
  },

  



  _parseText: function SP__parseText(aText) {
    try {
      return Reflect.parse(aText);
    } catch (e) {
      this.writeAsErrorComment(DevToolsUtils.safeErrorString(e));
      return false;
    }
  },

  





  _containsCursor: function (aLoc, aCursorPos) {
    
    const lineNumber = aCursorPos.line + 1;
    const columnNumber = aCursorPos.ch;

    if (aLoc.start.line <= lineNumber && aLoc.end.line >= lineNumber) {
      if (aLoc.start.line === aLoc.end.line) {
        return aLoc.start.column <= columnNumber
          && aLoc.end.column >= columnNumber;
      }

      if (aLoc.start.line == lineNumber) {
        return columnNumber >= aLoc.start.column;
      }

      if (aLoc.end.line == lineNumber) {
        return columnNumber <= aLoc.end.column;
      }

      return true;
    }

    return false;
  },

  




  _findTopLevelFunction: function SP__findTopLevelFunction(aAst, aCursorPos) {
    for (let statement of aAst.body) {
      switch (statement.type) {
      case "FunctionDeclaration":
        if (this._containsCursor(statement.loc, aCursorPos)) {
          return statement;
        }
        break;

      case "VariableDeclaration":
        for (let decl of statement.declarations) {
          if (!decl.init) {
            continue;
          }
          if ((decl.init.type == "FunctionExpression"
               || decl.init.type == "ArrowExpression")
              && this._containsCursor(decl.loc, aCursorPos)) {
            return decl;
          }
        }
        break;
      }
    }

    return null;
  },

  






  _getFunctionText: function SP__getFunctionText(aFunction, aFullText) {
    let functionText = "";
    
    
    let lineNumber = 0;
    const { start, end } = aFunction.loc;
    const singleLine = start.line === end.line;

    for (let line of aFullText.split(/\n/g)) {
      lineNumber++;

      if (singleLine && start.line === lineNumber) {
        functionText = line.slice(start.column, end.column);
        break;
      }

      if (start.line === lineNumber) {
        functionText += line.slice(start.column) + "\n";
        continue;
      }

      if (end.line === lineNumber) {
        functionText += line.slice(0, end.column);
        break;
      }

      if (start.line < lineNumber && end.line > lineNumber) {
        functionText += line + "\n";
      }
    }

    return functionText;
  },

  




  evalTopLevelFunction: function SP_evalTopLevelFunction() {
    const text = this.getText();
    const ast = this._parseText(text);
    if (!ast) {
      return promise.resolve([text, undefined, undefined]);
    }

    const cursorPos = this.editor.getCursor();
    const funcStatement = this._findTopLevelFunction(ast, cursorPos);
    if (!funcStatement) {
      return promise.resolve([text, undefined, undefined]);
    }

    let functionText = this._getFunctionText(funcStatement, text);

    
    
    if (funcStatement.type == "FunctionDeclaration"
        && !functionText.startsWith("function ")) {
      functionText = "function " + functionText;
      funcStatement.loc.start.column -= 9;
    }

    
    
    const from = {
      line: funcStatement.loc.start.line - 1,
      ch: funcStatement.loc.start.column
    };
    const to = {
      line: funcStatement.loc.end.line - 1,
      ch: funcStatement.loc.end.column
    };

    const marker = this.editor.markText(from, to, "eval-text");
    setTimeout(() => marker.clear(), EVAL_FUNCTION_TIMEOUT);

    return this.evaluate(functionText);
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
    let value = "\n/*\n" + aValue + "\n*/";

    if (this.editor.somethingSelected()) {
      let from = this.editor.getCursor("end");
      this.editor.replaceSelection(this.editor.getSelection() + value);
      let to = this.editor.getPosition(this.editor.getOffset(from) + value.length);
      this.editor.setSelection(from, to);
      return;
    }

    let text = this.editor.getText();
    this.editor.setText(text + value);

    let [ from, to ] = this.editor.getPosition(text.length, (text + value).length);
    this.editor.setSelection(from, to);
  },

  






  writeAsErrorComment: function SP_writeAsErrorComment(aError)
  {
    let deferred = promise.defer();

    if (VariablesView.isPrimitive({ value: aError })) {
      let type = aError.type;
      if (type == "undefined" ||
          type == "null" ||
          type == "Infinity" ||
          type == "-Infinity" ||
          type == "NaN" ||
          type == "-0") {
        deferred.resolve(type);
      }
      else if (type == "longString") {
        deferred.resolve(aError.initial + "\u2026");
      }
      else {
        deferred.resolve(aError);
      }
    }
    else {
      let objectClient = new ObjectClient(this.debuggerClient, aError);
      objectClient.getPrototypeAndProperties(aResponse => {
        if (aResponse.error) {
          deferred.reject(aResponse);
          return;
        }

        let { ownProperties, safeGetterValues } = aResponse;
        let error = Object.create(null);

        
        for (let key of Object.keys(safeGetterValues)) {
          error[key] = safeGetterValues[key].getterValue;
        }

        for (let key of Object.keys(ownProperties)) {
          error[key] = ownProperties[key].value;
        }

        
        let stack;
        if (typeof error.stack == "string" && error.stack) {
          stack = error.stack;
        }
        else if (typeof error.fileName == "string") {
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
          objectClient.getDisplayString(aResponse => {
            if (aResponse.error) {
              deferred.reject(aResponse);
            }
            else if (typeof aResponse.displayString == "string") {
              deferred.resolve(aResponse.displayString + stack);
            }
            else {
              deferred.resolve(stack);
            }
          });
        }
      });
    }

    return deferred.promise.then(aMessage => {
      console.error(aMessage);
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

        this.editor.setText(content);
        this.editor.clearHistory();
        this.dirty = false;
        document.getElementById("sp-cmd-revert").setAttribute("disabled", true);
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
      fp.appendFilter("JavaScript Files", "*.js; *.jsm; *.json");
      fp.appendFilter("All Files", "*.*");
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

        menuitem.addEventListener("command", Scratchpad.openFile.bind(Scratchpad, i));
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
        this.dirty = false;
        document.getElementById("sp-cmd-revert").setAttribute("disabled", true);
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
            this.dirty = false;
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
    fp.appendFilter("JavaScript Files", "*.js; *.jsm; *.json");
    fp.appendFilter("All Files", "*.*");
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
      [ShortcutUtils.prettifyShortcut(document.getElementById("sp-key-run"), true),
       ShortcutUtils.prettifyShortcut(document.getElementById("sp-key-inspect"), true),
       ShortcutUtils.prettifyShortcut(document.getElementById("sp-key-display"), true)],
      3);

    let args = window.arguments;
    let state = null;

    if (args && args[0] instanceof Ci.nsIDialogParamBlock) {
      args = args[0];
      this._instanceId = args.GetString(0);

      state = args.GetString(1) || null;
      if (state) {
        state = JSON.parse(state);
        this.setState(state);
        initialText = state.text;
      }
    } else {
      this._instanceId = ScratchpadManager.createUid();
    }

    let config = {
      mode: Editor.modes.js,
      value: initialText,
      lineNumbers: true,
      contextMenu: "scratchpad-text-popup",
      showTrailingSpace: Services.prefs.getBoolPref(SHOW_TRAILING_SPACE),
      autocomplete: Services.prefs.getBoolPref(ENABLE_AUTOCOMPLETION),
    };

    this.editor = new Editor(config);
    let editorElement = document.querySelector("#scratchpad-editor");
    this.editor.appendTo(editorElement).then(() => {
      var lines = initialText.split("\n");

      this.editor.on("change", this._onChanged);
      let okstring = this.strings.GetStringFromName("selfxss.okstring");
      let msg = this.strings.formatStringFromName("selfxss.msg", [okstring], 1);
      this._onPaste = WebConsoleUtils.pasteHandlerGen(this.editor.container.contentDocument.body,
                                                      document.querySelector('#scratchpad-notificationbox'),
                                                      msg, okstring);
      editorElement.addEventListener("paste", this._onPaste);
      editorElement.addEventListener("drop", this._onPaste);
      this.editor.on("save", () => this.saveFile());
      this.editor.focus();
      this.editor.setCursor({ line: lines.length, ch: lines.pop().length });

      if (state)
        this.dirty = !state.saved;

      this.initialized = true;
      this._triggerObservers("Ready");
      this.populateRecentFilesMenu();
      PreferenceObserver.init();
      CloseObserver.init();
    }).then(null, (err) => console.error(err));
    this._setupCommandListeners();
    this._setupPopupShowingListeners();
  },

  





  _onChanged: function SP__onChanged()
  {
    Scratchpad._updateTitle();

    if (Scratchpad.filename) {
      if (Scratchpad.dirty)
        document.getElementById("sp-cmd-revert").removeAttribute("disabled");
      else
        document.getElementById("sp-cmd-revert").setAttribute("disabled", true);
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

    
    if (this._reloadAndRunEvent && this.gBrowser) {
      this.gBrowser.selectedBrowser.removeEventListener("load",
          this._reloadAndRunEvent, true);
    }

    PreferenceObserver.uninit();
    CloseObserver.uninit();
    if (this._onPaste) {
      let editorElement = document.querySelector("#scratchpad-editor");
      editorElement.removeEventListener("paste", this._onPaste);
      editorElement.removeEventListener("drop", this._onPaste);
      this._onPaste = null;
    }
    this.editor.off("change", this._onChanged);
    this.editor.destroy();
    this.editor = null;

    if (this._sidebar) {
      this._sidebar.destroy();
      this._sidebar = null;
    }

    if (this._prettyPrintWorker) {
      this._prettyPrintWorker.terminate();
      this._prettyPrintWorker = null;
    }

    scratchpadTargets = null;
    this.webConsoleClient = null;
    this.debuggerClient = null;
    this.initialized = false;
  },

  












  promptSave: function SP_promptSave(aCallback)
  {
    if (this.dirty) {
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
    let shouldClose;

    this.promptSave((aShouldClose, aSaved, aStatus) => {
       shouldClose = aShouldClose;
      if (aSaved && !Components.isSuccessCode(aStatus)) {
        shouldClose = false;
      }

      if (shouldClose) {
        telemetry.toolClosed("scratchpad");
        window.close();
      }

      if (aCallback) {
        aCallback(shouldClose);
      }
    });

    return shouldClose;
  },

  


  toggleEditorOption: function SP_toggleEditorOption(optionName)
  {
    let newOptionValue = !this.editor.getOption(optionName);
    this.editor.setOption(optionName, newOptionValue);
  },

  


  increaseFontSize: function SP_increaseFontSize()
  {
    let size = this.editor.getFontSize();

    if (size < MAXIMUM_FONT_SIZE) {
      this.editor.setFontSize(size + 1);
    }
  },

  


  decreaseFontSize: function SP_decreaseFontSize()
  {
    let size = this.editor.getFontSize();

    if (size > MINIMUM_FONT_SIZE) {
      this.editor.setFontSize(size - 1);
    }
  },

  


  normalFontSize: function SP_normalFontSize()
  {
    this.editor.setFontSize(NORMAL_FONT_SIZE);
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
  return scratchpadTargets.get(aSubject).connect(aSubject);
};


ScratchpadTab.prototype = {
  


  _connector: null,

  







  connect: function ST_connect(aSubject)
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

    this._attach(aSubject).then(aTarget => {
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

  







  _attach: function ST__attach(aSubject)
  {
    let target = TargetFactory.forTab(this._tab);
    target.once("close", () => {
      if (scratchpadTargets) {
        scratchpadTargets.delete(aSubject);
      }
    });
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


function ScratchpadTarget(aTarget)
{
  this._target = aTarget;
}

ScratchpadTarget.consoleFor = ScratchpadTab.consoleFor;

ScratchpadTarget.prototype = Heritage.extend(ScratchpadTab.prototype, {
  _attach: function ST__attach()
  {
    if (this._target.isRemote) {
      return promise.resolve(this._target);
    }
    return this._target.makeRemote().then(() => this._target);
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
          getEnvironmentClient: aGrip => {
            return new EnvironmentClient(this._scratchpad.debuggerClient, aGrip);
          },
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

  







  _update: function SS__update(aValue)
  {
    let options, onlyEnumVisible;
    if (VariablesView.isPrimitive({ value: aValue })) {
      options = { rawObject: { value: aValue } };
      onlyEnumVisible = true;
    } else {
      options = { objectActor: aValue };
      onlyEnumVisible = false;
    }
    let view = this.variablesView;
    view.onlyEnumVisible = onlyEnumVisible;
    view.empty();
    return view.controller.setSingleVariable(options).expanded;
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






var CloseObserver = {
  init: function CO_init()
  {
    Services.obs.addObserver(this, "browser-lastwindow-close-requested", false);
  },

  observe: function CO_observe(aSubject)
  {
    if (Scratchpad.close()) {
      this.uninit();
    }
    else {
      aSubject.QueryInterface(Ci.nsISupportsPRBool);
      aSubject.data = true;
    }
  },

  uninit: function CO_uninit()
  {
    
    if (this._uninited) {
      return;
    }

    this._uninited = true;
    Services.obs.removeObserver(this, "browser-lastwindow-close-requested",
                                false);
  },
};

XPCOMUtils.defineLazyGetter(Scratchpad, "strings", function () {
  return Services.strings.createBundle(SCRATCHPAD_L10N);
});

addEventListener("load", Scratchpad.onLoad.bind(Scratchpad), false);
addEventListener("unload", Scratchpad.onUnload.bind(Scratchpad), false);
addEventListener("close", Scratchpad.onClose.bind(Scratchpad), false);
