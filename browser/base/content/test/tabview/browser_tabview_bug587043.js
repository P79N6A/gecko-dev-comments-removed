


function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(TabView.isVisible(), "Tab View is visible");

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    ok(!TabView.isVisible(), "Tab View is hidden");

    finish();
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  document.getElementById("cmd_closeWindow").doCommand();
}
