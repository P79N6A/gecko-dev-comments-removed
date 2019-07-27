


"use strict";

const DEFAULT_DETAILS_SUBVIEW = "waterfall";





let DetailsView = {
  


  components: {
    waterfall: { index: 0, view: WaterfallView },
    calltree: { index: 1, view: CallTreeView },
    flamegraph: { index: 2, view: FlameGraphView }
  },

  


  initialize: Task.async(function *() {
    this.el = $("#details-pane");
    this.toolbar = $("#performance-toolbar-controls-detail-views");

    this._onViewToggle = this._onViewToggle.bind(this);

    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.addEventListener("command", this._onViewToggle);
    }

    yield WaterfallView.initialize();
    yield CallTreeView.initialize();
    yield FlameGraphView.initialize();

    this.selectView(DEFAULT_DETAILS_SUBVIEW);
  }),

  


  destroy: Task.async(function *() {
    for (let button of $$("toolbarbutton[data-view]", this.toolbar)) {
      button.removeEventListener("command", this._onViewToggle);
    }

    yield WaterfallView.destroy();
    yield CallTreeView.destroy();
    yield FlameGraphView.destroy();
  }),

  






  selectView: function (viewName) {
    this.el.selectedIndex = this.components[viewName].index;

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
    let selectedIndex = this.el.selectedIndex;

    for (let [, { index, view }] of Iterator(this.components)) {
      if (index == selectedIndex && view == viewObject) {
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
