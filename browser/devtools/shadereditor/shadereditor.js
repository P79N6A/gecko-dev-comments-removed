


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = require("sdk/core/promise");
const EventEmitter = require("devtools/shared/event-emitter");
const Editor = require("devtools/sourceeditor/editor");


const EVENTS = {
  
  SOURCES_SHOWN: "ShaderEditor:SourcesShown",
  
  SHADER_COMPILED: "ShaderEditor:ShaderCompiled"
};

const STRINGS_URI = "chrome://browser/locale/devtools/shadereditor.properties"
const HIGHLIGHT_COLOR = [1, 0, 0, 1];
const BLACKBOX_COLOR = [0, 0, 0, 0];
const TYPING_MAX_DELAY = 500;
const SHADERS_AUTOGROW_ITEMS = 4;
const DEFAULT_EDITOR_CONFIG = {
  mode: Editor.modes.text,
  lineNumbers: true,
  showAnnotationRuler: true
};




let gTarget, gFront;




function startupShaderEditor() {
  return promise.all([
    EventsHandler.initialize(),
    ShadersListView.initialize(),
    ShadersEditorsView.initialize()
  ]);
}




function shutdownShaderEditor() {
  return promise.all([
    EventsHandler.destroy(),
    ShadersListView.destroy(),
    ShadersEditorsView.destroy()
  ]);
}




let EventsHandler = {
  


  initialize: function() {
    this._onWillNavigate = this._onWillNavigate.bind(this);
    this._onProgramLinked = this._onProgramLinked.bind(this);
    gTarget.on("will-navigate", this._onWillNavigate);
    gFront.on("program-linked", this._onProgramLinked);

  },

  


  destroy: function() {
    gTarget.off("will-navigate", this._onWillNavigate);
    gFront.off("program-linked", this._onProgramLinked);
  },

  


  _onWillNavigate: function() {
    gFront.setup();

    ShadersListView.empty();
    ShadersEditorsView.setText({ vs: "", fs: "" });
    $("#reload-notice").hidden = true;
    $("#waiting-notice").hidden = false;
    $("#content").hidden = true;
  },

  


  _onProgramLinked: function(programActor) {
    $("#waiting-notice").hidden = true;
    $("#reload-notice").hidden = true;
    $("#content").hidden = false;
    ShadersListView.addProgram(programActor);
  }
};




let ShadersListView = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    this.widget = new SideMenuWidget(this._pane = $("#shaders-pane"), {
      showArrows: true,
      showItemCheckboxes: true
    });

    this._onShaderSelect = this._onShaderSelect.bind(this);
    this._onShaderCheck = this._onShaderCheck.bind(this);
    this._onShaderMouseEnter = this._onShaderMouseEnter.bind(this);
    this._onShaderMouseLeave = this._onShaderMouseLeave.bind(this);

    this.widget.addEventListener("select", this._onShaderSelect, false);
    this.widget.addEventListener("check", this._onShaderCheck, false);
    this.widget.addEventListener("mouseenter", this._onShaderMouseEnter, true);
    this.widget.addEventListener("mouseleave", this._onShaderMouseLeave, true);
  },

  


  destroy: function() {
    this.widget.removeEventListener("select", this._onShaderSelect, false);
    this.widget.removeEventListener("check", this._onShaderCheck, false);
    this.widget.removeEventListener("mouseenter", this._onShaderMouseEnter, true);
    this.widget.removeEventListener("mouseleave", this._onShaderMouseLeave, true);
  },

  





  addProgram: function(programActor) {
    
    
    
    
    let label = L10N.getFormatStr("shadersList.programLabel", this.itemCount);

    
    this.push([label, ""], {
      index: -1, 
      relaxed: true, 
      attachment: {
        programActor: programActor,
        checkboxState: true,
        checkboxTooltip: L10N.getStr("shadersList.blackboxLabel")
      }
    });

    
    if (!this.selectedItem) {
      this.selectedIndex = 0;
    }
  },

  


  _onShaderSelect: function({ detail: sourceItem }) {
    if (!sourceItem) {
      return;
    }
    
    let attachment = sourceItem.attachment;

    function getShaders() {
      return promise.all([
        attachment.vs || (attachment.vs = attachment.programActor.getVertexShader()),
        attachment.fs || (attachment.fs = attachment.programActor.getFragmentShader())
      ]);
    }
    function getSources([vertexShaderActor, fragmentShaderActor]) {
      return promise.all([
        vertexShaderActor.getText(),
        fragmentShaderActor.getText()
      ]);
    }
    function showSources([vertexShaderText, fragmentShaderText]) {
      ShadersEditorsView.setText({
        vs: vertexShaderText,
        fs: fragmentShaderText
      });
    }

    getShaders().then(getSources).then(showSources).then(null, Cu.reportError);
  },

  


  _onShaderCheck: function({ detail: { checked }, target }) {
    let sourceItem = this.getItemForElement(target);
    let attachment = sourceItem.attachment;
    attachment.isBlackBoxed = !checked;
    attachment.programActor[checked ? "unhighlight" : "highlight"](BLACKBOX_COLOR);
  },

  


  _onShaderMouseEnter: function(e) {
    let sourceItem = this.getItemForElement(e.target, { noSiblings: true });
    if (sourceItem && !sourceItem.attachment.isBlackBoxed) {
      sourceItem.attachment.programActor.highlight(HIGHLIGHT_COLOR);

      if (e instanceof Event) {
        e.preventDefault();
        e.stopPropagation();
      }
    }
  },

  


  _onShaderMouseLeave: function(e) {
    let sourceItem = this.getItemForElement(e.target, { noSiblings: true });
    if (sourceItem && !sourceItem.attachment.isBlackBoxed) {
      sourceItem.attachment.programActor.unhighlight();

      if (e instanceof Event) {
        e.preventDefault();
        e.stopPropagation();
      }
    }
  }
});




