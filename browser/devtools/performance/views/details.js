


"use strict";

const DEFAULT_DETAILS_SUBVIEW = "waterfall";





let DetailsView = {
  


  viewIndexes: {
    waterfall: 0,
    calltree: 1
  },

  



  initialize: Task.async(function *() {
    this.el = $("#details-pane");

    this._onViewToggle = this._onViewToggle.bind(this);

    for (let button of $$("toolbarbutton[data-view]", $("#details-toolbar"))) {
      button.addEventListener("command", this._onViewToggle);
    }

    yield CallTreeView.initialize();
    yield WaterfallView.initialize();
    this.selectView(DEFAULT_DETAILS_SUBVIEW);
  }),

  






  selectView: function (selectedView) {
    this.el.selectedIndex = this.viewIndexes[selectedView];

    for (let button of $$("toolbarbutton[data-view]", $("#details-toolbar"))) {
      if (button.getAttribute("data-view") === selectedView)
        button.setAttribute("checked", true);
      else
        button.removeAttribute("checked");
    }

    this.emit(EVENTS.DETAILS_VIEW_SELECTED, selectedView);
  },

  


  _onViewToggle: function (e) {
    this.selectView(e.target.getAttribute("data-view"));
  },

  


  destroy: Task.async(function *() {
    for (let button of $$("toolbarbutton[data-view]", $("#details-toolbar"))) {
      button.removeEventListener("command", this._onViewToggle);
    }

    yield CallTreeView.destroy();
    yield WaterfallView.destroy();
  })
};




EventEmitter.decorate(DetailsView);
