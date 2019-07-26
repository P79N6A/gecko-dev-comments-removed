





let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

function test() {
  const options = {
    tabContent: "test closing toolbox and then reusing scratchpad"
  };
  openTabAndScratchpad(options)
    .then(Task.async(runTests))
    .then(finish, console.error);
}

function* runTests([win, sp]) {
  
  const source = "window.foobar = 7;";
  sp.setText(source);
  let [,,result] = yield sp.display();
  is(result, 7, "Display produced the expected output.");

  
  let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "webconsole");
  ok(toolbox, "Toolbox was opened.");
  let closed = yield gDevTools.closeToolbox(target);
  is(closed, true, "Toolbox was closed.");

  
  sp.setText(source);
  let [,,result2] = yield sp.display();
  is(result2, 7,
     "Display produced the expected output after the toolbox was gone.");
}
