




const TESTCASE_URI = TEST_BASE + "color-block.html";
const DIV_COLOR = "#0000FF";

function test() {
  return Task.spawn(spawnTest).then(finish, helpers.handleError);
}

function spawnTest() {
  let options = yield helpers.openTab(TESTCASE_URI);
  yield helpers.openToolbar(options);

  yield helpers.audit(options, [
    {
      setup: "eyedropper",
      check: {
        input: "eyedropper"
      },
      exec: { output: "" }
    },
  ]);

  yield inspectAndWaitForCopy();

  yield helpers.closeToolbar(options);
  yield helpers.closeTab(options);
}

function inspectAndWaitForCopy() {
  return waitForClipboard(() => {
    inspectPage(); 
  }, DIV_COLOR);
}

function inspectPage() {
  let target = document.documentElement;
  let win = window;

  
  let box = gBrowser.selectedTab.linkedBrowser.getBoundingClientRect();
  let x = box.left + 100;
  let y = box.top + 100;

  let dropper = EyedropperManager.getInstance(window);

  return dropperStarted(dropper).then(() => {
    EventUtils.synthesizeMouse(target, x, y, { type: "mousemove" }, win);

    return dropperLoaded(dropper).then(() => {
      EventUtils.synthesizeMouse(target, x + 10, y + 10, { type: "mousemove" }, win);

      EventUtils.synthesizeMouse(target, x + 10, y + 10, {}, win);
    });
  })
}
