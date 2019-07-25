var setupTest = function() {
  controller = mozmill.getBrowserController();
}

var testExpectedEvent = function() {
  controller.open("http://www.mozilla.com/en-US/");
  controller.waitForPageLoad();

  var search = new elementslib.ID(controller.tabs.activeTab, "query");
  var submit = new elementslib.ID(controller.tabs.activeTab, "submit");

  
  controller.click(search, 2, 2, {type: "focus"});

  
  controller.keypress(search, "F", {});
  controller.keypress(search, "i", {}, {type: "keypress"});

  
  controller.type(search, "ref");
  controller.type(search, "ox", {type: "keypress"});

  
  controller.keypress(search, "a", {accelKey: true}, {type: "keypress"});

  
  controller.keypress(null, "VK_TAB", {}, {type: "focus", target: submit});

  
  try {
    controller.rightClick(submit, 2, 2, {type: "click", target: submit});
    throw new Error("Opening a context menu has raised a click event.");
  } catch (ex) {
  }

  
  controller.rightClick(submit, 2, 2, {type: "contextmenu", target: submit});

  
  var catched = true;
  try {
    controller.keypress(null, "VK_TAB", {}, {target: submit});
    catched = false;
  } catch (ex) {}

  if (!catched) {
    throw new Error("Missing event type should cause a failure.")
  }
}

