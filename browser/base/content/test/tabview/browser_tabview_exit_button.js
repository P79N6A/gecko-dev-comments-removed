




































function test() {
  waitForExplicitFinish();

  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);
    ok(TabView.isVisible(), "Tab View is visible");

    let contentWindow = document.getElementById("tab-view").contentWindow;
    let button = contentWindow.document.getElementById("exit-button");

    ok(button, "Exit button exists");
    EventUtils.sendMouseEvent({ type: "click" }, button, contentWindow);
  }

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden");
    finish();
  }

  window.addEventListener("tabviewshown", onTabViewShown, false);
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  TabView.toggle();
}
