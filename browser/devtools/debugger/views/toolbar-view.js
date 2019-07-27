


"use strict";





function ToolbarView(DebuggerController, DebuggerView) {
  dumpn("ToolbarView was instantiated");

  this.StackFrames = DebuggerController.StackFrames;
  this.ThreadState = DebuggerController.ThreadState;
  this.DebuggerController = DebuggerController;
  this.DebuggerView = DebuggerView;

  this._onTogglePanesPressed = this._onTogglePanesPressed.bind(this);
  this._onResumePressed = this._onResumePressed.bind(this);
  this._onStepOverPressed = this._onStepOverPressed.bind(this);
  this._onStepInPressed = this._onStepInPressed.bind(this);
  this._onStepOutPressed = this._onStepOutPressed.bind(this);
}

ToolbarView.prototype = {
  get activeThread() {
    return this.DebuggerController.activeThread;
  },

  get resumptionWarnFunc() {
    return this.DebuggerController._ensureResumptionOrder;
  },

  


  initialize: function() {
    dumpn("Initializing the ToolbarView");

    this._instrumentsPaneToggleButton = document.getElementById("instruments-pane-toggle");
    this._resumeButton = document.getElementById("resume");
    this._stepOverButton = document.getElementById("step-over");
    this._stepInButton = document.getElementById("step-in");
    this._stepOutButton = document.getElementById("step-out");
    this._resumeOrderTooltip = new Tooltip(document);
    this._resumeOrderTooltip.defaultPosition = TOOLBAR_ORDER_POPUP_POSITION;

    let resumeKey = ShortcutUtils.prettifyShortcut(document.getElementById("resumeKey"));
    let stepOverKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepOverKey"));
    let stepInKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepInKey"));
    let stepOutKey = ShortcutUtils.prettifyShortcut(document.getElementById("stepOutKey"));
    this._resumeTooltip = L10N.getFormatStr("resumeButtonTooltip", resumeKey);
    this._pauseTooltip = L10N.getFormatStr("pauseButtonTooltip", resumeKey);
    this._stepOverTooltip = L10N.getFormatStr("stepOverTooltip", stepOverKey);
    this._stepInTooltip = L10N.getFormatStr("stepInTooltip", stepInKey);
    this._stepOutTooltip = L10N.getFormatStr("stepOutTooltip", stepOutKey);

    this._instrumentsPaneToggleButton.addEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.addEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.addEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.addEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.addEventListener("mousedown", this._onStepOutPressed, false);

    this._stepOverButton.setAttribute("tooltiptext", this._stepOverTooltip);
    this._stepInButton.setAttribute("tooltiptext", this._stepInTooltip);
    this._stepOutButton.setAttribute("tooltiptext", this._stepOutTooltip);
    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the ToolbarView");

    this._instrumentsPaneToggleButton.removeEventListener("mousedown", this._onTogglePanesPressed, false);
    this._resumeButton.removeEventListener("mousedown", this._onResumePressed, false);
    this._stepOverButton.removeEventListener("mousedown", this._onStepOverPressed, false);
    this._stepInButton.removeEventListener("mousedown", this._onStepInPressed, false);
    this._stepOutButton.removeEventListener("mousedown", this._onStepOutPressed, false);
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      resumeCommand: () => this._onResumePressed(),
      stepOverCommand: () => this._onStepOverPressed(),
      stepInCommand: () => this._onStepInPressed(),
      stepOutCommand: () => this._onStepOutPressed()
    });
  },

  






  showResumeWarning: function(aPausedUrl) {
    let label = L10N.getFormatStr("resumptionOrderPanelTitle", aPausedUrl);
    let defaultStyle = "default-tooltip-simple-text-colors";
    this._resumeOrderTooltip.setTextContent({ messages: [label], isAlertTooltip: true });
    this._resumeOrderTooltip.show(this._resumeButton);
  },

  





  toggleResumeButtonState: function(aState) {
    
    if (aState == "paused") {
      this._resumeButton.setAttribute("checked", "true");
      this._resumeButton.setAttribute("tooltiptext", this._resumeTooltip);
    }
    
    else if (aState == "attached") {
      this._resumeButton.removeAttribute("checked");
      this._resumeButton.setAttribute("tooltiptext", this._pauseTooltip);
    }
  },

  


  _onTogglePanesPressed: function() {
    DebuggerView.toggleInstrumentsPane({
      visible: DebuggerView.instrumentsPaneHidden,
      animated: true,
      delayed: true
    });
  },

  


  _onResumePressed: function() {
    if (this.StackFrames._currentFrameDescription != FRAME_TYPE.NORMAL) {
      return;
    }

    if (this.activeThread.paused) {
      this.StackFrames.currentFrameDepth = -1;
      this.activeThread.resume(this.resumptionWarnFunc);
    } else {
      this.ThreadState.interruptedByResumeButton = true;
      this.activeThread.interrupt();
    }
  },

  


  _onStepOverPressed: function() {
    if (this.activeThread.paused) {
      this.StackFrames.currentFrameDepth = -1;
      this.activeThread.stepOver(this.resumptionWarnFunc);
    }
  },

  


  _onStepInPressed: function() {
    if (this.StackFrames._currentFrameDescription != FRAME_TYPE.NORMAL) {
      return;
    }

    if (this.activeThread.paused) {
      this.StackFrames.currentFrameDepth = -1;
      this.activeThread.stepIn(this.resumptionWarnFunc);
    }
  },

  


  _onStepOutPressed: function() {
    if (this.activeThread.paused) {
      this.StackFrames.currentFrameDepth = -1;
      this.activeThread.stepOut(this.resumptionWarnFunc);
    }
  },

  _instrumentsPaneToggleButton: null,
  _resumeButton: null,
  _stepOverButton: null,
  _stepInButton: null,
  _stepOutButton: null,
  _resumeOrderTooltip: null,
  _resumeTooltip: "",
  _pauseTooltip: "",
  _stepOverTooltip: "",
  _stepInTooltip: "",
  _stepOutTooltip: ""
};

DebuggerView.Toolbar = new ToolbarView(DebuggerController, DebuggerView);
