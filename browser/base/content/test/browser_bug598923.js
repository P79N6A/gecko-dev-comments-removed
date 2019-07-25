








function test() {
  let aml = AddonsMgrListener;
  ok(aml, "AddonsMgrListener exists");
  
  is(aml.addonBar.collapsed, true, "aob is hidden");
  
  AddonsMgrListener.onInstalling();
  
  let element = document.createElement("toolbaritem");
  aml.addonBar.appendChild(element);
  
  AddonsMgrListener.onInstalled();
  
  is(aml.addonBar.collapsed, false, "aob is visible");
  
  AddonsMgrListener.onUninstalling();
  
  aml.addonBar.removeChild(element);
  
  AddonsMgrListener.onUninstalled();
  
  is(aml.addonBar.collapsed, true, "aob is hidden");
}
