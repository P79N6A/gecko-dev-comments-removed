


















































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/PropertyPanel.jsm");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/scratchpad-manager.jsm");


const SCRATCHPAD_CONTEXT_CONTENT = 1;
const SCRATCHPAD_CONTEXT_BROWSER = 2;
const SCRATCHPAD_L10N = "chrome://browser/locale/devtools/scratchpad.properties";
const DEVTOOLS_CHROME_ENABLED = "devtools.chrome.enabled";
const BUTTON_POSITION_SAVE = 0;
const BUTTON_POSITION_CANCEL = 1;
const BUTTON_POSITION_DONT_SAVE = 2;




var Scratchpad = {
  _initialWindowTitle: document.title,

  









  executionContext: SCRATCHPAD_CONTEXT_CONTENT,

  




  initialized: false,

  



  get notificationBox() document.getElementById("scratchpad-notificationbox"),

  





  get selectedText() this.editor.getSelectedText(),

  












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
    if (this.filename) {
      document.title = (this.editor && this.editor.dirty ? "*" : "") +
                       this.filename;
    } else {
      document.title = this._initialWindowTitle;
    }
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

  






  setState: function SP_getState(aState)
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
        this.browserWindow != this._previousBrowserWindow ||
        this._previousBrowser != this.gBrowser.selectedBrowser ||
        this._previousLocation != this.gBrowser.contentWindow.location.href) {
      let contentWindow = this.gBrowser.selectedBrowser.contentWindow;
      this._contentSandbox = new Cu.Sandbox(contentWindow,
        { sandboxPrototype: contentWindow, wantXrays: false, 
          sandboxName: 'scratchpad-content'});

      this._previousBrowserWindow = this.browserWindow;
      this._previousBrowser = this.gBrowser.selectedBrowser;
      this._previousLocation = contentWindow.location.href;
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
        { sandboxPrototype: this.browserWindow, wantXrays: false, 
          sandboxName: 'scratchpad-chrome'});

      this._previousBrowserWindow = this.browserWindow;
    }

    return this._chromeSandbox;
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

  







  evalInContentSandbox: function SP_evalInContentSandbox(aString)
  {
    let error, result;
    try {
      result = Cu.evalInSandbox(aString, this.contentSandbox, "1.8",
                                "Scratchpad", 1);
    }
    catch (ex) {
      error = ex;
    }

    return [error, result];
  },

  







  evalInChromeSandbox: function SP_evalInChromeSandbox(aString)
  {
    let error, result;
    try {
      result = Cu.evalInSandbox(aString, this.chromeSandbox, "1.8",
                                "Scratchpad", 1);
    }
    catch (ex) {
      error = ex;
    }

    return [error, result];
  },

  








  evalForContext: function SP_evaluateForContext(aString)
  {
    return this.executionContext == SCRATCHPAD_CONTEXT_CONTENT ?
           this.evalInContentSandbox(aString) :
           this.evalInChromeSandbox(aString);
  },

  





  execute: function SP_execute()
  {
    let selection = this.selectedText || this.getText();
    let [error, result] = this.evalForContext(selection);
    return [selection, error, result];
  },

  



  run: function SP_run()
  {
    let [selection, error, result] = this.execute();

    if (!error) {
      this.deselect();
    } else {
      this.writeAsErrorComment(error);
    }

    return [selection, error, result];
  },

  




  inspect: function SP_inspect()
  {
    let [selection, error, result] = this.execute();

    if (!error) {
      this.deselect();
      this.openPropertyPanel(selection, result);
    } else {
      this.writeAsErrorComment(error);
    }
  },

  





  display: function SP_display()
  {
    let [selectedText, error, result] = this.execute();

    if (!error) {
      this.writeAsComment(result);
    } else {
      this.writeAsErrorComment(error);
    }
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
    let newComment = "Exception: " + aError.message + "\n" + aError.stack.substring(0, aError.stack.length - 1);
    
    this.writeAsComment(newComment);
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
          let [error, result] = self.evalForContext(aEvalString);

          if (!error) {
            propPanel.treeView.data = result;
          }
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

    let fs = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
    let modeFlags = 0x02 | 0x08 | 0x20;
    fs.init(aFile, modeFlags, 420 , fs.DEFER_OPEN);

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let input = converter.convertToInputStream(this.getText());

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
        let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
        converter.charset = "UTF-8";
        content = NetUtil.readInputStreamToString(aInputStream,
                                                  aInputStream.available());
        content = converter.ConvertToUnicode(content);
        self.setText(content);
        self.editor.resetUndo();
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
      this.setFilename(fp.file.path);
      this.importFromFile(fp.file, false);
    }
  },

  





  saveFile: function SP_saveFile(aCallback)
  {
    if (!this.filename) {
      return this.saveFileAs(aCallback);
    }

    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.filename);

    this.exportToFile(file, true, false, function(aStatus) {
      if (Components.isSuccessCode(aStatus)) {
        this.editor.dirty = false;
      }
      if (aCallback) {
        aCallback(aStatus);
      }
    });
  },

  





  saveFileAs: function SP_saveFileAs(aCallback)
  {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, this.strings.GetStringFromName("saveFileAs"),
            Ci.nsIFilePicker.modeSave);
    fp.defaultString = "scratchpad.js";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      this.setFilename(fp.file.path);

      this.exportToFile(fp.file, true, false, function(aStatus) {
        if (Components.isSuccessCode(aStatus)) {
          this.editor.dirty = false;
        }
        if (aCallback) {
          aCallback(aStatus);
        }
      });
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
    if (this.executionContext == SCRATCHPAD_CONTEXT_CONTENT) {
      return;
    }

    let content = document.getElementById("sp-menu-content");
    document.getElementById("sp-menu-browser").removeAttribute("checked");
    content.setAttribute("checked", true);
    this.executionContext = SCRATCHPAD_CONTEXT_CONTENT;
    this.notificationBox.removeAllNotifications(false);
    this.resetContext();
  },

  


  setBrowserContext: function SP_setBrowserContext()
  {
    if (this.executionContext == SCRATCHPAD_CONTEXT_BROWSER) {
      return;
    }

    let browser = document.getElementById("sp-menu-browser");
    document.getElementById("sp-menu-content").removeAttribute("checked");
    browser.setAttribute("checked", true);
    this.executionContext = SCRATCHPAD_CONTEXT_BROWSER;
    this.notificationBox.appendNotification(
      this.strings.GetStringFromName("browserContext.notification"),
      SCRATCHPAD_CONTEXT_BROWSER,
      null,
      this.notificationBox.PRIORITY_WARNING_HIGH,
      null);
    this.resetContext();
  },

  


  resetContext: function SP_resetContext()
  {
    this._chromeSandbox = null;
    this._contentSandbox = null;
    this._previousWindow = null;
    this._previousBrowser = null;
    this._previousLocation = null;
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

    let state = null;
    let initialText = this.strings.GetStringFromName("scratchpadIntro");
    if ("arguments" in window &&
         window.arguments[0] instanceof Ci.nsIDialogParamBlock) {
      state = JSON.parse(window.arguments[0].GetString(0));
      this.setState(state);
      initialText = state.text;
    }

    this.editor = new SourceEditor();

    let config = {
      mode: SourceEditor.MODES.JAVASCRIPT,
      showLineNumbers: true,
      initialText: initialText,
    };

    let editorPlaceholder = document.getElementById("scratchpad-editor");
    this.editor.init(editorPlaceholder, config,
                     this._onEditorLoad.bind(this, state));
  },

  







  _onEditorLoad: function SP__onEditorLoad(aState)
  {
    this.editor.addEventListener(SourceEditor.EVENTS.CONTEXT_MENU,
                                 this.onContextMenu);
    this.editor.addEventListener(SourceEditor.EVENTS.DIRTY_CHANGED,
                                 this._onDirtyChanged);
    this.editor.focus();
    this.editor.setCaretOffset(this.editor.getCharCount());
    if (aState) {
      this.editor.dirty = !aState.saved;
    }

    this.initialized = true;

    this._triggerObservers("Ready");
  },

  





  insertTextAtCaret: function SP_insertTextAtCaret(aText)
  {
    let caretOffset = this.editor.getCaretOffset();
    this.setText(aText, caretOffset, caretOffset);
    this.editor.setCaretOffset(caretOffset + aText.length);
  },

  







  onContextMenu: function SP_onContextMenu(aEvent)
  {
    let menu = document.getElementById("scratchpad-text-popup");
    if (menu.state == "closed") {
      menu.openPopupAtScreen(aEvent.screenX, aEvent.screenY, true);
    }
  },

  








  _onDirtyChanged: function SP__onDirtyChanged(aEvent)
  {
    Scratchpad._updateTitle();
  },

  





  onEditPopupShowing: function SP_onEditPopupShowing()
  {
    goUpdateGlobalEditMenuItems();

    let undo = document.getElementById("sp-cmd-undo");
    undo.setAttribute("disabled", !this.editor.canUndo());

    let redo = document.getElementById("sp-cmd-redo");
    redo.setAttribute("disabled", !this.editor.canRedo());
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

    this.resetContext();
    this.editor.removeEventListener(SourceEditor.EVENTS.DIRTY_CHANGED,
                                    this._onDirtyChanged);
    this.editor.removeEventListener(SourceEditor.EVENTS.CONTEXT_MENU,
                                    this.onContextMenu);
    this.editor.destroy();
    this.editor = null;
    this.initialized = false;
  },

  












  promptSave: function SP_promptSave(aCallback)
  {
    if (this.filename && this.editor.dirty) {
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
        this.saveFile(function(aStatus) {
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

  





  onClose: function SP_onClose(aEvent)
  {
    if (this._skipClosePrompt) {
      return;
    }

    this.promptSave(function(aShouldClose, aSaved, aStatus) {
      let shouldClose = aShouldClose;
      if (aSaved && !Components.isSuccessCode(aStatus)) {
        shouldClose = false;
      }

      if (shouldClose) {
        this._skipClosePrompt = true;
        window.close();
      }
    }.bind(this));
    aEvent.preventDefault();
  },

  






  close: function SP_close(aCallback)
  {
    this.promptSave(function(aShouldClose, aSaved, aStatus) {
      let shouldClose = aShouldClose;
      if (aSaved && !Components.isSuccessCode(aStatus)) {
        shouldClose = false;
      }

      if (shouldClose) {
        this._skipClosePrompt = true;
        window.close();
      }
      if (aCallback) {
        aCallback();
      }
    }.bind(this));
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

XPCOMUtils.defineLazyGetter(Scratchpad, "strings", function () {
  return Services.strings.createBundle(SCRATCHPAD_L10N);
});

addEventListener("load", Scratchpad.onLoad.bind(Scratchpad), false);
addEventListener("unload", Scratchpad.onUnload.bind(Scratchpad), false);
addEventListener("close", Scratchpad.onClose.bind(Scratchpad), false);
