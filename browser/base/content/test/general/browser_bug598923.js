








function test() {
  let aml = AddonsMgrListener;
  ok(aml, "AddonsMgrListener exists");
  
  is(aml.addonBar.collapsed, true, "add-on bar is hidden initially");
  
  AddonsMgrListener.onInstalling();
  
  let element = document.createElement("toolbaritem");
  element.id = "bug598923-addon-item";
  aml.addonBar.appendChild(element);
  
  AddonsMgrListener.onInstalled();
  
  is(aml.addonBar.collapsed, false, "add-on bar has been made visible");
  
  AddonsMgrListener.onUninstalling();
  
  aml.addonBar.removeChild(element);
  
  AddonsMgrListener.onUninstalled();
  
  is(aml.addonBar.collapsed, true, "add-on bar is hidden again");
}
