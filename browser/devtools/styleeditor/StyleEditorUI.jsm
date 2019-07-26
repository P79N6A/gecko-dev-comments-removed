




"use strict";

this.EXPORTED_SYMBOLS = ["StyleEditorUI"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/devtools/SplitView.jsm");
Cu.import("resource:///modules/devtools/StyleSheetEditor.jsm");


const LOAD_ERROR = "error-load";

const STYLE_EDITOR_TEMPLATE = "stylesheet";














function StyleEditorUI(debuggee, panelDoc) {
  EventEmitter.decorate(this);

  this._debuggee = debuggee;
  this._panelDoc = panelDoc;
  this._window = this._panelDoc.defaultView;
  this._root = this._panelDoc.getElementById("style-editor-chrome");

  this.editors = [];
  this.selectedStyleSheetIndex = -1;

  this._onStyleSheetAdded = this._onStyleSheetAdded.bind(this);
  this._onStyleSheetCreated = this._onStyleSheetCreated.bind(this);
  this._onStyleSheetsCleared = this._onStyleSheetsCleared.bind(this);
  this._onError = this._onError.bind(this);

  debuggee.on("stylesheet-added", this._onStyleSheetAdded);
  debuggee.on("stylesheets-cleared", this._onStyleSheetsCleared);

  this.createUI();
}

StyleEditorUI.prototype = {
  




  get isDirty()
  {
    if (this._markedDirty === true) {
      return true;
    }
    return this.editors.some((editor) => {
      return editor.sourceEditor && editor.sourceEditor.dirty;
    });
  },

  


  set isDirty(value) {
    this._markedDirty = value;
  },

  


  createUI: function() {
    let viewRoot = this._root.parentNode.querySelector(".splitview-root");

    this._view = new SplitView(viewRoot);

    wire(this._view.rootElement, ".style-editor-newButton", function onNew() {
      this._debuggee.createStyleSheet(null, this._onStyleSheetCreated);
    }.bind(this));

    wire(this._view.rootElement, ".style-editor-importButton", function onImport() {
      this._importFromFile(this._mockImportFile || null, this._window);
    }.bind(this));
  },

  









  _importFromFile: function(file, parentWindow)
  {
    let onFileSelected = function(file) {
      if (!file) {
        this.emit("error", LOAD_ERROR);
        return;
      }
      NetUtil.asyncFetch(file, (stream, status) => {
        if (!Components.isSuccessCode(status)) {
          this.emit("error", LOAD_ERROR);
          return;
        }
        let source = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();

        this._debuggee.createStyleSheet(source, (styleSheet) => {
          this._onStyleSheetCreated(styleSheet, file);
        });
      });

    }.bind(this);

    showFilePicker(file, false, parentWindow, onFileSelected);
  },

  


  _onStyleSheetsCleared: function() {
    this._clearStyleSheetEditors();

    this._view.removeAll();
    this.selectedStyleSheetIndex = -1;

    this._root.classList.add("loading");
  },

  



  _onStyleSheetCreated: function(styleSheet, file) {
    this._addStyleSheetEditor(styleSheet, file, true);
  },

  







  _onStyleSheetAdded: function(event, styleSheet) {
    
    this._root.classList.remove("loading");
    this._addStyleSheetEditor(styleSheet);
  },

  







  _onError: function(event, errorCode) {
    this.emit("error", errorCode);
  },

  









  _addStyleSheetEditor: function(styleSheet, file, isNew) {
    let editor = new StyleSheetEditor(styleSheet, this._window, file, isNew);

    editor.once("source-load", this._sourceLoaded.bind(this, editor));
    editor.on("property-change", this._summaryChange.bind(this, editor));
    editor.on("style-applied", this._summaryChange.bind(this, editor));
    editor.on("error", this._onError);

    this.editors.push(editor);

    
    
    this._window.setTimeout(editor.fetchSource.bind(editor), 0);
  },

  


  _clearStyleSheetEditors: function() {
    for (let editor of this.editors) {
      editor.destroy();
    }
    this.editors = [];
  },

  






  _sourceLoaded: function(editor) {
    
    this._view.appendTemplatedItem(STYLE_EDITOR_TEMPLATE, {
      data: {
        editor: editor
      },
      disableAnimations: this._alwaysDisableAnimations,
      ordinal: editor.styleSheet.styleSheetIndex,
      onCreate: function(summary, details, data) {
        let editor = data.editor;
        editor.summary = summary;

        wire(summary, ".stylesheet-enabled", function onToggleDisabled(event) {
          event.stopPropagation();
          event.target.blur();

          editor.toggleDisabled();
        });

        wire(summary, ".stylesheet-name", {
          events: {
            "keypress": function onStylesheetNameActivate(aEvent) {
              if (aEvent.keyCode == aEvent.DOM_VK_RETURN) {
                this._view.activeSummary = summary;
              }
            }.bind(this)
          }
        });

        wire(summary, ".stylesheet-saveButton", function onSaveButton(event) {
          event.stopPropagation();
          event.target.blur();

          editor.saveToFile(editor.savedFile);
        });

        this._updateSummaryForEditor(editor, summary);

        summary.addEventListener("focus", function onSummaryFocus(event) {
          if (event.target == summary) {
            
            summary.querySelector(".stylesheet-name").focus();
          }
        }, false);

        
        if (editor.isNew) {
          this._selectEditor(editor);
        }

        if (this._styleSheetToSelect
            && this._styleSheetToSelect.href == editor.styleSheet.href) {
          this.switchToSelectedSheet();
        }

        
        if (this.selectedStyleSheetIndex == -1
            && !this._styleSheetToSelect
            && editor.styleSheet.styleSheetIndex == 0) {
          this._selectEditor(editor);
        }

        this.emit("editor-added", editor);
      }.bind(this),

      onShow: function(summary, details, data) {
        let editor = data.editor;
        if (!editor.sourceEditor) {
          
          let inputElement = details.querySelector(".stylesheet-editor-input");
          editor.load(inputElement);
        }
        editor.onShow();
      }
    });
  },

  


  switchToSelectedSheet: function() {
    let sheet = this._styleSheetToSelect;

    for each (let editor in this.editors) {
      if (editor.styleSheet.href == sheet.href) {
        this._selectEditor(editor, sheet.line, sheet.col);
        this._styleSheetToSelect = null;
        break;
      }
    }
  },

  









  _selectEditor: function(editor, line, col) {
    line = line || 1;
    col = col || 1;

    this.selectedStyleSheetIndex = editor.styleSheet.styleSheetIndex;

    editor.getSourceEditor().then(() => {
      editor.sourceEditor.setCaretPosition(line - 1, col - 1);
    });

    this._view.activeSummary = editor.summary;
  },

  












  selectStyleSheet: function(href, line, col)
  {
    let alreadyCalled = !!this._styleSheetToSelect;

    this._styleSheetToSelect = {
      href: href,
      line: line,
      col: col,
    };

    if (alreadyCalled) {
      return;
    }

    

    this.switchToSelectedSheet();
  },


  






  _summaryChange: function(editor) {
    this._updateSummaryForEditor(editor);
  },

  







  _updateSummaryForEditor: function(editor, summary) {
    summary = summary || editor.summary;
    if (!summary) {
      return;
    }
    let ruleCount = "-";
    if (editor.styleSheet.ruleCount !== undefined) {
      ruleCount = editor.styleSheet.ruleCount;
    }

    var flags = [];
    if (editor.styleSheet.disabled) {
      flags.push("disabled");
    }
    if (editor.unsaved) {
      flags.push("unsaved");
    }
    this._view.setItemClassName(summary, flags.join(" "));

    let label = summary.querySelector(".stylesheet-name > label");
    label.setAttribute("value", editor.friendlyName);

    text(summary, ".stylesheet-title", editor.styleSheet.title || "");
    text(summary, ".stylesheet-rule-count",
      PluralForm.get(ruleCount, _("ruleCount.label")).replace("#1", ruleCount));
    text(summary, ".stylesheet-error-message", editor.errorMessage);
  },

  destroy: function() {
    this._clearStyleSheetEditors();

    this._debuggee.off("stylesheet-added", this._onStyleSheetAdded);
    this._debuggee.off("stylesheets-cleared", this._onStyleSheetsCleared);
  }
}
