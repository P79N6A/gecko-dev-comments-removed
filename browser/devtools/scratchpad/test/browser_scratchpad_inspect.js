



function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html;charset=utf8,<p>test inspect() in Scratchpad</p>";
}

function runTests()
{
  let sp = gScratchpadWindow.Scratchpad;

  sp.setText("({ a: 'foobarBug636725' })");

  sp.inspect().then(function() {
    let sidebar = sp.sidebar;
    ok(sidebar.visible, "sidebar is open");


    let found = false;

    outer: for (let scope in sidebar.variablesView) {
      for (let [, obj] in scope) {
        for (let [, prop] in obj) {
          if (prop.name == "a" && prop.value == "foobarBug636725") {
            found = true;
            break outer;
          }
        }
      }
    }

    ok(found, "found the property");

    finish();
  });
}