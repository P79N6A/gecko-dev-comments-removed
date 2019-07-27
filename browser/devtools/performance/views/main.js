


"use strict";




let PerformanceView = {
  


  initialize: function () {
    this._recordButton = $("#record-button");

    this._onRecordButtonClick = this._onRecordButtonClick.bind(this);
    this._unlockRecordButton = this._unlockRecordButton.bind(this);

    this._recordButton.addEventListener("click", this._onRecordButtonClick);

    
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._unlockRecordButton);

    return promise.all([
      OverviewView.initialize(),
      DetailsView.initialize()
    ]);
  },

  


  destroy: function () {
    this._recordButton.removeEventListener("click", this._onRecordButtonClick);
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._unlockRecordButton);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._unlockRecordButton);

    return promise.all([
      OverviewView.destroy(),
      DetailsView.destroy()
    ]);
  },

  


  _unlockRecordButton: function () {
    this._recordButton.removeAttribute("locked");
  },

  


  _onRecordButtonClick: function (e) {
    if (this._recordButton.hasAttribute("checked")) {
      this._recordButton.removeAttribute("checked");
      this._recordButton.setAttribute("locked", "true");
      this.emit(EVENTS.UI_STOP_RECORDING);
    } else {
      this._recordButton.setAttribute("checked", "true");
      this._recordButton.setAttribute("locked", "true");
      this.emit(EVENTS.UI_START_RECORDING);
    }
  }
};




EventEmitter.decorate(PerformanceView);
