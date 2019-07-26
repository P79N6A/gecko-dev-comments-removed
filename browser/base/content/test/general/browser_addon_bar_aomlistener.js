



function test() {

  let addonbar = document.getElementById("addon-bar");
  ok(addonbar.collapsed, "addon bar is collapsed by default");

  function addItem(id) {
    let button = document.createElement("toolbarbutton");
    button.id = id;
    let palette = document.getElementById("navigator-toolbox").palette;
    palette.appendChild(button);
    addonbar.insertItem(id, null, null, false);
  }

  
  AddonsMgrListener.onInstalling();

  
  let id = "testbutton";
  addItem(id);

  
  AddonsMgrListener.onInstalled();

  
  ok(!addonbar.collapsed, "addon bar is not collapsed after toggle");

  
  AddonsMgrListener.onUninstalling();

  
  addonbar.currentSet = addonbar.currentSet.replace("," + id, "");

  
  AddonsMgrListener.onUninstalled();

  
  ok(addonbar.collapsed, "addon bar is collapsed after toggle");

  
  AddonsMgrListener.onEnabling();

  
  let id = "testbutton";
  addItem(id);

  
  AddonsMgrListener.onEnabled();

  
  ok(!addonbar.collapsed, "addon bar is not collapsed after toggle");

  
  AddonsMgrListener.onDisabling();

  
  addonbar.currentSet = addonbar.currentSet.replace("," + id, "");

  
  AddonsMgrListener.onDisabled();

  
  ok(addonbar.collapsed, "addon bar is collapsed after toggle");
}
