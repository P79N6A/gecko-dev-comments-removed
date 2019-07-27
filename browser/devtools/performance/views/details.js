


"use strict";





let DetailsView = {
  



  components: {
    "waterfall": {
      id: "waterfall-view",
      view: WaterfallView,
      requires: ["timeline"]
    },
    "js-calltree": {
      id: "js-profile-view",
      view: JsCallTreeView
    },
    "js-flamegraph": {
      id: "js-flamegraph-view",
      view: JsFlameGraphView,
      requires: ["timeline"]
    },
    "memory-calltree": {
      id: "memory-calltree-view",
      view: MemoryCallTreeView,
      requires: ["memory"],
      pref: "enable-memory"
    },
    "memory-flamegraph": {
      id: "memory-flamegraph-view",
      view: MemoryFlameGraphView,
      requires: ["memory", "timeline"],
      pref: "enable-memory"
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

    yield this.selectDefaultView();
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
    let mocks = gFront.getMocksInUse();

    for (let [name, { view, pref, requires }] of Iterator(this.components)) {
      let recording = PerformanceController.getCurrentRecording();

      let isRecorded = recording && !recording.isRecording();
      
      let isEnabled = !pref || PerformanceController.getOption(pref);
      
      let isSupported = !requires || requires.every(r => !mocks[r]);

      $(`toolbarbutton[data-view=${name}]`).hidden = !isRecorded || !(isEnabled && isSupported);

      
      
      if (!isEnabled && this.isViewSelected(view)) {
        yield this.selectDefaultView();
      }
    }
  }),

  






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

    this.emit(EVENTS.DETAILS_VIEW_SELECTED, viewName);
  }),

  



  selectDefaultView: function () {
    let { timeline: mockTimeline } = gFront.getMocksInUse();
    
    if (mockTimeline) {
      return this.selectView("js-calltree");
    } else {
      
      
      return this.selectView("waterfall");
    }
  },

  





  isViewSelected: function(viewObject) {
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
