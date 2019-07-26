



function test() {
  let addonbar = document.getElementById("addon-bar");
  ok(addonbar.collapsed, "addon bar is collapsed by default");

  
  EventUtils.synthesizeKey("/", { accelKey: true }, window);
  ok(!addonbar.collapsed, "addon bar is not collapsed after toggle");

  
  EventUtils.synthesizeKey("/", { accelKey: true }, window);

  
  ok(addonbar.collapsed, "addon bar is collapsed after toggle");
}
