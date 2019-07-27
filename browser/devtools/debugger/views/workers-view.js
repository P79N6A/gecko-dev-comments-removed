


"use strict";

function WorkersView() {}

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
  }
});

DebuggerView.Workers = new WorkersView();
