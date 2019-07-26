



"use strict";




let test = asyncTest(function* () {

  info ("Testing tab closure when projecteditor is in various states");
  let loaderUrl = "chrome://browser/content/devtools/projecteditor-test.xul";

  yield addTab(loaderUrl).then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    ok (iframe, "Tab has placeholder iframe for projecteditor");

    info ("Closing the tab without doing anything");
    gBrowser.removeCurrentTab();
  });

  yield addTab(loaderUrl).then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    ok (iframe, "Tab has placeholder iframe for projecteditor");

    let projecteditor = ProjectEditor.ProjectEditor();
    ok (projecteditor, "ProjectEditor has been initialized");

    info ("Closing the tab before attempting to load");
    gBrowser.removeCurrentTab();
  });

  yield addTab(loaderUrl).then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    ok (iframe, "Tab has placeholder iframe for projecteditor");

    let projecteditor = ProjectEditor.ProjectEditor();
    ok (projecteditor, "ProjectEditor has been initialized");

    projecteditor.load(iframe);

    info ("Closing the tab after a load is requested, but before load is finished");
    gBrowser.removeCurrentTab();
  });

  yield addTab(loaderUrl).then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    ok (iframe, "Tab has placeholder iframe for projecteditor");

    let projecteditor = ProjectEditor.ProjectEditor();
    ok (projecteditor, "ProjectEditor has been initialized");

    return projecteditor.load(iframe).then(() => {
      info ("Closing the tab after a load has been requested and finished");
      gBrowser.removeCurrentTab();
    });
  });

  yield addTab(loaderUrl).then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    ok (iframe, "Tab has placeholder iframe for projecteditor");

    let projecteditor = ProjectEditor.ProjectEditor(iframe);
    ok (projecteditor, "ProjectEditor has been initialized");

    let loadedDone = promise.defer();
    projecteditor.loaded.then(() => {
      ok (false, "Loaded has finished after destroy() has been called");
      loadedDone.resolve();
    }, () => {
      ok (true, "Loaded has been rejected after destroy() has been called");
      loadedDone.resolve();
    });

    projecteditor.destroy();

    return loadedDone.promise.then(() => {
      gBrowser.removeCurrentTab();
    });
  });

  finish();
});


