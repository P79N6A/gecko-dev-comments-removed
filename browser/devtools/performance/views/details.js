


"use strict";

const DEFAULT_DETAILS_SUBVIEW = "waterfall";





let DetailsView = {
  


  components: {
    waterfall: { id: "waterfall-view", view: WaterfallView },
    calltree: { id: "calltree-view", view: CallTreeView },
    flamegraph: { id: "flamegraph-view", view: FlameGraphView }
  },

  


  initialize: Task.async(function *() {
    this.el = $("#details-pane");
    this.toolbar = $("#performance-toolbar-controls-detail-views");

    this._onViewToggle = this._onViewToggle.bind(this);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.addEventListener("command", this._onViewToggle);
    }

    for (let [_, { view }] of Iterator(this.components)) {
      yield view.initialize();
    }

    this.selectView(DEFAULT_DETAILS_SUBVIEW);
  }),

  


  destroy: Task.async(function *() {
    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.removeEventListener("command", this._onViewToggle);
    }

    for (let [_, { view }] of Iterator(this.components)) {
      yield view.destroy();
    }
  }),

  






  selectView: function (viewName) {
    this.el.selectedPanel = $("#" + this.components[viewName].id);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      if (button.getAttribute("data-view") === viewName) {
        button.setAttribute("checked", true);
      } else {
        button.removeAttribute("checked");
      }
    }

    this.emit(EVENTS.DETAILS_VIEW_SELECTED, viewName);
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

  


  _onViewToggle: function (e) {
    this.selectView(e.target.getAttribute("data-view"));
  }
};




EventEmitter.decorate(DetailsView);
