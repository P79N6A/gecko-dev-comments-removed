





"use strict";

function Startup() {
  var bundle = document.getElementById("extensionsStrings");
  var addon = window.arguments[0].addon;

  document.documentElement.setAttribute("addontype", addon.type);

  if (addon.iconURL)
    document.getElementById("icon").src = addon.iconURL;

  var label = document.createTextNode(bundle.getFormattedString("eulaHeader", [addon.name]));
  document.getElementById("heading").appendChild(label);
  document.getElementById("eula").value = addon.eula;
}
