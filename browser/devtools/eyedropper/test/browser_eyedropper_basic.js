



const TESTCASE_URI = TEST_BASE + "color-block.html";
const DIV_COLOR = "#0000FF";






let test = asyncTest(function*() {
  yield addTab(TESTCASE_URI);

  info("added tab");

  yield testEscape();

  info("testing selecting a color");

  yield testSelect();
});

function* testEscape() {
  let dropper = new Eyedropper(window);

  yield inspectPage(dropper, false);

  let destroyed = dropper.once("destroy");
  pressESC();
  yield destroyed;

  ok(true, "escape closed the eyedropper");
}

function* testSelect() {
  let dropper = new Eyedropper(window);

  let selected = dropper.once("select");
  let copied = waitForClipboard(() => {}, DIV_COLOR);

  yield inspectPage(dropper);

  let color = yield selected;
  is(color, DIV_COLOR, "correct color selected");

  
  yield copied;
}



function* inspectPage(dropper, click=true) {
  yield dropper.open();

  info("dropper opened");

  let target = document.documentElement;
  let win = window;

  
  let box = gBrowser.selectedTab.linkedBrowser.getBoundingClientRect();
  let x = box.left + 100;
  let y = box.top + 100;

  EventUtils.synthesizeMouse(target, x, y, { type: "mousemove" }, win);

  yield dropperLoaded(dropper);

  EventUtils.synthesizeMouse(target, x + 10, y + 10, { type: "mousemove" }, win);

  if (click) {
    EventUtils.synthesizeMouse(target, x + 10, y + 10, {}, win);
  }
}

function pressESC() {
  EventUtils.synthesizeKey("VK_ESCAPE", { });
}
