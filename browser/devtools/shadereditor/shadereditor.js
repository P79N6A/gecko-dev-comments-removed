


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
const EventEmitter = require("devtools/toolkit/event-emitter");
const {Tooltip} = require("devtools/shared/widgets/Tooltip");
const Editor = require("devtools/sourceeditor/editor");


const EVENTS = {
  
  NEW_PROGRAM: "ShaderEditor:NewProgram",
  PROGRAMS_ADDED: "ShaderEditor:ProgramsAdded",

  
  SOURCES_SHOWN: "ShaderEditor:SourcesShown",

  
  SHADER_COMPILED: "ShaderEditor:ShaderCompiled",

  
  UI_RESET: "ShaderEditor:UIReset",

  
  EDITOR_ERROR_MARKERS_REMOVED: "ShaderEditor:EditorCleaned"
};

const STRINGS_URI = "chrome://browser/locale/devtools/shadereditor.properties"
const HIGHLIGHT_TINT = [1, 0, 0.25, 1]; 
const TYPING_MAX_DELAY = 500; 
const SHADERS_AUTOGROW_ITEMS = 4;
const GUTTER_ERROR_PANEL_OFFSET_X = 7; 
const GUTTER_ERROR_PANEL_DELAY = 100; 
const DEFAULT_EDITOR_CONFIG = {
  gutters: ["errors"],
  lineNumbers: true,
  showAnnotationRuler: true
};




