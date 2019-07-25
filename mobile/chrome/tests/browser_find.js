



function test() {
  let menu = document.getElementById("identity-container");
  let item = document.getElementById("pageaction-findinpage");
  let navigator = document.getElementById("content-navigator");

  

  getIdentityHandler().show();
  ok(!menu.hidden, "Site menu is open");
  ok(!navigator.isActive, "Toolbar is closed");

  EventUtils.sendMouseEvent({ type: "click" }, item);
  ok(menu.hidden, "Site menu is closed");
  ok(navigator.isActive, "Toolbar is open");

  is(navigator._previousButton.disabled, true, "Previous button should be disabled");
  is(navigator._nextButton.disabled, true, "Previous button should be disabled");

  EventUtils.synthesizeKey("VK_ESCAPE", {}, window);
  ok(menu.hidden, "Site menu is closed");
  ok(!navigator.isActive, "Toolbar is closed");
}
