



































function test() {
  let addonbar = document.getElementById("addon-bar");
  ok(addonbar.collapsed, "addon bar is collapsed by default");

  
  setToolbarVisibility(addonbar, true);
  ok(!addonbar.collapsed, "addon bar is not collapsed after toggle");

  
  let closeButton = document.getElementById("addonbar-closebutton");
  EventUtils.synthesizeMouseAtCenter(closeButton, {});

  
  ok(addonbar.collapsed, "addon bar is collapsed after clicking close button");
}
