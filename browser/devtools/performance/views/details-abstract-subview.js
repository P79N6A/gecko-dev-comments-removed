


"use strict";




let DetailsSubview = {
  


  initialize: function () {
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onOverviewRangeChange = this._onOverviewRangeChange.bind(this);
    this._onDetailsViewSelected = this._onDetailsViewSelected.bind(this);

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onOverviewRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onOverviewRangeChange);
    DetailsView.on(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  },

  


  destroy: function () {
    clearNamedTimeout("range-change-debounce");

    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onOverviewRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onOverviewRangeChange);
    DetailsView.off(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  },

  



  rangeChangeDebounceTime: 0,

  




  shouldUpdateWhenShown: false,

  



  canUpdateWhileHidden: false,

  


  _onRecordingStoppedOrSelected: function(_, recording) {
    if (recording.isRecording()) {
      return;
    }
    if (DetailsView.isViewSelected(this) || this.canUpdateWhileHidden) {
      this.render();
    } else {
      this.shouldUpdateWhenShown = true;
    }
  },

  


  _onOverviewRangeChange: function (_, interval) {
    if (DetailsView.isViewSelected(this)) {
      let debounced = () => this.render(interval);
      setNamedTimeout("range-change-debounce", this.rangeChangeDebounceTime, debounced);
    } else {
      this.shouldUpdateWhenShown = true;
    }
  },

  


  _onDetailsViewSelected: function() {
    if (DetailsView.isViewSelected(this) && this.shouldUpdateWhenShown) {
      this.render(OverviewView.getTimeInterval());
      this.shouldUpdateWhenShown = false;
    }
  }
};




EventEmitter.decorate(DetailsSubview);
