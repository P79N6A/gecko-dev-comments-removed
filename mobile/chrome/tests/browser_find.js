



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

  EventUtils.synthesizeKey("VK_ESCAPE", {}, window);
  ok(menu.hidden, "Site menu is closed");
  ok(!navigator.isActive, "Toolbar is closed");
}
