


"use strict";




function VariableBubbleView(DebuggerController, DebuggerView) {
  dumpn("VariableBubbleView was instantiated");

  this.StackFrames = DebuggerController.StackFrames;
  this.Parser = DebuggerController.Parser;
  this.DebuggerView = DebuggerView;

  this._onMouseMove = this._onMouseMove.bind(this);
  this._onMouseOut = this._onMouseOut.bind(this);
  this._onPopupHiding = this._onPopupHiding.bind(this);
}

VariableBubbleView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the VariableBubbleView");

    this._toolbox = DebuggerController._toolbox;
    this._editorContainer = document.getElementById("editor");
    this._editorContainer.addEventListener("mousemove", this._onMouseMove, false);
    this._editorContainer.addEventListener("mouseout", this._onMouseOut, false);

    this._tooltip = new Tooltip(document, {
      closeOnEvents: [{
        emitter: this._toolbox,
        event: "select"
      }, {
        emitter: this._editorContainer,
        event: "scroll",
        useCapture: true
      }]
    });
    this._tooltip.defaultPosition = EDITOR_VARIABLE_POPUP_POSITION;
    this._tooltip.defaultShowDelay = EDITOR_VARIABLE_HOVER_DELAY;
    this._tooltip.panel.addEventListener("popuphiding", this._onPopupHiding);
  },

  


  destroy: function() {
    dumpn("Destroying the VariableBubbleView");

    this._tooltip.panel.removeEventListener("popuphiding", this._onPopupHiding);
    this._editorContainer.removeEventListener("mousemove", this._onMouseMove, false);
    this._editorContainer.removeEventListener("mouseout", this._onMouseOut, false);
  },

  



  _ignoreLiterals: true,

  






  _findIdentifier: function(x, y) {
    let editor = this.DebuggerView.editor;

    
    let hoveredPos = editor.getPositionFromCoords({ left: x, top: y });
    let hoveredOffset = editor.getOffset(hoveredPos);
    let hoveredLine = hoveredPos.line;
    let hoveredColumn = hoveredPos.ch;

    
    
    let contents = editor.getText();
    let location = this.DebuggerView.Sources.selectedValue;
    let parsedSource = this.Parser.get(contents, location);
    let scriptInfo = parsedSource.getScriptInfo(hoveredOffset);

    
    if (scriptInfo.length == -1) {
      return;
    }

    
    
    let scriptStart = editor.getPosition(scriptInfo.start);
    let scriptLineOffset = scriptStart.line;
    let scriptColumnOffset = (hoveredLine == scriptStart.line ? scriptStart.ch : 0);

    let scriptLine = hoveredLine - scriptLineOffset;
    let scriptColumn = hoveredColumn - scriptColumnOffset;
    let identifierInfo = parsedSource.getIdentifierAt({
      line: scriptLine + 1,
      column: scriptColumn,
      scriptIndex: scriptInfo.index,
      ignoreLiterals: this._ignoreLiterals
    });

    
    if (!identifierInfo) {
      return;
    }

    
    
    let { start: identifierStart, end: identifierEnd } = identifierInfo.location;
    let identifierCoords = {
      line: identifierStart.line + scriptLineOffset,
      column: identifierStart.column + scriptColumnOffset,
      length: identifierEnd.column - identifierStart.column
    };

    
    
    this.StackFrames.evaluate(identifierInfo.evalString)
      .then(frameFinished => {
        if ("return" in frameFinished) {
          this.showContents({
            coords: identifierCoords,
            evalPrefix: identifierInfo.evalString,
            objectActor: frameFinished.return
          });
        } else {
          let msg = "Evaluation has thrown for: " + identifierInfo.evalString;
          console.warn(msg);
          dumpn(msg);
        }
      })
      .then(null, err => {
        let msg = "Couldn't evaluate: " + err.message;
        console.error(msg);
        dumpn(msg);
      });
  },

  









  showContents: function({ coords, evalPrefix, objectActor }) {
    let editor = this.DebuggerView.editor;
    let { line, column, length } = coords;

    
    this._markedText = editor.markText(
      { line: line - 1, ch: column },
      { line: line - 1, ch: column + length });

    
    
    if (VariablesView.isPrimitive({ value: objectActor })) {
      let className = VariablesView.getClass(objectActor);
      let textContent = VariablesView.getString(objectActor);
      this._tooltip.setTextContent({
        messages: [textContent],
        messagesClass: className,
        containerClass: "plain"
      }, [{
        label: L10N.getStr('addWatchExpressionButton'),
        className: "dbg-expression-button",
        command: () => {
          this.DebuggerView.VariableBubble.hideContents();
          this.DebuggerView.WatchExpressions.addExpression(evalPrefix, true);
        }
      }]);
    } else {
      this._tooltip.setVariableContent(objectActor, {
        searchPlaceholder: L10N.getStr("emptyPropertiesFilterText"),
        searchEnabled: Prefs.variablesSearchboxVisible,
        eval: (variable, value) => {
          let string = variable.evaluationMacro(variable, value);
          this.StackFrames.evaluate(string);
          this.DebuggerView.VariableBubble.hideContents();
        }
      }, {
        getEnvironmentClient: aObject => gThreadClient.environment(aObject),
        getObjectClient: aObject => gThreadClient.pauseGrip(aObject),
        simpleValueEvalMacro: this._getSimpleValueEvalMacro(evalPrefix),
        getterOrSetterEvalMacro: this._getGetterOrSetterEvalMacro(evalPrefix),
        overrideValueEvalMacro: this._getOverrideValueEvalMacro(evalPrefix)
      }, {
        fetched: (aEvent, aType) => {
          if (aType == "properties") {
            window.emit(EVENTS.FETCHED_BUBBLE_PROPERTIES);
          }
        }
      }, [{
        label: L10N.getStr("addWatchExpressionButton"),
        className: "dbg-expression-button",
        command: () => {
          this.DebuggerView.VariableBubble.hideContents();
          this.DebuggerView.WatchExpressions.addExpression(evalPrefix, true);
        }
      }], this._toolbox);
    }

    this._tooltip.show(this._markedText.anchor);
  },

  


  hideContents: function() {
    clearNamedTimeout("editor-mouse-move");
    this._tooltip.hide();
  },

  





  contentsShown: function() {
    return this._tooltip.isShown();
  },

  





  _getSimpleValueEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.simpleValueEvalMacro(item, string, aPrefix);
  },
  _getGetterOrSetterEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.getterOrSetterEvalMacro(item, string, aPrefix);
  },
  _getOverrideValueEvalMacro: function(aPrefix) {
    return (item, string) =>
      VariablesView.overrideValueEvalMacro(item, string, aPrefix);
  },

  


  _onMouseMove: function(e) {
    
    
    
    let isResumed = gThreadClient && gThreadClient.state != "paused";
    let isSelecting = this.DebuggerView.editor.somethingSelected() && e.buttons > 0;
    let isPopupVisible = !this._tooltip.isHidden();
    if (isResumed || isSelecting || isPopupVisible) {
      clearNamedTimeout("editor-mouse-move");
      return;
    }
    
    
    setNamedTimeout("editor-mouse-move",
      EDITOR_VARIABLE_HOVER_DELAY, () => this._findIdentifier(e.clientX, e.clientY));
  },

  


  _onMouseOut: function() {
    clearNamedTimeout("editor-mouse-move");
  },

  


  _onPopupHiding: function({ target }) {
    if (this._tooltip.panel != target) {
      return;
    }
    if (this._markedText) {
      this._markedText.clear();
      this._markedText = null;
    }
    if (!this._tooltip.isEmpty()) {
      this._tooltip.empty();
    }
  },

  _editorContainer: null,
  _markedText: null,
  _tooltip: null
};

DebuggerView.VariableBubble = new VariableBubbleView(DebuggerController, DebuggerView);
