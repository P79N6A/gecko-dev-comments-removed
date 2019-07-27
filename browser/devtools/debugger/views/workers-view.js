


"use strict";

function WorkersView() {
  this._onWorkerSelect = this._onWorkerSelect.bind(this);
}

WorkersView.prototype = Heritage.extend(WidgetMethods, {
  initialize: function () {
    if (!Prefs.workersEnabled) {
      return;
    }

    document.getElementById("workers-pane").removeAttribute("hidden");

    this.widget = new SideMenuWidget(document.getElementById("workers"), {
      showArrows: true,
    });
    this.emptyText = L10N.getStr("noWorkersText");
    this.widget.addEventListener("select", this._onWorkerSelect, false);
  },

  addWorker: function (actor, name) {
    let element = document.createElement("label");
    element.className = "plain dbg-worker-item";
    element.setAttribute("value", name);
    element.setAttribute("flex", "1");

    this.push([element, actor], {});
  },

  removeWorker: function (actor) {
    this.remove(this.getItemByValue(actor));
  },

  _onWorkerSelect: function () {
    if (this.selectedItem !== null) {
      DebuggerController.Workers._onWorkerSelect(this.selectedItem.value);
      this.selectedItem = null;
    }
  }
});

DebuggerView.Workers = new WorkersView();
