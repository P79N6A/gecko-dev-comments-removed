


"use strict";





let AutomationView = {

  


  initialize: function () {
    this._buttons = $("#automation-param-toolbar-buttons");
    this.graph = new LineGraphWidget($("#automation-graph"), { avg: false });
    this.graph.selectionEnabled = false;

    this._onButtonClick = this._onButtonClick.bind(this);
    this._onNodeSet = this._onNodeSet.bind(this);
    this._onResize = this._onResize.bind(this);

    this._buttons.addEventListener("click", this._onButtonClick);
    window.on(EVENTS.UI_INSPECTOR_RESIZE, this._onResize);
    window.on(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSet);
  },

  


  destroy: function () {
    this._buttons.removeEventListener("click", this._onButtonClick);
    window.off(EVENTS.UI_INSPECTOR_RESIZE, this._onResize);
    window.off(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSet);
  },

  


  resetUI: function () {
    this._currentNode = null;
  },

  



  build: Task.async(function* () {
    let node = this._currentNode;

    let props = yield node.getParams();
    let params = props.filter(({ flags }) => flags && flags.param);

    this._createParamButtons(params);

    this._selectedParamName = params[0] ? params[0].param : null;
    this.render();
  }),

  




  render: Task.async(function *() {
    let node = this._currentNode;
    let paramName = this._selectedParamName;
    
    if (!node || !paramName) {
      this._setState("no-params");
      window.emit(EVENTS.UI_AUTOMATION_TAB_RENDERED, null);
      return;
    }

    let { values, events } = yield node.getAutomationData(paramName);
    this._setState(events.length ? "show" : "no-events");
    yield this.graph.setDataWhenReady(values);
    window.emit(EVENTS.UI_AUTOMATION_TAB_RENDERED, node.id);
  }),

  



  _createParamButtons: function (params) {
    this._buttons.innerHTML = "";
    params.forEach((param, i) => {
      let button = document.createElement("toolbarbutton");
      button.setAttribute("class", "devtools-toolbarbutton automation-param-button");
      button.setAttribute("data-param", param.param);
      
      button.setAttribute("label", param.param);

      
      if (i === 0) {
        button.setAttribute("selected", true);
      }

      this._buttons.appendChild(button);
    });
  },

  



  _setAudioNode: function (node) {
    this._currentNode = node;
    if (this._currentNode) {
      this.build();
    }
  },

  




  _setState: function (state) {
    let contentView = $("#automation-content");
    let emptyView = $("#automation-empty");

    let graphView = $("#automation-graph-container");
    let noEventsView = $("#automation-no-events");

    contentView.hidden = state === "no-params";
    emptyView.hidden = state !== "no-params";

    graphView.hidden = state !== "show";
    noEventsView.hidden = state !== "no-events";
  },

  



  _onButtonClick: function (e) {
    Array.forEach($$(".automation-param-button"), $btn => $btn.removeAttribute("selected"));
    let paramName = e.target.getAttribute("data-param");
    e.target.setAttribute("selected", true);
    this._selectedParamName = paramName;
    this.render();
  },

  


  _onResize: function () {
    this.graph.refresh();
  },

  


  _onNodeSet: function (_, id) {
    this._setAudioNode(id != null ? gAudioNodes.get(id) : null);
  }
};
