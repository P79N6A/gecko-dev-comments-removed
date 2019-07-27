





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, $, DetailsView, document: doc } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel);

  info("views on startup");
  checkViews(DetailsView, doc, "waterfall");

  
  let viewChanged = onceSpread(DetailsView, EVENTS.DETAILS_VIEW_SELECTED);
  command($("toolbarbutton[data-view='js-calltree']"));
  let [_, viewName] = yield viewChanged;
  is(viewName, "js-calltree", "DETAILS_VIEW_SELECTED fired with view name");
  checkViews(DetailsView, doc, "js-calltree");

  
  viewChanged = onceSpread(DetailsView, EVENTS.DETAILS_VIEW_SELECTED);
  command($("toolbarbutton[data-view='js-flamegraph']"));
  [_, viewName] = yield viewChanged;
  is(viewName, "js-flamegraph", "DETAILS_VIEW_SELECTED fired with view name");
  checkViews(DetailsView, doc, "js-flamegraph");

  
  viewChanged = onceSpread(DetailsView, EVENTS.DETAILS_VIEW_SELECTED);
  command($("toolbarbutton[data-view='waterfall']"));
  [_, viewName] = yield viewChanged;
  is(viewName, "waterfall", "DETAILS_VIEW_SELECTED fired with view name");
  checkViews(DetailsView, doc, "waterfall");

  yield teardown(panel);
  finish();
}

function checkViews (DetailsView, doc, currentView) {
  for (let viewName in DetailsView.components) {
    let button = doc.querySelector(`toolbarbutton[data-view="${viewName}"]`);

    is(DetailsView.el.selectedPanel.id, DetailsView.components[currentView].id,
      `DetailsView correctly has ${currentView} selected.`);
    if (viewName === currentView) {
      ok(button.getAttribute("checked"), `${viewName} button checked`);
    } else {
      ok(!button.getAttribute("checked"), `${viewName} button not checked`);
    }
  }
}