let ShadersEditorsView = {
  


  initialize: function() {
    XPCOMUtils.defineLazyGetter(this, "_editorPromises", () => new Map());
    this._vsFocused = this._onFocused.bind(this, "vs", "fs");
    this._fsFocused = this._onFocused.bind(this, "fs", "vs");
    this._vsChanged = this._onChanged.bind(this, "vs");
    this._fsChanged = this._onChanged.bind(this, "fs");
  },

  


  destroy: function() {
    this._toggleListeners("off");
  },

  







  setText: function(sources) {
    function setTextAndClearHistory(editor, text) {
      editor.setText(text);
      editor.clearHistory();
    }

    this._toggleListeners("off");
    this._getEditor("vs").then(e => setTextAndClearHistory(e, sources.vs));
    this._getEditor("fs").then(e => setTextAndClearHistory(e, sources.fs));
    this._toggleListeners("on");

    window.emit(EVENTS.SOURCES_SHOWN, sources);
  },

  






  _getEditor: function(type) {
    if ($("#content").hidden) {
      return promise.reject(null);
    }
    if (this._editorPromises.has(type)) {
      return this._editorPromises.get(type);
    }

    let deferred = promise.defer();
    this._editorPromises.set(type, deferred.promise);

    
    
    let parent = $("#" + type +"-editor");
    let editor = new Editor(DEFAULT_EDITOR_CONFIG);
    editor.appendTo(parent).then(() => deferred.resolve(editor));

    return deferred.promise;
  },

  





  _toggleListeners: function(flag) {
    ["vs", "fs"].forEach(type => {
      this._getEditor(type).then(editor => {
        editor[flag]("focus", this["_" + type + "Focused"]);
        editor[flag]("change", this["_" + type + "Changed"]);
      });
    });
  },

  







  _onFocused: function(focused, unfocused) {
    $("#" + focused + "-editor-label").setAttribute("selected", "");
    $("#" + unfocused + "-editor-label").removeAttribute("selected");
  },

  





  _onChanged: function(type) {
    setNamedTimeout("gl-typed", TYPING_MAX_DELAY, () => this._doCompile(type));
  },

  






  _doCompile: function(type) {
    Task.spawn(function() {
      let editor = yield this._getEditor(type);
      let shaderActor = yield ShadersListView.selectedAttachment[type];

      try {
        yield shaderActor.compile(editor.getText());
        window.emit(EVENTS.SHADER_COMPILED, null);
        
      } catch (error) {
        window.emit(EVENTS.SHADER_COMPILED, error);
        
      }
    }.bind(this));
  }
};




let L10N = new ViewHelpers.L10N(STRINGS_URI);




EventEmitter.decorate(this);




function $(selector, target = document) target.querySelector(selector);
