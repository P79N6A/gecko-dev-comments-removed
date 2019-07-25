


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    let cw = win.TabView.getContentWindow();

    win.addEventListener("SSWindowClosing", function onClose() {
      win.removeEventListener("SSWindowClosing", onClose);

      executeSoon(function () {
        ok(cw.UI.isDOMWindowClosing, "dom window is closing");
        waitForFocus(finish);
      });
    });

    win.close();
  });
}
