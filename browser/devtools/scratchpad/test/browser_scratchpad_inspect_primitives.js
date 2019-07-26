





let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

function test() {
  const options = {
    tabContent: "test inspecting primitive values"
  };
  openTabAndScratchpad(options)
    .then(Task.async(runTests))
    .then(finish, console.error);
}

function* runTests([win, sp]) {
  
  yield checkResults(sp, 7);

  
  yield checkResults(sp, "foobar", true);

  
  yield checkResults(sp, true);
}


let checkResults = Task.async(function* (sp, value, isString = false) {
  let sourceValue = value;
  if (isString) {
    sourceValue = '"' + value + '"';
  }
  let source = "var foobar = " + sourceValue + "; foobar";
  sp.setText(source);
  yield sp.inspect();

  let sidebar = sp.sidebar;
  ok(sidebar.visible, "sidebar is open");

  let found = false;

  outer: for (let scope of sidebar.variablesView) {
    for (let [, obj] of scope) {
      for (let [, prop] of obj) {
        if (prop.name == "value" && prop.value == value) {
          found = true;
          break outer;
        }
      }
    }
  }

  ok(found, "found the value of " + value);

  let tabbox = sidebar._sidebar._tabbox;
  ok(!tabbox.hasAttribute("hidden"), "Scratchpad sidebar visible");
  sidebar.hide();
  ok(tabbox.hasAttribute("hidden"), "Scratchpad sidebar hidden");
});
