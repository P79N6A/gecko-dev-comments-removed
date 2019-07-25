





"use strict";

function test() {
  if (!isTiltEnabled()) {
    info("Skipping highlight test because Tilt isn't enabled.");
    return;
  }
  if (!isWebGLSupported()) {
    info("Skipping highlight test because WebGL isn't supported.");
    return;
  }

  waitForExplicitFinish();

  createTab(function() {
    createTilt({
      onTiltOpen: function(instance)
      {
        let presenter = instance.presenter;
        let canvas = presenter.canvas;

        presenter.onSetupMesh = function() {

          presenter.highlightNodeAt(canvas.width / 2, canvas.height / 2, {
            onpick: function()
            {
              ok(presenter._currentSelection > 0,
                "Highlighting a node didn't work properly.");
              ok(!presenter.highlight.disabled,
                "After highlighting a node, it should be highlighted. D'oh.");

              Services.obs.addObserver(cleanup, TILT_DESTROYED, false);
              InspectorUI.closeInspectorUI();
            }
          });
        };
      }
    });
  });
}

function cleanup() {
  Services.obs.removeObserver(cleanup, TILT_DESTROYED);
  gBrowser.removeCurrentTab();
  finish();
}
