



const TESTCASE_URI = TEST_BASE + "color-block.html";
const DIV_COLOR = "#0000FF";






function test() {
  addTab(TESTCASE_URI).then(testEscape);
}

function testEscape() {
  let dropper = new Eyedropper(window);

  dropper.once("destroy", (event) => {
    ok(true, "escape closed the eyedropper");

    
    testSelect();
  });

  inspectPage(dropper, false).then(pressESC);
}

function testSelect() {
  let dropper = new Eyedropper(window);

  dropper.once("select", (event, color) => {
    is(color, DIV_COLOR, "correct color selected");
  });

  
  waitForClipboard(DIV_COLOR, () => {
    inspectPage(dropper); 
  }, finish, finish);
}



function inspectPage(dropper, click=true) {
  dropper.open();

  let target = content.document.getElementById("test");
  let win = content.window;

  EventUtils.synthesizeMouse(target, 20, 20, { type: "mousemove" }, win);

  return dropperLoaded(dropper).then(() => {
    EventUtils.synthesizeMouse(target, 30, 30, { type: "mousemove" }, win);

    if (click) {
      EventUtils.synthesizeMouse(target, 30, 30, {}, win);
    }
  });
}

function pressESC() {
  EventUtils.synthesizeKey("VK_ESCAPE", { });
}
