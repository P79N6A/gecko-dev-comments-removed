


"use strict";

const DEFAULT_DETAILS_SUBVIEW = "waterfall";





let DetailsView = {
  


  components: {
    "waterfall": { id: "waterfall-view", view: WaterfallView },
    "js-calltree": { id: "js-calltree-view", view: JsCallTreeView },
    "js-flamegraph": { id: "js-flamegraph-view", view: JsFlameGraphView },
    "memory-calltree": { id: "memory-calltree-view", view: MemoryCallTreeView, pref: "enable-memory" },
    "memory-flamegraph": { id: "memory-flamegraph-view", view: MemoryFlameGraphView, pref: "enable-memory" }
  },

  


  initialize: Task.async(function *() {
    this.el = $("#details-pane");
    this.toolbar = $("#performance-toolbar-controls-detail-views");

    this._onViewToggle = this._onViewToggle.bind(this);
    this.setAvailableViews = this.setAvailableViews.bind(this);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.addEventListener("command", this._onViewToggle);
    }

    yield this.selectView(DEFAULT_DETAILS_SUBVIEW);
    yield this.setAvailableViews();

    PerformanceController.on(EVENTS.PREF_CHANGED, this.setAvailableViews);
  }),

  


  destroy: Task.async(function *() {
    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.removeEventListener("command", this._onViewToggle);
    }

    for (let [_, component] of Iterator(this.components)) {
      component.initialized && (yield component.view.destroy());
    }

    PerformanceController.off(EVENTS.PREF_CHANGED, this.setAvailableViews);
  }),

  




  setAvailableViews: Task.async(function* () {
    for (let [name, { view, pref }] of Iterator(this.components)) {
      if (!pref) {
        continue;
      }
      let value = PerformanceController.getPref(pref);
      $(`toolbarbutton[data-view=${name}]`).hidden = !value;

      
      
      if (!value && this.isViewSelected(view)) {
        yield this.selectView(DEFAULT_DETAILS_SUBVIEW);
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

  


  _onViewToggle: function (e) {
    this.selectView(e.target.getAttribute("data-view"));
  },

  toString: () => "[object DetailsView]"
};




EventEmitter.decorate(DetailsView);
