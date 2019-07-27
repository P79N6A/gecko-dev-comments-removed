


"use strict";




let RecordingsView = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    this.widget = new SideMenuWidget($("#recordings-list"));

    this._onSelect = this._onSelect.bind(this);
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRecordingImported = this._onRecordingImported.bind(this);
    this._onSaveButtonClick = this._onSaveButtonClick.bind(this);
    this._onRecordingsCleared = this._onRecordingsCleared.bind(this);

    this.emptyText = L10N.getStr("noRecordingsText");

    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.CONSOLE_RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_IMPORTED, this._onRecordingImported);
    PerformanceController.on(EVENTS.RECORDINGS_CLEARED, this._onRecordingsCleared);
    this.widget.addEventListener("select", this._onSelect, false);
  },

  


  destroy: function() {
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.CONSOLE_RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_IMPORTED, this._onRecordingImported);
    PerformanceController.off(EVENTS.RECORDINGS_CLEARED, this._onRecordingsCleared);
    this.widget.removeEventListener("select", this._onSelect, false);
  },

  





  addEmptyRecording: function (recording) {
    let titleNode = document.createElement("label");
    titleNode.className = "plain recording-item-title";
    titleNode.setAttribute("value", recording.getLabel() ||
      L10N.getFormatStr("recordingsList.itemLabel", this.itemCount + 1));

    let durationNode = document.createElement("label");
    durationNode.className = "plain recording-item-duration";
    durationNode.setAttribute("value",
      L10N.getStr("recordingsList.recordingLabel"));

    let saveNode = document.createElement("label");
    saveNode.className = "plain recording-item-save";
    saveNode.addEventListener("click", this._onSaveButtonClick);

    let hspacer = document.createElement("spacer");
    hspacer.setAttribute("flex", "1");

    let footerNode = document.createElement("hbox");
    footerNode.className = "recording-item-footer";
    footerNode.appendChild(durationNode);
    footerNode.appendChild(hspacer);
    footerNode.appendChild(saveNode);

    let vspacer = document.createElement("spacer");
    vspacer.setAttribute("flex", "1");

    let contentsNode = document.createElement("vbox");
    contentsNode.className = "recording-item";
    contentsNode.setAttribute("flex", "1");
    contentsNode.appendChild(titleNode);
    contentsNode.appendChild(vspacer);
    contentsNode.appendChild(footerNode);

    
    return this.push([contentsNode], {
      
      
      attachment: recording
    });
  },

  





  _onRecordingStarted: function (_, recording) {
    
    
    
    
    let recordingItem = this.addEmptyRecording(recording);

    
    recordingItem.isRecording = true;

    
    
    if (!recording.isConsole() || this.selectedIndex === -1) {
      this.selectedItem = recordingItem;
    }
  },

  





  _onRecordingStopped: function (_, recording) {
    let recordingItem = this.getItemForPredicate(e => e.attachment === recording);

    
    recordingItem.isRecording = false;

    
    this.finalizeRecording(recordingItem);

    
    if (!recording.isConsole()) {
      this.forceSelect(recordingItem);
    }
  },

  





  _onRecordingImported: function (_, model) {
    let recordingItem = this.addEmptyRecording(model);
    recordingItem.isRecording = false;

    
    this.selectedItem = recordingItem;

    
    this.finalizeRecording(recordingItem);
  },

  


  _onRecordingsCleared: function () {
    this.empty();
  },

  





  finalizeRecording: function (recordingItem) {
    let model = recordingItem.attachment;

    let saveNode = $(".recording-item-save", recordingItem.target);
    saveNode.setAttribute("value",
      L10N.getStr("recordingsList.saveLabel"));

    let durationMillis = model.getDuration().toFixed(0);
    let durationNode = $(".recording-item-duration", recordingItem.target);
    durationNode.setAttribute("value",
      L10N.getFormatStr("recordingsList.durationLabel", durationMillis));
  },

  


  _onSelect: Task.async(function*({ detail: recordingItem }) {
    if (!recordingItem) {
      return;
    }

    let model = recordingItem.attachment;
    this.emit(EVENTS.RECORDING_SELECTED, model);
  }),

  


  _onSaveButtonClick: function (e) {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("recordingsList.saveDialogTitle"), Ci.nsIFilePicker.modeSave);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");
    fp.defaultString = "profile.json";

    fp.open({ done: result => {
      if (result == Ci.nsIFilePicker.returnCancel) {
        return;
      }
      let recordingItem = this.getItemForElement(e.target);
      this.emit(EVENTS.UI_EXPORT_RECORDING, recordingItem.attachment, fp.file);
    }});
  },

  toString: () => "[object RecordingsView]"
});




EventEmitter.decorate(RecordingsView);
