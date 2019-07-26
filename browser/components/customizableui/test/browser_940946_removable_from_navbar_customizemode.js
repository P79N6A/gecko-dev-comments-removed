



const kTestBtnId = "test-removable-navbar-customize-mode";

let gTests = [
  {
    desc: "Items without the removable attribute in the navbar should be considered non-removable",
    setup: function() {
      let btn = createDummyXULButton(kTestBtnId, "Test removable in navbar in customize mode");
      document.getElementById("nav-bar").customizationTarget.appendChild(btn);
      return startCustomizing();
    },
    run: function() {
      ok(!CustomizableUI.isWidgetRemovable(kTestBtnId), "Widget should not be considered removable");
    },
    teardown: function() {
      yield endCustomizing();
      document.getElementById(kTestBtnId).remove();
    }
  },
];
function asyncCleanup() {
  yield endCustomizing();
  yield resetCustomization();
}

function test() {
  waitForExplicitFinish();
  runTests(gTests, asyncCleanup);
}

