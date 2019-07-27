





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, $, DetailsView, document: doc } = panel.panelWin;

  info("views on startup");
  checkViews(DetailsView, doc, "waterfall");

  
  let viewChanged = onceSpread(DetailsView, EVENTS.DETAILS_VIEW_SELECTED);
  command($("toolbarbutton[data-view='calltree']"));
  let [_, viewName] = yield viewChanged;
  is(viewName, "calltree", "DETAILS_VIEW_SELECTED fired with view name");
  checkViews(DetailsView, doc, "calltree");

  
  viewChanged = onceSpread(DetailsView, EVENTS.DETAILS_VIEW_SELECTED);
  command($("toolbarbutton[data-view='waterfall']"));
  [_, viewName] = yield viewChanged;
  is(viewName, "waterfall", "DETAILS_VIEW_SELECTED fired with view name");
  checkViews(DetailsView, doc, "waterfall");


  yield teardown(panel);
  finish();
}

function checkViews (DetailsView, doc, currentView) {
  for (let viewName in DetailsView.views) {
    let view = DetailsView.views[viewName].el;
    let button = doc.querySelector("toolbarbutton[data-view='" + viewName + "']");

    if (viewName === currentView) {
      ok(!view.getAttribute("hidden"), view + " view displayed");
      ok(button.getAttribute("checked"), view + " button checked");
    } else {
      ok(view.getAttribute("hidden"), view + " view hidden");
      ok(!button.getAttribute("checked"), view + " button not checked");
    }
  }
}
