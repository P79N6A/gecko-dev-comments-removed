




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
const SELECTOR_HIGHLIGHTER_TYPE = "SelectorHighlighter";
const PREF_MEDIA_SIDEBAR = "devtools.styleeditor.showMediaSidebar";
const PREF_SIDEBAR_WIDTH = "devtools.styleeditor.mediaSidebarWidth";
const PREF_NAV_WIDTH = "devtools.styleeditor.navSidebarWidth";

















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

  this._onOptionsPopupShowing = this._onOptionsPopupShowing.bind(this);
  this._onOptionsPopupHiding = this._onOptionsPopupHiding.bind(this);
  this._onStyleSheetCreated = this._onStyleSheetCreated.bind(this);
  this._onNewDocument = this._onNewDocument.bind(this);
  this._onMediaPrefChanged = this._onMediaPrefChanged.bind(this);
  this._updateMediaList = this._updateMediaList.bind(this);
  this._clear = this._clear.bind(this);
  this._onError = this._onError.bind(this);
  this._updateOpenLinkItem = this._updateOpenLinkItem.bind(this);
  this._openLinkNewTab = this._openLinkNewTab.bind(this);

  this._prefObserver = new PrefObserver("devtools.styleeditor.");
  this._prefObserver.on(PREF_ORIG_SOURCES, this._onNewDocument);
  this._prefObserver.on(PREF_MEDIA_SIDEBAR, this._onMediaPrefChanged);
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

  



  initialize: Task.async(function* () {
    yield this.initializeHighlighter();

    this.createUI();

    let styleSheets = yield this._debuggee.getStyleSheets();
    yield this._resetStyleSheetList(styleSheets);

    this._target.on("will-navigate", this._clear);
    this._target.on("navigate", this._onNewDocument);
  }),

  initializeHighlighter: Task.async(function* () {
    let toolbox = gDevTools.getToolbox(this._target);
    yield toolbox.initInspector();
    this._walker = toolbox.walker;

    let hUtils = toolbox.highlighterUtils;
    if (hUtils.supportsCustomHighlighters()) {
      try {
        this._highlighter =
          yield hUtils.getHighlighterByType(SELECTOR_HIGHLIGHTER_TYPE);
      } catch (e) {
        
        
        
        console.warn("The selectorHighlighter couldn't be instantiated, " +
          "elements matching hovered selectors will not be highlighted");
      }
    }
  }),

  


  createUI: function() {
    let viewRoot = this._root.parentNode.querySelector(".splitview-root");

    this._view = new SplitView(viewRoot);

    wire(this._view.rootElement, ".style-editor-newButton", () =>{
      this._debuggee.addStyleSheet(null).then(this._onStyleSheetCreated);
    });

    wire(this._view.rootElement, ".style-editor-importButton", ()=> {
      this._importFromFile(this._mockImportFile || null, this._window);
    });

    this._optionsButton = this._panelDoc.getElementById("style-editor-options");
    this._panelDoc.addEventListener("contextmenu", () => {
      this._contextMenuStyleSheet = null;
    }, true);

    this._contextMenu = this._panelDoc.getElementById("sidebar-context");
    this._contextMenu.addEventListener("popupshowing",
                                       this._updateOpenLinkItem);

    this._optionsMenu = this._panelDoc.getElementById("style-editor-options-popup");
    this._optionsMenu.addEventListener("popupshowing",
                                       this._onOptionsPopupShowing);
    this._optionsMenu.addEventListener("popuphiding",
                                       this._onOptionsPopupHiding);

    this._sourcesItem = this._panelDoc.getElementById("options-origsources");
    this._sourcesItem.addEventListener("command",
                                       this._toggleOrigSources);

    this._mediaItem = this._panelDoc.getElementById("options-show-media");
    this._mediaItem.addEventListener("command",
                                     this._toggleMediaSidebar);

    this._openLinkNewTabItem = this._panelDoc.getElementById("context-openlinknewtab");
    this._openLinkNewTabItem.addEventListener("command",
                                              this._openLinkNewTab);

    let nav = this._panelDoc.querySelector(".splitview-controller");
    nav.setAttribute("width", Services.prefs.getIntPref(PREF_NAV_WIDTH));
  },

  



  _onOptionsPopupShowing: function() {
    this._optionsButton.setAttribute("open", "true");
    this._sourcesItem.setAttribute("checked",
      Services.prefs.getBoolPref(PREF_ORIG_SOURCES));
    this._mediaItem.setAttribute("checked",
      Services.prefs.getBoolPref(PREF_MEDIA_SIDEBAR));
  },

  


  _onOptionsPopupHiding: function() {
    this._optionsButton.removeAttribute("open");
  },

  







  _onNewDocument: function() {
    this._debuggee.getStyleSheets().then((styleSheets) => {
      return this._resetStyleSheetList(styleSheets);
    }).then(null, Cu.reportError);
  },

  





  _resetStyleSheetList: Task.async(function* (styleSheets) {
    this._clear();

    for (let sheet of styleSheets) {
      yield this._addStyleSheet(sheet);
    }

    this._root.classList.remove("loading");

    this.emit("stylesheets-reset");
  }),

  


  _clear: function() {
    
    if (this.selectedEditor && this.selectedEditor.sourceEditor) {
      let href = this.selectedEditor.styleSheet.href;
      let {line, ch} = this.selectedEditor.sourceEditor.getCursor();

      this._styleSheetToSelect = {
        stylesheet: href,
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

  






  _addStyleSheet: Task.async(function* (styleSheet) {
    let editor = yield this._addStyleSheetEditor(styleSheet);

    if (!Services.prefs.getBoolPref(PREF_ORIG_SOURCES)) {
      return;
    }

    let sources = yield styleSheet.getOriginalSources();
    if (sources && sources.length) {
      this._removeStyleSheetEditor(editor);

      for (let source of sources) {
        
        source.styleSheetIndex = styleSheet.styleSheetIndex;
        source.relatedStyleSheet = styleSheet;

        yield this._addStyleSheetEditor(source);
      }
    }
  }),

  











  _addStyleSheetEditor: Task.async(function* (styleSheet, file, isNew) {
    
    let identifier = this.getStyleSheetIdentifier(styleSheet);
    let savedFile = this.savedLocations[identifier];
    if (savedFile && !file) {
      file = savedFile;
    }

    let editor = new StyleSheetEditor(styleSheet, this._window, file, isNew,
                                      this._walker, this._highlighter);

    editor.on("property-change", this._summaryChange.bind(this, editor));
    editor.on("media-rules-changed", this._updateMediaList.bind(this, editor));
    editor.on("linked-css-file", this._summaryChange.bind(this, editor));
    editor.on("linked-css-file-error", this._summaryChange.bind(this, editor));
    editor.on("error", this._onError);

    this.editors.push(editor);

    yield editor.fetchSource();
    this._sourceLoaded(editor);

    return editor;
  }),

  









  _importFromFile: function(file, parentWindow) {
    let onFileSelected = (file) => {
      if (!file) {
        
        return;
      }
      NetUtil.asyncFetch2(file, (stream, status) => {
        if (!Components.isSuccessCode(status)) {
          this.emit("error", { key: LOAD_ERROR });
          return;
        }
        let source = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();

        this._debuggee.addStyleSheet(source).then((styleSheet) => {
          this._onStyleSheetCreated(styleSheet, file);
        });
      },
      this._window.document,
      null,  
      null,  
      Ci.nsILoadInfo.SEC_NORMAL,
      Ci.nsIContentPolicy.TYPE_OTHER);
    };

    showFilePicker(file, false, parentWindow, onFileSelected);
  },


  



  _onStyleSheetCreated: function(styleSheet, file) {
    this._addStyleSheetEditor(styleSheet, file, true);
  },

  







  _onError: function(event, data) {
    this.emit("error", data);
  },

  


  _toggleOrigSources: function() {
    let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    Services.prefs.setBoolPref(PREF_ORIG_SOURCES, !isEnabled);
  },

  


  _toggleMediaSidebar: function() {
    let isEnabled = Services.prefs.getBoolPref(PREF_MEDIA_SIDEBAR);
    Services.prefs.setBoolPref(PREF_MEDIA_SIDEBAR, !isEnabled);
  },

  


  _onMediaPrefChanged: function() {
    this.editors.forEach(this._updateMediaList);
  },

  






  _updateOpenLinkItem: function() {
    this._openLinkNewTabItem.setAttribute("hidden", !this._contextMenuStyleSheet);
    if (this._contextMenuStyleSheet) {
      this._openLinkNewTabItem.setAttribute("disabled", !this._contextMenuStyleSheet.href);
    }
  },

  


  _openLinkNewTab: function() {
    if (this._contextMenuStyleSheet) {
      this._window.openUILinkIn(this._contextMenuStyleSheet.href, "tab");
    }
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
    let ordinal = editor.styleSheet.styleSheetIndex;
    ordinal = ordinal == -1 ? Number.MAX_SAFE_INTEGER : ordinal;
    
    this._view.appendTemplatedItem(STYLE_EDITOR_TEMPLATE, {
      data: {
        editor: editor
      },
      disableAnimations: this._alwaysDisableAnimations,
      ordinal: ordinal,
      onCreate: function(summary, details, data) {
        let editor = data.editor;
        editor.summary = summary;
        editor.details = details;

        wire(summary, ".stylesheet-enabled", function onToggleDisabled(event) {
          event.stopPropagation();
          event.target.blur();

          editor.toggleDisabled();
        });

        wire(summary, ".stylesheet-name", {
          events: {
            "keypress": (aEvent) => {
              if (aEvent.keyCode == aEvent.DOM_VK_RETURN) {
                this._view.activeSummary = summary;
              }
            }
          }
        });

        wire(summary, ".stylesheet-saveButton", function onSaveButton(event) {
          event.stopPropagation();
          event.target.blur();

          editor.saveToFile(editor.savedFile);
        });

        this._updateSummaryForEditor(editor, summary);

        summary.addEventListener("contextmenu", (event) => {
          this._contextMenuStyleSheet = editor.styleSheet;
        }, false);

        summary.addEventListener("focus", function onSummaryFocus(event) {
          if (event.target == summary) {
            
            summary.querySelector(".stylesheet-name").focus();
          }
        }, false);

        let sidebar = details.querySelector(".stylesheet-sidebar");
        sidebar.setAttribute("width",
            Services.prefs.getIntPref(PREF_SIDEBAR_WIDTH));

        let splitter = details.querySelector(".devtools-side-splitter");
        splitter.addEventListener("mousemove", () => {
          let sidebarWidth = sidebar.getAttribute("width");
          Services.prefs.setIntPref(PREF_SIDEBAR_WIDTH, sidebarWidth);

          
          let sidebars = [...this._panelDoc.querySelectorAll(".stylesheet-sidebar")];
          for (let mediaSidebar of sidebars) {
            mediaSidebar.setAttribute("width", sidebarWidth);
          }
        });

        
        if (editor.isNew) {
          this._selectEditor(editor);
        }

        if (this._isEditorToSelect(editor)) {
          this.switchToSelectedSheet();
        }

        
        
        if (!this.selectedEditor && !this._styleSheetBoundToSelect
            && editor.styleSheet.styleSheetIndex == 0) {
          this._selectEditor(editor);
        }
        this.emit("editor-added", editor);
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

          this.emit("editor-selected", editor);

          
          let usage = yield csscoverage.getUsage(this._target);
          if (usage == null) {
            return;
          }

          let href = csscoverage.sheetToUrl(editor.styleSheet);
          let data = yield usage.createEditorReport(href)

          editor.removeAllUnusedRegions();

          if (data.reports.length > 0) {
            
            
            let text = editor.sourceEditor.getText();
            let lineCount = text.split("\n").length;
            let ruleCount = editor.styleSheet.ruleCount;
            if (lineCount >= ruleCount) {
              editor.addUnusedRegions(data.reports);
            }
            else {
              this.emit("error", { key: "error-compressed", level: "info" });
            }
          }
        }.bind(this)).then(null, Cu.reportError);
      }.bind(this)
    });
  },

  





  switchToSelectedSheet: function() {
    let toSelect = this._styleSheetToSelect;

    for (let editor of this.editors) {
      if (this._isEditorToSelect(editor)) {
        
        
        
        
        this._styleSheetBoundToSelect = this._styleSheetToSelect;
        this._styleSheetToSelect = null;
        return this._selectEditor(editor, toSelect.line, toSelect.col);
      }
    }

    return promise.resolve();
  },

  






  _isEditorToSelect: function(editor) {
    let toSelect = this._styleSheetToSelect;
    if (!toSelect) {
      return false;
    }
    let isHref = toSelect.stylesheet === null ||
                 typeof toSelect.stylesheet == "string";

    return (isHref && editor.styleSheet.href == toSelect.stylesheet) ||
           (toSelect.stylesheet == editor.styleSheet);
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

  getEditorDetails: function(editor) {
    if (editor.details) {
      return promise.resolve(editor.details);
    }

    let deferred = promise.defer();
    let self = this;

    this.on("editor-added", function onAdd(e, selected) {
      if (selected == editor) {
        self.off("editor-added", onAdd);
        deferred.resolve(editor.details);
      }
    });

    return deferred.promise;
  },

  





  getStyleSheetIdentifier: function (aStyleSheet) {
    
    return aStyleSheet.href ? aStyleSheet.href :
            "inline-" + aStyleSheet.styleSheetIndex + "-at-" + aStyleSheet.nodeHref;
  },

  












  selectStyleSheet: function(stylesheet, line, col) {
    this._styleSheetToSelect = {
      stylesheet: stylesheet,
      line: line,
      col: col,
    };

    

    return this.switchToSelectedSheet();
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

  







  _updateMediaList: function(editor) {
    Task.spawn(function* () {
      let details = yield this.getEditorDetails(editor);
      let list = details.querySelector(".stylesheet-media-list");

      while (list.firstChild) {
        list.removeChild(list.firstChild);
      }

      let rules = editor.mediaRules;
      let showSidebar = Services.prefs.getBoolPref(PREF_MEDIA_SIDEBAR);
      let sidebar = details.querySelector(".stylesheet-sidebar");

      let inSource = false;

      for (let rule of rules) {
        let {line, column, parentStyleSheet} = rule;

        let location = {
          line: line,
          column: column,
          source: editor.styleSheet.href,
          styleSheet: parentStyleSheet
        };
        if (editor.styleSheet.isOriginalSource) {
          location = yield editor.cssSheet.getOriginalLocation(line, column);
        }

        
        if (location.source != editor.styleSheet.href) {
          continue;
        }
        inSource = true;

        let div = this._panelDoc.createElement("div");
        div.className = "media-rule-label";
        div.addEventListener("click", this._jumpToLocation.bind(this, location));

        let cond = this._panelDoc.createElement("div");
        cond.textContent = rule.conditionText;
        cond.className = "media-rule-condition"
        if (!rule.matches) {
          cond.classList.add("media-condition-unmatched");
        }
        div.appendChild(cond);

        let link = this._panelDoc.createElement("div");
        link.className = "media-rule-line theme-link";
        if (location.line != -1) {
          link.textContent = ":" + location.line;
        }
        div.appendChild(link);

        list.appendChild(div);
      }

      sidebar.hidden = !showSidebar || !inSource;

      this.emit("media-list-changed", editor);
    }.bind(this)).then(null, Cu.reportError);
  },

  





  _jumpToLocation: function(location) {
    let source = location.styleSheet || location.source;
    this.selectStyleSheet(source, location.line - 1, location.column - 1);
  },

  destroy: function() {
    if (this._highlighter) {
      this._highlighter.finalize();
      this._highlighter = null;
    }

    this._clearStyleSheetEditors();

    let sidebar = this._panelDoc.querySelector(".splitview-controller");
    let sidebarWidth = sidebar.getAttribute("width");
    Services.prefs.setIntPref(PREF_NAV_WIDTH, sidebarWidth);

    this._optionsMenu.removeEventListener("popupshowing",
                                          this._onOptionsPopupShowing);
    this._optionsMenu.removeEventListener("popuphiding",
                                          this._onOptionsPopupHiding);

    this._prefObserver.off(PREF_ORIG_SOURCES, this._onNewDocument);
    this._prefObserver.off(PREF_MEDIA_SIDEBAR, this._onMediaPrefChanged);
    this._prefObserver.destroy();
  }
}
