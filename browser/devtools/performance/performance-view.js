


"use strict";




let PerformanceView = {

  _state: null,

  
  
  _bufferStatusSupported: false,

  
  
  states: {
    empty: [
      { deck: "#performance-view", pane: "#empty-notice" }
    ],
    recording: [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#recording-notice" }
    ],
    "console-recording": [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#console-recording-notice" }
    ],
    recorded: [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#details-pane" }
    ],
    loading: [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#loading-notice" }
    ]
  },

  


  initialize: Task.async(function* () {
    this._recordButton = $("#main-record-button");
    this._importButton = $("#import-button");
    this._clearButton = $("#clear-button");

    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._onImportButtonClick = this._onImportButtonClick.bind(this);
    this._onClearButtonClick = this._onClearButtonClick.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onProfilerStatusUpdated = this._onProfilerStatusUpdated.bind(this);
    this._onRecordingWillStart = this._onRecordingWillStart.bind(this);
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingWillStop = this._onRecordingWillStop.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);

    for (let button of $$(".record-button")) {
      button.addEventListener("click", this._onRecordButtonClick);
    }
    this._importButton.addEventListener("click", this._onImportButtonClick);
    this._clearButton.addEventListener("click", this._onClearButtonClick);

    
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
    PerformanceController.on(EVENTS.PROFILER_STATUS_UPDATED, this._onProfilerStatusUpdated);
    PerformanceController.on(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);

    this.setState("empty");

    
    
    yield ToolbarView.initialize();
    yield RecordingsView.initialize();
    yield OverviewView.initialize();
    yield DetailsView.initialize();
  }),

  


  destroy: Task.async(function* () {
    for (let button of $$(".record-button")) {
      button.removeEventListener("click", this._onRecordButtonClick);
    }
    this._importButton.removeEventListener("click", this._onImportButtonClick);
    this._clearButton.removeEventListener("click", this._onClearButtonClick);

    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
    PerformanceController.off(EVENTS.PROFILER_STATUS_UPDATED, this._onProfilerStatusUpdated);
    PerformanceController.off(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);

    yield ToolbarView.destroy();
    yield RecordingsView.destroy();
    yield OverviewView.destroy();
    yield DetailsView.destroy();
  }),

  



  setState: function (state) {
    let viewConfig = this.states[state];
    if (!viewConfig) {
      throw new Error(`Invalid state for PerformanceView: ${state}`);
    }
    for (let { deck, pane } of viewConfig) {
      $(deck).selectedPanel = $(pane);
    }

    this._state = state;

    if (state === "console-recording") {
      let recording = PerformanceController.getCurrentRecording();
      let label = recording.getLabel() || "";
      
      label = label ? `"${label}"` : "";

      let startCommand = $(".console-profile-recording-notice .console-profile-command");
      let stopCommand = $(".console-profile-stop-notice .console-profile-command");

      startCommand.value = `console.profile(${label})`;
      stopCommand.value = `console.profileEnd(${label})`;
    }

    this.updateBufferStatus();
    this.emit(EVENTS.UI_STATE_CHANGED, state);
  },

  


  getState: function () {
    return this._state;
  },

  


  updateBufferStatus: function () {
    
    
    if (!this._bufferStatusSupported) {
      return;
    }

    let recording = PerformanceController.getCurrentRecording();
    if (!recording || !recording.isRecording()) {
      return;
    }

    let bufferUsage = recording.getBufferUsage();

    
    let percent = Math.floor(bufferUsage * 100);

    let $container = $("#details-pane-container");
    let $bufferLabel = $(".buffer-status-message", $container.selectedPanel);

    
    
    if (percent >= 99) {
      $container.setAttribute("buffer-status", "full");
    } else {
      $container.setAttribute("buffer-status", "in-progress");
    }

    $bufferLabel.value = `Buffer ${percent}% full`;
    this.emit(EVENTS.UI_BUFFER_UPDATED, percent);
  },

  





  _lockRecordButtons: function (lock) {
    for (let button of $$(".record-button")) {
      if (lock) {
        button.setAttribute("locked", "true");
      } else {
        button.removeAttribute("locked");
      }
    }
  },

  





  _activateRecordButtons: function (activate) {
    for (let button of $$(".record-button")) {
      if (activate) {
        button.setAttribute("checked", "true");
      } else {
        button.removeAttribute("checked");
      }
    }
  },

  



  _onRecordingWillStart: function (_, recording) {
    if (!recording.isConsole()) {
      this._lockRecordButtons(true);
      this._activateRecordButtons(true);
    }
  },

  


  _onRecordingStarted: function (_, recording) {
    
    
    if (!recording.isConsole()) {
      this._lockRecordButtons(false);
    }
    if (recording.isRecording()) {
      this.updateBufferStatus();
    }
  },

  


  _onRecordingWillStop: function (_, recording) {
    if (!recording.isConsole()) {
      this._lockRecordButtons(true);
      this._activateRecordButtons(false);
    }
    
    
    if (recording === PerformanceController.getCurrentRecording()) {
      this.setState("loading");
    }
  },

  


  _onRecordingStopped: function (_, recording) {
    
    
    if (!recording.isConsole()) {
      this._lockRecordButtons(false);
    }

    
    
    if (recording === PerformanceController.getCurrentRecording()) {
      this.setState("recorded");
    }
  },

  


  _onClearButtonClick: function (e) {
    this.emit(EVENTS.UI_CLEAR_RECORDINGS);
  },

  


  _onRecordButtonClick: function (e) {
    if (this._recordButton.hasAttribute("checked")) {
      this.emit(EVENTS.UI_STOP_RECORDING);
    } else {
      this.emit(EVENTS.UI_START_RECORDING);
    }
  },

  


  _onImportButtonClick: function(e) {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    
    fp.init(window, "Import recordingâ€¦", Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");

    if (fp.show() == Ci.nsIFilePicker.returnOK) {
      this.emit(EVENTS.UI_IMPORT_RECORDING, fp.file);
    }
  },

  


  _onRecordingSelected: function (_, recording) {
    if (!recording) {
      this.setState("empty");
    } else if (recording.isRecording() && recording.isConsole()) {
      this.setState("console-recording");
    } else if (recording.isRecording()) {
      this.setState("recording");
    } else {
      this.setState("recorded");
    }
  },

  



  _onProfilerStatusUpdated: function (_, data) {
    
    
    if (!data || data.position === void 0) {
      return;
    }
    
    if (!this._bufferStatusSupported) {
      this._bufferStatusSupported = true;
      $("#details-pane-container").setAttribute("buffer-status", "in-progress");
    }

    if (!this.getState("recording") && !this.getState("console-recording")) {
      return;
    }

    this.updateBufferStatus();
  },

  toString: () => "[object PerformanceView]"
};




EventEmitter.decorate(PerformanceView);
