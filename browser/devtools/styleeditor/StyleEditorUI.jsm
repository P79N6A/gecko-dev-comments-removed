




"use strict";

this.EXPORTED_SYMBOLS = ["StyleEditorUI"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/devtools/SplitView.jsm");
Cu.import("resource:///modules/devtools/StyleSheetEditor.jsm");
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const { PrefObserver, PREF_ORIG_SOURCES } = require("devtools/styleeditor/utils");
const csscoverage = require("devtools/server/actors/csscoverage");
const console = require("resource://gre/modules/devtools/Console.jsm").console;

const LOAD_ERROR = "error-load";
const STYLE_EDITOR_TEMPLATE = "stylesheet";

















function StyleEditorUI(debuggee, target, panelDoc) {
  EventEmitter.decorate(this);

  this._debuggee = debuggee;
  this._target = target;
  this._panelDoc = panelDoc;
  this._window = this._panelDoc.defaultView;
  this._root = this._panelDoc.getElementById("style-editor-chrome");

  this.editors = [];
  this.selectedEditor = null;
  this.savedLocations = {};

  this._updateSourcesLabel = this._updateSourcesLabel.bind(this);
  this._onStyleSheetCreated = this._onStyleSheetCreated.bind(this);
  this._onNewDocument = this._onNewDocument.bind(this);
  this._clear = this._clear.bind(this);
  this._onError = this._onError.bind(this);

  this._prefObserver = new PrefObserver("devtools.styleeditor.");
  this._prefObserver.on(PREF_ORIG_SOURCES, this._onNewDocument);
}

StyleEditorUI.prototype = {
  




  get isDirty() {
    if (this._markedDirty === true) {
      return true;
    }
    return this.editors.some((editor) => {
      return editor.sourceEditor && !editor.sourceEditor.isClean();
    });
  },

  


  set isDirty(value) {
    this._markedDirty = value;
  },

  


  get selectedStyleSheetIndex() {
    return this.selectedEditor ?
           this.selectedEditor.styleSheet.styleSheetIndex : -1;
  },

  



  initialize: function() {
    let toolbox = gDevTools.getToolbox(this._target);
    return toolbox.initInspector().then(() => {
      this._walker = toolbox.walker;
    }).then(() => {
      this.createUI();
      this._debuggee.getStyleSheets().then((styleSheets) => {
        this._resetStyleSheetList(styleSheets);

        this._target.on("will-navigate", this._clear);
        this._target.on("navigate", this._onNewDocument);
      });
    });
  },

  


  createUI: function() {
    let viewRoot = this._root.parentNode.querySelector(".splitview-root");

    this._view = new SplitView(viewRoot);

    wire(this._view.rootElement, ".style-editor-newButton", function onNew() {
      this._debuggee.addStyleSheet(null).then(this._onStyleSheetCreated);
    }.bind(this));

    wire(this._view.rootElement, ".style-editor-importButton", function onImport() {
      this._importFromFile(this._mockImportFile || null, this._window);
    }.bind(this));

    this._contextMenu = this._panelDoc.getElementById("sidebar-context");
    this._contextMenu.addEventListener("popupshowing",
                                       this._updateSourcesLabel);

    this._sourcesItem = this._panelDoc.getElementById("context-origsources");
    this._sourcesItem.addEventListener("command",
                                       this._toggleOrigSources);
  },

  



  _updateSourcesLabel: function() {
    let string = "showOriginalSources";
    if (Services.prefs.getBoolPref(PREF_ORIG_SOURCES)) {
      string = "showCSSSources";
    }
    this._sourcesItem.setAttribute("label", _(string + ".label"));
    this._sourcesItem.setAttribute("accesskey", _(string + ".accesskey"));
  },

  







  _onNewDocument: function() {
    this._debuggee.getStyleSheets().then((styleSheets) => {
      this._resetStyleSheetList(styleSheets);
    })
  },

  





  _resetStyleSheetList: function(styleSheets) {
    this._clear();

    for (let sheet of styleSheets) {
      this._addStyleSheet(sheet);
    }

    this._root.classList.remove("loading");

    this.emit("stylesheets-reset");
  },

  


  _clear: function() {
    
    if (this.selectedEditor && this.selectedEditor.sourceEditor) {
      let href = this.selectedEditor.styleSheet.href;
      let {line, ch} = this.selectedEditor.sourceEditor.getCursor();

      this._styleSheetToSelect = {
        href: href,
        line: line,
        col: ch
      };
    }

    
    for (let editor of this.editors) {
      if (editor.savedFile) {
        let identifier = this.getStyleSheetIdentifier(editor.styleSheet);
        this.savedLocations[identifier] = editor.savedFile;
      }
    }

    this._clearStyleSheetEditors();
    this._view.removeAll();

    this.selectedEditor = null;

    this._root.classList.add("loading");
  },

  






  _addStyleSheet: function(styleSheet) {
    let editor = this._addStyleSheetEditor(styleSheet);

    if (!Services.prefs.getBoolPref(PREF_ORIG_SOURCES)) {
      return;
    }

    styleSheet.getOriginalSources().then((sources) => {
      if (sources && sources.length) {
        this._removeStyleSheetEditor(editor);
        sources.forEach((source) => {
          
          source.styleSheetIndex = styleSheet.styleSheetIndex;
          source.relatedStyleSheet = styleSheet;

          this._addStyleSheetEditor(source);
        });
      }
    });
  },

  









  _addStyleSheetEditor: function(styleSheet, file, isNew) {
    
    let identifier = this.getStyleSheetIdentifier(styleSheet);
    let savedFile = this.savedLocations[identifier];
    if (savedFile && !file) {
      file = savedFile;
    }

    let editor =
      new StyleSheetEditor(styleSheet, this._window, file, isNew, this._walker);

    editor.on("property-change", this._summaryChange.bind(this, editor));
    editor.on("linked-css-file", this._summaryChange.bind(this, editor));
    editor.on("linked-css-file-error", this._summaryChange.bind(this, editor));
    editor.on("error", this._onError);

    this.editors.push(editor);

    editor.fetchSource(this._sourceLoaded.bind(this, editor));
    return editor;
  },

  









  _importFromFile: function(file, parentWindow) {
    let onFileSelected = function(file) {
      if (!file) {
        
        return;
      }
      NetUtil.asyncFetch(file, (stream, status) => {
        if (!Components.isSuccessCode(status)) {
          this.emit("error", LOAD_ERROR);
          return;
        }
        let source = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();

        this._debuggee.addStyleSheet(source).then((styleSheet) => {
          this._onStyleSheetCreated(styleSheet, file);
        });
      });

    }.bind(this);

    showFilePicker(file, false, parentWindow, onFileSelected);
  },


  



  _onStyleSheetCreated: function(styleSheet, file) {
    this._addStyleSheetEditor(styleSheet, file, true);
  },

  









  _onError: function(event, errorCode, message) {
    this.emit("error", errorCode, message);
  },

  


  _toggleOrigSources: function() {
    let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    Services.prefs.setBoolPref(PREF_ORIG_SOURCES, !isEnabled);
  },

  





  _removeStyleSheetEditor: function(editor) {
    if (editor.summary) {
      this._view.removeItem(editor.summary);
    }
    else {
      let self = this;
      this.on("editor-added", function onAdd(event, added) {
        if (editor == added) {
          self.off("editor-added", onAdd);
          self._view.removeItem(editor.summary);
        }
      })
    }

    editor.destroy();
    this.editors.splice(this.editors.indexOf(editor), 1);
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

        Task.spawn(function* () {
          
          if (editor.isNew) {
            yield this._selectEditor(editor);
          }

          if (this._styleSheetToSelect
              && this._styleSheetToSelect.href == editor.styleSheet.href) {
            yield this.switchToSelectedSheet();
          }

          
          
          if (!this.selectedEditor && !this._styleSheetBoundToSelect
              && editor.styleSheet.styleSheetIndex == 0) {
            yield this._selectEditor(editor);
          }

          this.emit("editor-added", editor);
        }.bind(this)).then(null, Cu.reportError);
      }.bind(this),

      onShow: function(summary, details, data) {
        let editor = data.editor;
        this.selectedEditor = editor;

        Task.spawn(function* () {
          if (!editor.sourceEditor) {
            
            let inputElement = details.querySelector(".stylesheet-editor-input");
            yield editor.load(inputElement);
          }

          editor.onShow();

          csscoverage.getUsage(this._target).then(usage => {
            let href = editor.styleSheet.href || editor.styleSheet.nodeHref;
            usage.createEditorReport(href).then(data => {
              editor.removeAllUnusedRegions();
              editor.addUnusedRegions(data.reports);
            });
          }, console.error);

          this.emit("editor-selected", editor);
        }.bind(this)).then(null, Cu.reportError);
      }.bind(this)
    });
  },

  





  switchToSelectedSheet: function() {
    let sheet = this._styleSheetToSelect;

    for (let editor of this.editors) {
      if (editor.styleSheet.href == sheet.href) {
        
        
        
        
        this._styleSheetBoundToSelect = this._styleSheetToSelect;
        this._styleSheetToSelect = null;
        return this._selectEditor(editor, sheet.line, sheet.col);
      }
    }

    return promise.resolve();
  },

  











  _selectEditor: function(editor, line, col) {
    line = line || 0;
    col = col || 0;

    let editorPromise = editor.getSourceEditor().then(() => {
      editor.sourceEditor.setCursor({line: line, ch: col});
      this._styleSheetBoundToSelect = null;
    });

    let summaryPromise = this.getEditorSummary(editor).then((summary) => {
      this._view.activeSummary = summary;
    });

    return promise.all([editorPromise, summaryPromise]);
  },

  getEditorSummary: function(editor) {
    if (editor.summary) {
      return promise.resolve(editor.summary);
    }

    let deferred = promise.defer();
    let self = this;

    this.on("editor-added", function onAdd(e, selected) {
      if (selected == editor) {
        self.off("editor-added", onAdd);
        deferred.resolve(editor.summary);
      }
    });

    return deferred.promise;
  },

  





  getStyleSheetIdentifier: function (aStyleSheet) {
    
    return aStyleSheet.href ? aStyleSheet.href :
            "inline-" + aStyleSheet.styleSheetIndex + "-at-" + aStyleSheet.nodeHref;
  },

  












  selectStyleSheet: function(href, line, col) {
    this._styleSheetToSelect = {
      href: href,
      line: line,
      col: col,
    };

    

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

    let ruleCount = editor.styleSheet.ruleCount;
    if (editor.styleSheet.relatedStyleSheet && editor.linkedCSSFile) {
      ruleCount = editor.styleSheet.relatedStyleSheet.ruleCount;
    }
    if (ruleCount === undefined) {
      ruleCount = "-";
    }

    var flags = [];
    if (editor.styleSheet.disabled) {
      flags.push("disabled");
    }
    if (editor.unsaved) {
      flags.push("unsaved");
    }
    if (editor.linkedCSSFileError) {
      flags.push("linked-file-error");
    }
    this._view.setItemClassName(summary, flags.join(" "));

    let label = summary.querySelector(".stylesheet-name > label");
    label.setAttribute("value", editor.friendlyName);
    if (editor.styleSheet.href) {
      label.setAttribute("tooltiptext", editor.styleSheet.href);
    }

    let linkedCSSFile = "";
    if (editor.linkedCSSFile) {
      linkedCSSFile = OS.Path.basename(editor.linkedCSSFile);
    }
    text(summary, ".stylesheet-linked-file", linkedCSSFile);
    text(summary, ".stylesheet-title", editor.styleSheet.title || "");
    text(summary, ".stylesheet-rule-count",
      PluralForm.get(ruleCount, _("ruleCount.label")).replace("#1", ruleCount));
  },

  destroy: function() {
    this._clearStyleSheetEditors();

    this._prefObserver.off(PREF_ORIG_SOURCES, this._onNewDocument);
    this._prefObserver.destroy();
  }
}
