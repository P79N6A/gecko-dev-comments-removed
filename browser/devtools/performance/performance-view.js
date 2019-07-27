


"use strict";




let PerformanceView = {

  _state: null,

  
  
  states: {
    empty: [
      { deck: "#performance-view", pane: "#empty-notice" }
    ],
    recording: [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#recording-notice" }
    ],
    recorded: [
      { deck: "#performance-view", pane: "#performance-view-content" },
      { deck: "#details-pane-container", pane: "#details-pane" }
    ]
  },

  


  initialize: function () {
    this._recordButton = $("#record-button");
    this._importButton = $("#import-button");

    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._onImportButtonClick = this._onImportButtonClick.bind(this);
    this._lockRecordButton = this._lockRecordButton.bind(this);
    this._unlockRecordButton = this._unlockRecordButton.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);

    for (let button of $$(".record-button")) {
      button.addEventListener("click", this._onRecordButtonClick);
    }
    this._importButton.addEventListener("click", this._onImportButtonClick);

    
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);

    this.setState("empty");

    return promise.all([
      RecordingsView.initialize(),
      OverviewView.initialize(),
      ToolbarView.initialize(),
      DetailsView.initialize()
    ]);
  },

  


  destroy: function () {
    for (let button of $$(".record-button")) {
      button.removeEventListener("click", this._onRecordButtonClick);
    }
    this._importButton.removeEventListener("click", this._onImportButtonClick);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);

    return promise.all([
      RecordingsView.destroy(),
      OverviewView.destroy(),
      ToolbarView.destroy(),
      DetailsView.destroy()
    ]);
  },

  



  setState: function (state) {
    let viewConfig = this.states[state];
    if (!viewConfig) {
      throw new Error(`Invalid state for PerformanceView: ${state}`);
    }
    for (let { deck, pane } of viewConfig) {
      $(deck).selectedPanel = $(pane);
    }

    this._state = state;
  },

  


  getState: function () {
    return this._state;
  },

  



  _lockRecordButton: function () {
    this._recordButton.setAttribute("locked", "true");
  },

  


  _unlockRecordButton: function () {
    this._recordButton.removeAttribute("locked");
  },

  


  _onRecordingStopped: function (_, recording) {
    this._unlockRecordButton();

    
    
    
    if (recording === PerformanceController.getCurrentRecording()) {
      this.setState("recorded");
    }
  },

  


  _onRecordButtonClick: function (e) {
    if (this._recordButton.hasAttribute("checked")) {
      this._recordButton.removeAttribute("checked");
      this._lockRecordButton();
      this.emit(EVENTS.UI_STOP_RECORDING);
    } else {
      this._recordButton.setAttribute("checked", "true");
      this._lockRecordButton();
      this.emit(EVENTS.UI_START_RECORDING);
    }
  },

  


  _onImportButtonClick: function(e) {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("recordingsList.saveDialogTitle"), Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");

    if (fp.show() == Ci.nsIFilePicker.returnOK) {
      this.emit(EVENTS.UI_IMPORT_RECORDING, fp.file);
    }
  },

  


  _onRecordingSelected: function (_, recording) {
    if (recording.isRecording()) {
      this.setState("recording");
    } else {
      this.setState("recorded");
    }
  }
};




EventEmitter.decorate(PerformanceView);
