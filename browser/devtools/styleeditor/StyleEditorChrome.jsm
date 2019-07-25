




































"use strict";

const EXPORTED_SYMBOLS = ["StyleEditorChrome"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource:///modules/devtools/StyleEditor.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/devtools/SplitView.jsm");

const STYLE_EDITOR_TEMPLATE = "stylesheet";














function StyleEditorChrome(aRoot, aContentWindow)
{
  assert(aRoot, "Argument 'aRoot' is required to initialize StyleEditorChrome.");

  this._root = aRoot;
  this._document = this._root.ownerDocument;
  this._window = this._document.defaultView;

  this._editors = [];
  this._listeners = []; 

  this._contentWindow = null;
  this._isContentAttached = false;

  let initializeUI = function (aEvent) {
    if (aEvent) {
      this._window.removeEventListener("load", initializeUI, false);
    }

    let viewRoot = this._root.parentNode.querySelector(".splitview-root");
    this._view = new SplitView(viewRoot);

    this._setupChrome();

    
    this.contentWindow = aContentWindow;
    this._contentWindowID = null;
  }.bind(this);

  if (this._document.readyState == "complete") {
    initializeUI();
  } else {
    this._window.addEventListener("load", initializeUI, false);
  }
}

StyleEditorChrome.prototype = {
  





  get contentWindow() this._contentWindow,

  





  get contentWindowID()
  {
    try {
      return this._contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
    } catch (ex) {
      return -1;
    }
  },

  








  set contentWindow(aContentWindow)
  {
    if (this._contentWindow == aContentWindow) {
      return; 
    }

    this._contentWindow = aContentWindow;

    if (!aContentWindow) {
      this._disableChrome();
      return;
    }

    let onContentUnload = function () {
      aContentWindow.removeEventListener("unload", onContentUnload, false);
      if (this.contentWindow == aContentWindow) {
        this.contentWindow = null; 
      }
    }.bind(this);
    aContentWindow.addEventListener("unload", onContentUnload, false);

    if (aContentWindow.document.readyState == "complete") {
      this._populateChrome();
      return;
    } else {
      let onContentReady = function () {
        aContentWindow.removeEventListener("load", onContentReady, false);
        this._populateChrome();
      }.bind(this);
      aContentWindow.addEventListener("load", onContentReady, false);
    }
  },

  




  get contentDocument()
  {
    return this._contentWindow ? this._contentWindow.document : null;
  },

  






  get isContentAttached() this._isContentAttached,

  





  get editors()
  {
    let editors = [];
    this._editors.forEach(function (aEditor) {
      if (aEditor.styleSheetIndex >= 0) {
        editors[aEditor.styleSheetIndex] = aEditor;
      }
    });
    return editors;
  },

  


























  addChromeListener: function SEC_addChromeListener(aListener)
  {
    this._listeners.push(aListener);
  },

  





  removeChromeListener: function SEC_removeChromeListener(aListener)
  {
    let index = this._listeners.indexOf(aListener);
    if (index != -1) {
      this._listeners.splice(index, 1);
    }
  },

  








  _triggerChromeListeners: function SE__triggerChromeListeners(aName, aArgs)
  {
    
    if (!aArgs) {
      aArgs = [this];
    } else {
      aArgs.unshift(this);
    }

    
    for (let i = 0; i < this._listeners.length; ++i) {
      let listener = this._listeners[i];
      let handler = listener["on" + aName];
      if (handler) {
        handler.apply(listener, aArgs);
      }
    }
  },

  


  _setupChrome: function SEC__setupChrome()
  {
    
    wire(this._view.rootElement, ".style-editor-newButton", function onNewButton() {
      let editor = new StyleEditor(this.contentDocument);
      this._editors.push(editor);
      editor.addActionListener(this);
      editor.load();
    }.bind(this));

    wire(this._view.rootElement, ".style-editor-importButton", function onImportButton() {
      let editor = new StyleEditor(this.contentDocument);
      this._editors.push(editor);
      editor.addActionListener(this);
      editor.importFromFile(this._mockImportFile || null, this._window);
    }.bind(this));
  },

  


  _resetChrome: function SEC__resetChrome()
  {
    this._editors.forEach(function (aEditor) {
      aEditor.removeActionListener(this);
    }.bind(this));
    this._editors = [];

    this._view.removeAll();
  },

  




  _populateChrome: function SEC__populateChrome()
  {
    this._resetChrome();

    this._document.title = _("chromeWindowTitle",
          this.contentDocument.title || this.contentDocument.location.href);

    let document = this.contentDocument;
    for (let i = 0; i < document.styleSheets.length; ++i) {
      let styleSheet = document.styleSheets[i];

      let editor = new StyleEditor(document, styleSheet);
      editor.addActionListener(this);
      this._editors.push(editor);
    }

    this._triggerChromeListeners("ContentAttach");

    
    
    
    
    this._editors.forEach(function (aEditor) {
      this._window.setTimeout(aEditor.load.bind(aEditor), 0);
    }, this);
  },

  





  _disableChrome: function SEC__disableChrome()
  {
    let matches = this._root.querySelectorAll("button,input,select");
    for (let i = 0; i < matches.length; ++i) {
      matches[i].setAttribute("disabled", "disabled");
    }

    this.editors.forEach(function onEnterReadOnlyMode(aEditor) {
      aEditor.readOnly = true;
    });

    this._view.rootElement.setAttribute("disabled", "disabled");

    this._triggerChromeListeners("ContentDetach");
  },

  







  getSummaryElementForEditor: function SEC_getSummaryElementForEditor(aEditor)
  {
    return this._view.getSummaryElementByOrdinal(aEditor.styleSheetIndex);
  },

  







  _updateSummaryForEditor: function SEC__updateSummaryForEditor(aEditor, aSummary)
  {
    let summary = aSummary || this.getSummaryElementForEditor(aEditor);
    let ruleCount = aEditor.styleSheet.cssRules.length;

    this._view.setItemClassName(summary, aEditor.flags);

    text(summary, ".stylesheet-name", aEditor.getFriendlyName());
    text(summary, ".stylesheet-title", aEditor.styleSheet.title || "");
    text(summary, ".stylesheet-rule-count",
      PluralForm.get(ruleCount, _("ruleCount.label")).replace("#1", ruleCount));
    text(summary, ".stylesheet-error-message", aEditor.errorMessage);
  },

  




  




  onLoad: function SEAL_onLoad(aEditor)
  {
    let item = this._view.appendTemplatedItem(STYLE_EDITOR_TEMPLATE, {
      data: {
        editor: aEditor
      },
      disableAnimations: this._alwaysDisableAnimations,
      ordinal: aEditor.styleSheetIndex,
      onCreate: function ASV_onItemCreate(aSummary, aDetails, aData) {
        let editor = aData.editor;

        wire(aSummary, ".stylesheet-enabled", function onToggleEnabled(aEvent) {
          aEvent.stopPropagation();
          aEvent.target.blur();

          editor.enableStyleSheet(editor.styleSheet.disabled);
        });

        wire(aSummary, ".stylesheet-saveButton", function onSaveButton(aEvent) {
          aEvent.stopPropagation();
          aEvent.target.blur();

          editor.saveToFile(editor.savedFile);
        });

        this._updateSummaryForEditor(editor, aSummary);

        
        if (editor.styleSheetIndex == 0 ||
            editor.hasFlag(StyleEditorFlags.NEW)) {
          this._view.activeSummary = aSummary;
        }

        aSummary.addEventListener("focus", function onSummaryFocus(aEvent) {
          if (aEvent.target == aSummary) {
            
            aSummary.querySelector(".stylesheet-name").focus();
          }
        }, false);

        this._triggerChromeListeners("EditorAdded", [editor]);
      }.bind(this),
      onShow: function ASV_onItemShow(aSummary, aDetails, aData) {
        let editor = aData.editor;
        if (!editor.inputElement) {
          
          editor.inputElement = aDetails.querySelector(".stylesheet-editor-input");
        }
        editor.inputElement.focus();
      }
    });
  },

  






  onFlagChange: function SEAL_onFlagChange(aEditor, aFlagName)
  {
    this._updateSummaryForEditor(aEditor);
  },

  





  onCommit: function SEAL_onCommit(aEditor)
  {
    this._updateSummaryForEditor(aEditor);
  },
};
