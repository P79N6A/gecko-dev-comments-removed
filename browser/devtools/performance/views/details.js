


"use strict";





let DetailsView = {
  



  components: {
    "waterfall": {
      id: "waterfall-view",
      view: WaterfallView,
      actors: ["timeline"],
      features: ["withMarkers"]
    },
    "js-calltree": {
      id: "js-profile-view",
      view: JsCallTreeView
    },
    "js-flamegraph": {
      id: "js-flamegraph-view",
      view: JsFlameGraphView,
      actors: ["timeline"]
    },
    "memory-calltree": {
      id: "memory-calltree-view",
      view: MemoryCallTreeView,
      actors: ["memory"],
      features: ["withAllocations"]
    },
    "memory-flamegraph": {
      id: "memory-flamegraph-view",
      view: MemoryFlameGraphView,
      actors: ["memory", "timeline"],
      features: ["withAllocations"]
    }
  },

  


  initialize: Task.async(function *() {
    this.el = $("#details-pane");
    this.toolbar = $("#performance-toolbar-controls-detail-views");

    this._onViewToggle = this._onViewToggle.bind(this);
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this.setAvailableViews = this.setAvailableViews.bind(this);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.addEventListener("command", this._onViewToggle);
    }

    yield this.setAvailableViews();

    PerformanceController.on(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.PREF_CHANGED, this.setAvailableViews);
  }),

  


  destroy: Task.async(function *() {
    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.removeEventListener("command", this._onViewToggle);
    }

    for (let [_, component] of Iterator(this.components)) {
      component.initialized && (yield component.view.destroy());
    }

    PerformanceController.off(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.PREF_CHANGED, this.setAvailableViews);
  }),

  




  setAvailableViews: Task.async(function* () {
    let recording = PerformanceController.getCurrentRecording();
    let isRecording = recording && recording.isRecording();
    let invalidCurrentView = false;

    for (let [name, { view }] of Iterator(this.components)) {
      let isSupported = this._isViewSupported(name, false);

      $(`toolbarbutton[data-view=${name}]`).hidden = !isSupported;

      
      
      if (!isSupported && this.isViewSelected(view)) {
        invalidCurrentView = true;
      }
    }

    
    
    
    
    
    
    
    
    if ((this._initialized  && !isRecording && invalidCurrentView) ||
        (!this._initialized && !isRecording && recording)) {
      yield this.selectDefaultView();
    }
  }),

  






  _isViewSupported: function (viewName, isRecording) {
    let { features, actors } = this.components[viewName];
    return PerformanceController.isFeatureSupported({ features, actors, isRecording });
  },

  






  selectView: Task.async(function *(viewName) {
    let component = this.components[viewName];
    this.el.selectedPanel = $("#" + component.id);

    yield this._whenViewInitialized(component);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      if (button.getAttribute("data-view") === viewName) {
        button.setAttribute("checked", true);
      } else {
        button.removeAttribute("checked");
      }
    }

    
    
    this._initialized = true;

    this.emit(EVENTS.DETAILS_VIEW_SELECTED, viewName);
  }),

  



  selectDefaultView: function () {
    
    
    
    if (this._isViewSupported("waterfall")) {
      return this.selectView("waterfall");
    } else {
      
      
      return this.selectView("js-calltree");
    }
  },

  





  isViewSelected: function(viewObject) {
    
    
    if (!this._initialized) {
      return false;
    }

    let selectedPanel = this.el.selectedPanel;
    let selectedId = selectedPanel.id;

    for (let [, { id, view }] of Iterator(this.components)) {
      if (id == selectedId && view == viewObject) {
        return true;
      }
    }

    return false;
  },

  






  whenViewSelected: Task.async(function*(viewObject) {
    if (this.isViewSelected(viewObject)) {
      return promise.resolve();
    }
    yield this.once(EVENTS.DETAILS_VIEW_SELECTED);
    return this.whenViewSelected(viewObject);
  }),

  






  _whenViewInitialized: Task.async(function *(component) {
    if (component.initialized) {
      return;
    }
    component.initialized = true;
    yield component.view.initialize();

    
    
    
    
    let recording = PerformanceController.getCurrentRecording();
    if (recording && !recording.isRecording()) {
      component.view.shouldUpdateWhenShown = true;
    }
  }),

  


  _onRecordingStoppedOrSelected: function(_, recording) {
    this.setAvailableViews();
  },

  


  _onViewToggle: function (e) {
    this.selectView(e.target.getAttribute("data-view"));
  },

  toString: () => "[object DetailsView]"
};




EventEmitter.decorate(DetailsView);
