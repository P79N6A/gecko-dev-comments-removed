


"use strict";




let DetailsSubview = {
  


  initialize: function () {
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onOverviewRangeChange = this._onOverviewRangeChange.bind(this);
    this._onDetailsViewSelected = this._onDetailsViewSelected.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onOverviewRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onOverviewRangeChange);
    DetailsView.on(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  },

  


  destroy: function () {
    clearNamedTimeout("range-change-debounce");

    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onOverviewRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onOverviewRangeChange);
    DetailsView.off(EVENTS.DETAILS_VIEW_SELECTED, this._onDetailsViewSelected);
  },

  



  rangeChangeDebounceTime: 0,

  






  requiresUpdateOnRangeChange: true,

  




  shouldUpdateWhenShown: false,

  



  canUpdateWhileHidden: false,

  



  rerenderPrefs: [],

  



  observedPrefs: [],

  



  shouldUpdateWhileMouseIsActive: false,

  


  _onRecordingStoppedOrSelected: function(_, recording) {
    if (!recording || !recording.isCompleted()) {
      return;
    }
    if (DetailsView.isViewSelected(this) || this.canUpdateWhileHidden) {
      this.render();
    } else {
      this.shouldUpdateWhenShown = true;
    }
  },

  


  _onOverviewRangeChange: function (_, interval) {
    if (!this.requiresUpdateOnRangeChange) {
      return;
    }
    if (DetailsView.isViewSelected(this)) {
      let debounced = () => {
        if (!this.shouldUpdateWhileMouseIsActive && OverviewView.isMouseActive) {
          
          setNamedTimeout("range-change-debounce", this.rangeChangeDebounceTime, debounced);
        } else {
          this.render(interval);
        }
      };
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
  },

  


  _onPrefChanged: function (_, prefName) {
    if (~this.observedPrefs.indexOf(prefName) && this._onObservedPrefChange) {
      this._onObservedPrefChange(_, prefName);
    }

    
    
    let recording = PerformanceController.getCurrentRecording();
    if (!recording || !recording.isCompleted()) {
      return;
    }

    if (!~this.rerenderPrefs.indexOf(prefName)) {
      return;
    }

    if (this._onRerenderPrefChanged) {
      this._onRerenderPrefChanged(_, prefName);
    }

    if (DetailsView.isViewSelected(this) || this.canUpdateWhileHidden) {
      this.render(OverviewView.getTimeInterval());
    } else {
      this.shouldUpdateWhenShown = true;
    }
  }
};