let gToolbox, gTarget, gFront;




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
    this._onHostChanged = this._onHostChanged.bind(this);
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onProgramLinked = this._onProgramLinked.bind(this);
    this._onProgramsAdded = this._onProgramsAdded.bind(this);
    gToolbox.on("host-changed", this._onHostChanged);
    gTarget.on("will-navigate", this._onTabNavigated);
    gTarget.on("navigate", this._onTabNavigated);
    gFront.on("program-linked", this._onProgramLinked);

  },

  


  destroy: function() {
    gToolbox.off("host-changed", this._onHostChanged);
    gTarget.off("will-navigate", this._onTabNavigated);
    gTarget.off("navigate", this._onTabNavigated);
    gFront.off("program-linked", this._onProgramLinked);
  },

  


  _onHostChanged: function() {
    if (gToolbox.hostType == "side") {
      $("#shaders-pane").removeAttribute("height");
    }
  },

  


  _onTabNavigated: function(event, {isFrameSwitching}) {
    switch (event) {
      case "will-navigate": {
        
        if (!isFrameSwitching) {
          gFront.setup({ reload: false });
        }

        
        ShadersListView.empty();
        
        
        if (isFrameSwitching) {
          $("#reload-notice").hidden = false;
          $("#waiting-notice").hidden = true;
        } else {
          $("#reload-notice").hidden = true;
          $("#waiting-notice").hidden = false;
        }

        $("#content").hidden = true;
        window.emit(EVENTS.UI_RESET);

        break;
      }
      case "navigate": {
        
        
        
        
        gFront.getPrograms().then(this._onProgramsAdded);
        break;
      }
    }
  },

  


  _onProgramLinked: function(programActor) {
    this._addProgram(programActor);
    window.emit(EVENTS.NEW_PROGRAM);
  },

  


  _onProgramsAdded: function(programActors) {
    programActors.forEach(this._addProgram);
    window.emit(EVENTS.PROGRAMS_ADDED);
  },

  


  _addProgram: function(programActor) {
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

    this._onProgramSelect = this._onProgramSelect.bind(this);
    this._onProgramCheck = this._onProgramCheck.bind(this);
    this._onProgramMouseOver = this._onProgramMouseOver.bind(this);
    this._onProgramMouseOut = this._onProgramMouseOut.bind(this);

    this.widget.addEventListener("select", this._onProgramSelect, false);
    this.widget.addEventListener("check", this._onProgramCheck, false);
    this.widget.addEventListener("mouseover", this._onProgramMouseOver, true);
    this.widget.addEventListener("mouseout", this._onProgramMouseOut, true);
  },

  


  destroy: function() {
    this.widget.removeEventListener("select", this._onProgramSelect, false);
    this.widget.removeEventListener("check", this._onProgramCheck, false);
    this.widget.removeEventListener("mouseover", this._onProgramMouseOver, true);
    this.widget.removeEventListener("mouseout", this._onProgramMouseOut, true);
  },

  





  addProgram: function(programActor) {
    if (this.hasProgram(programActor)) {
      return;
    }

    
    
    
    
    let label = L10N.getFormatStr("shadersList.programLabel", this.itemCount);
    let contents = document.createElement("label");
    contents.className = "plain program-item";
    contents.setAttribute("value", label);
    contents.setAttribute("crop", "start");
    contents.setAttribute("flex", "1");

    
    this.push([contents], {
      index: -1, 
      attachment: {
        label: label,
        programActor: programActor,
        checkboxState: true,
        checkboxTooltip: L10N.getStr("shadersList.blackboxLabel")
      }
    });

    
    if (!this.selectedItem) {
      this.selectedIndex = 0;
    }

    
    
    if (gToolbox.hostType == "side" && this.itemCount == SHADERS_AUTOGROW_ITEMS) {
      this._pane.setAttribute("height", this._pane.getBoundingClientRect().height);
    }
  },

  







  hasProgram: function(programActor) {
    return !!this.attachments.filter(e => e.programActor == programActor).length;
  },

  


  _onProgramSelect: function({ detail: sourceItem }) {
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
      return ShadersEditorsView.setText({
        vs: vertexShaderText,
        fs: fragmentShaderText
      });
    }

    getShaders()
      .then(getSources)
      .then(showSources)
      .then(null, Cu.reportError);
  },

  


  _onProgramCheck: function({ detail: { checked }, target }) {
    let sourceItem = this.getItemForElement(target);
    let attachment = sourceItem.attachment;
    attachment.isBlackBoxed = !checked;
    attachment.programActor[checked ? "unblackbox" : "blackbox"]();
  },

  


  _onProgramMouseOver: function(e) {
    let sourceItem = this.getItemForElement(e.target, { noSiblings: true });
    if (sourceItem && !sourceItem.attachment.isBlackBoxed) {
      sourceItem.attachment.programActor.highlight(HIGHLIGHT_TINT);

      if (e instanceof Event) {
        e.preventDefault();
        e.stopPropagation();
      }
    }
  },

  


  _onProgramMouseOut: function(e) {
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

  


  destroy: Task.async(function*() {
    this._destroyed = true;
    yield this._toggleListeners("off");
    for (let p of this._editorPromises.values()) {
      let editor = yield p;
      editor.destroy();
    }
  }),

  









  setText: function(sources) {
    let view = this;
    function setTextAndClearHistory(editor, text) {
      editor.setText(text);
      editor.clearHistory();
    }

    return Task.spawn(function*() {
      yield view._toggleListeners("off");
      yield promise.all([
        view._getEditor("vs").then(e => setTextAndClearHistory(e, sources.vs)),
        view._getEditor("fs").then(e => setTextAndClearHistory(e, sources.fs))
      ]);
      yield view._toggleListeners("on");
    }).then(() => window.emit(EVENTS.SOURCES_SHOWN, sources));
  },

  








  _getEditor: function(type) {
    if (this._editorPromises.has(type)) {
      return this._editorPromises.get(type);
    }

    let deferred = promise.defer();
    this._editorPromises.set(type, deferred.promise);

    
    
    let parent = $("#" + type +"-editor");
    let editor = new Editor(DEFAULT_EDITOR_CONFIG);
    editor.config.mode = Editor.modes[type];

    if (this._destroyed) {
      deferred.resolve(editor);
    } else {
      editor.appendTo(parent).then(() => deferred.resolve(editor));
    }

    return deferred.promise;
  },

  







  _toggleListeners: function(flag) {
    return promise.all(["vs", "fs"].map(type => {
      return this._getEditor(type).then(editor => {
        editor[flag]("focus", this["_" + type + "Focused"]);
        editor[flag]("change", this["_" + type + "Changed"]);
      });
    }));
  },

  







  _onFocused: function(focused, unfocused) {
    $("#" + focused + "-editor-label").setAttribute("selected", "");
    $("#" + unfocused + "-editor-label").removeAttribute("selected");
  },

  





  _onChanged: function(type) {
    setNamedTimeout("gl-typed", TYPING_MAX_DELAY, () => this._doCompile(type));

    
    this._cleanEditor(type);
  },

  






  _doCompile: function(type) {
    Task.spawn(function*() {
      let editor = yield this._getEditor(type);
      let shaderActor = yield ShadersListView.selectedAttachment[type];

      try {
        yield shaderActor.compile(editor.getText());
        this._onSuccessfulCompilation();
      } catch (e) {
        this._onFailedCompilation(type, editor, e);
      }
    }.bind(this));
  },

  


  _onSuccessfulCompilation: function() {
    
    window.emit(EVENTS.SHADER_COMPILED, null);
  },

  


  _onFailedCompilation: function(type, editor, errors) {
    let lineCount = editor.lineCount();
    let currentLine = editor.getCursor().line;
    let listeners = { mouseover: this._onMarkerMouseOver };

    function matchLinesAndMessages(string) {
      return {
        
        lineMatch: string.match(/\d{2,}|[1-9]/),
        
        textMatch: string.match(/[^\s\d:][^\r\n|]*/)
      };
    }
    function discardInvalidMatches(e) {
      
      return e.lineMatch && e.textMatch;
    }
    function sanitizeValidMatches(e) {
      return {
        
        
        
        line: e.lineMatch[0] > lineCount ? currentLine : e.lineMatch[0] - 1,
        
        
        text: e.textMatch[0].trim().replace(/\s{2,}/g, " ")
      };
    }
    function sortByLine(first, second) {
      
      return first.line > second.line ? 1 : -1;
    }
    function groupSameLineMessages(accumulator, current) {
      
      let previous = accumulator[accumulator.length - 1];
      if (!previous || previous.line != current.line) {
        return [...accumulator, {
          line: current.line,
          messages: [current.text]
        }];
      } else {
        previous.messages.push(current.text);
        return accumulator;
      }
    }
    function displayErrors({ line, messages }) {
      
      editor.addMarker(line, "errors", "error");
      editor.setMarkerListeners(line, "errors", "error", listeners, messages);
      editor.addLineClass(line, "error-line");
    }

    (this._errors[type] = errors.link
      .split("ERROR")
      .map(matchLinesAndMessages)
      .filter(discardInvalidMatches)
      .map(sanitizeValidMatches)
      .sort(sortByLine)
      .reduce(groupSameLineMessages, []))
      .forEach(displayErrors);

    
    window.emit(EVENTS.SHADER_COMPILED, errors);
  },

  


  _onMarkerMouseOver: function(line, node, messages) {
    if (node._markerErrorsTooltip) {
      return;
    }

    let tooltip = node._markerErrorsTooltip = new Tooltip(document);
    tooltip.defaultOffsetX = GUTTER_ERROR_PANEL_OFFSET_X;
    tooltip.setTextContent({ messages: messages });
    tooltip.startTogglingOnHover(node, () => true, GUTTER_ERROR_PANEL_DELAY);
  },

  


  _cleanEditor: function(type) {
    this._getEditor(type).then(editor => {
      editor.removeAllMarkers("errors");
      this._errors[type].forEach(e => editor.removeLineClass(e.line));
      this._errors[type].length = 0;
      window.emit(EVENTS.EDITOR_ERROR_MARKERS_REMOVED);
    });
  },

  _errors: {
    vs: [],
    fs: []
  }
};




let L10N = new ViewHelpers.L10N(STRINGS_URI);




EventEmitter.decorate(this);




function $(selector, target = document) target.querySelector(selector);
