




"use strict";

this.EXPORTED_SYMBOLS = ["StyleEditorChrome"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource:///modules/devtools/StyleEditor.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/devtools/SplitView.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

const STYLE_EDITOR_TEMPLATE = "stylesheet";














this.StyleEditorChrome = function StyleEditorChrome(aRoot, aContentWindow)
{
  assert(aRoot, "Argument 'aRoot' is required to initialize StyleEditorChrome.");

  this._root = aRoot;
  this._document = this._root.ownerDocument;
  this._window = this._document.defaultView;

  this._editors = [];
  this._listeners = []; 

  
  
  this._contentWindowTemp = aContentWindow;

  this._contentWindow = null;
}

StyleEditorChrome.prototype = {
  _styleSheetToSelect: null,

  open: function() {
    let deferred = Promise.defer();
    let initializeUI = function (aEvent) {
      if (aEvent) {
        this._window.removeEventListener("load", initializeUI, false);
      }
      let viewRoot = this._root.parentNode.querySelector(".splitview-root");
      this._view = new SplitView(viewRoot);
      this._setupChrome();

      
      
      this.contentWindow = this._contentWindowTemp; 
      this._contentWindowTemp = null;

      deferred.resolve();
    }.bind(this);

    if (this._document.readyState == "complete") {
      initializeUI();
    } else {
      this._window.addEventListener("load", initializeUI, false);
    }

    return deferred.promise;
  },

  





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
      this._root.classList.remove("loading");
      this._populateChrome();
      return;
    } else {
      this._root.classList.add("loading");
      let onContentReady = function () {
        aContentWindow.removeEventListener("load", onContentReady, false);
        this._root.classList.remove("loading");
        this._populateChrome();
      }.bind(this);
      aContentWindow.addEventListener("load", onContentReady, false);
    }
  },

  




  get contentDocument()
  {
    return this._contentWindow ? this._contentWindow.document : null;
  },

  





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

  




  get isDirty()
  {
    if (this._markedDirty === true) {
      return true;
    }
    return this.editors.some(function(editor) {
      return editor.sourceEditor && editor.sourceEditor.dirty;
    });
  },

  


  markDirty: function SEC_markDirty() {
    this._markedDirty = true;
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

    
    let listeners = this._listeners.concat();
    
    for (let i = 0; i < listeners.length; i++) {
      let listener = listeners[i];
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

  


  resetChrome: function SEC__resetChrome()
  {
    this._editors.forEach(function (aEditor) {
      aEditor.removeActionListener(this);
    }.bind(this));
    this._editors = [];

    this._view.removeAll();

    
    let matches = this._root.querySelectorAll("toolbarbutton,input,select");
    for (let i = 0; i < matches.length; i++) {
      matches[i].removeAttribute("disabled");
    }
  },

  




  _populateChrome: function SEC__populateChrome()
  {
    this.resetChrome();

    let document = this.contentDocument;
    this._document.title = _("chromeWindowTitle",
      document.title || document.location.href);

    for (let i = 0; i < document.styleSheets.length; i++) {
      let styleSheet = document.styleSheets[i];

      let editor = new StyleEditor(document, styleSheet);
      editor.addActionListener(this);
      this._editors.push(editor);
    }

    
    
    
    
    this._editors.forEach(function (aEditor) {
      this._window.setTimeout(aEditor.load.bind(aEditor), 0);
    }, this);
  },

  












  selectStyleSheet: function SEC_selectSheet(aSheet, aLine, aCol)
  {
    let alreadyCalled = !!this._styleSheetToSelect;

    this._styleSheetToSelect = {
      sheet: aSheet,
      line: aLine,
      col: aCol,
    };

    if (alreadyCalled) {
      return;
    }

    let select = function DEC_select(aEditor) {
      let sheet = this._styleSheetToSelect.sheet;
      let line = this._styleSheetToSelect.line || 1;
      let col = this._styleSheetToSelect.col || 1;

      if (!aEditor.sourceEditor) {
        let onAttach = function SEC_selectSheet_onAttach() {
          aEditor.removeActionListener(this);
          this.selectedStyleSheetIndex = aEditor.styleSheetIndex;
          aEditor.sourceEditor.setCaretPosition(line - 1, col - 1);

          let newSheet = this._styleSheetToSelect.sheet;
          let newLine = this._styleSheetToSelect.line;
          let newCol = this._styleSheetToSelect.col;
          this._styleSheetToSelect = null;
          if (newSheet != sheet) {
              this.selectStyleSheet.bind(this, newSheet, newLine, newCol);
          }
        }.bind(this);

        aEditor.addActionListener({
          onAttach: onAttach
        });
      } else {
        
        aEditor.sourceEditor.setCaretPosition(line - 1, col - 1);
        this._styleSheetToSelect = null;
      }

        let summary = sheet ? this.getSummaryElementForEditor(aEditor)
                            : this._view.getSummaryElementByOrdinal(0);
        this._view.activeSummary = summary;
      this.selectedStyleSheetIndex = aEditor.styleSheetIndex;
    }.bind(this);

    if (!this.editors.length) {
      
      
      
      let self = this;
      this.addChromeListener({
        onEditorAdded: function SEC_selectSheet_onEditorAdded(aChrome, aEditor) {
          let sheet = self._styleSheetToSelect.sheet;
          if ((sheet && aEditor.styleSheet == sheet) ||
              (aEditor.styleSheetIndex == 0 && sheet == null)) {
            aChrome.removeChromeListener(this);
            aEditor.addActionListener(self);
            select(aEditor);
          }
        }
      });
    } else if (aSheet) {
      
      
      
      for each (let editor in this.editors) {
        if (editor.styleSheet == aSheet) {
          select(editor);
          break;
        }
      }
    }
  },

  





  _disableChrome: function SEC__disableChrome()
  {
    let matches = this._root.querySelectorAll("button,toolbarbutton,textbox");
    for (let i = 0; i < matches.length; i++) {
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

    let label = summary.querySelector(".stylesheet-name > label");
    label.setAttribute("value", aEditor.getFriendlyName());

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

        aSummary.addEventListener("focus", function onSummaryFocus(aEvent) {
          if (aEvent.target == aSummary) {
            
            aSummary.querySelector(".stylesheet-name").focus();
          }
        }, false);

        
        if (editor.hasFlag(StyleEditorFlags.NEW)) {
          this._view.activeSummary = aSummary;
        }

        this._triggerChromeListeners("EditorAdded", [editor]);
      }.bind(this),
      onHide: function ASV_onItemShow(aSummary, aDetails, aData) {
        aData.editor.onHide();
      },
      onShow: function ASV_onItemShow(aSummary, aDetails, aData) {
        let editor = aData.editor;
        if (!editor.inputElement) {
          
          editor.inputElement = aDetails.querySelector(".stylesheet-editor-input");
        }
        editor.onShow();
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
