


"use strict";




let PerformanceView = {
  


  initialize: function () {
    this._recordButton = $("#record-button");
    this._importButton = $("#import-button");
    this._exportButton = $("#export-button");

    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._onImportButtonClick = this._onImportButtonClick.bind(this);
    this._onExportButtonClick = this._onExportButtonClick.bind(this);
    this._lockRecordButton = this._lockRecordButton.bind(this);
    this._unlockRecordButton = this._unlockRecordButton.bind(this);

    this._recordButton.addEventListener("click", this._onRecordButtonClick);
    this._importButton.addEventListener("click", this._onImportButtonClick);
    this._exportButton.addEventListener("click", this._onExportButtonClick);

    
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._unlockRecordButton);

    return promise.all([
      OverviewView.initialize(),
      DetailsView.initialize()
    ]);
  },

  


  destroy: function () {
    this._recordButton.removeEventListener("click", this._onRecordButtonClick);
    this._importButton.removeEventListener("click", this._onImportButtonClick);
    this._exportButton.removeEventListener("click", this._onExportButtonClick);

    PerformanceController.off(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._unlockRecordButton);

    return promise.all([
      OverviewView.destroy(),
      DetailsView.destroy()
    ]);
  },

  



  _lockRecordButton: function () {
    this._recordButton.setAttribute("locked", "true");
  },

  


  _unlockRecordButton: function () {
    this._recordButton.removeAttribute("locked");
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

  


  _onExportButtonClick: function(e) {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, L10N.getStr("recordingsList.saveDialogTitle"), Ci.nsIFilePicker.modeSave);
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogJSONFilter"), "*.json");
    fp.appendFilter(L10N.getStr("recordingsList.saveDialogAllFilter"), "*.*");
    fp.defaultString = "profile.json";

    fp.open({ done: result => {
      if (result != Ci.nsIFilePicker.returnCancel) {
        this.emit(EVENTS.UI_EXPORT_RECORDING, fp.file);
      }
    }});
  }
};




EventEmitter.decorate(PerformanceView);
