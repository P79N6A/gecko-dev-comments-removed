



const {Cu} = require("chrome");
Cu.import("resource://gre/modules/Services.jsm");

let { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});
XPCOMUtils.defineLazyGetter(this, "PlatformKeys", function() {
  return Services.strings.createBundle(
    "chrome://global-platform/locale/platformKeys.properties");
});











exports.prettyKey = function Helpers_prettyKey(aElemKey, aAllowCloverleaf) {
  let elemString = "";
  let elemMod = aElemKey.getAttribute("modifiers");

  if (elemMod.match("accel")) {
    if (Services.appinfo.OS == "Darwin") {
      
      
      if (!aAllowCloverleaf) {
        elemString += "Cmd-";
      } else {
        elemString += PlatformKeys.GetStringFromName("VK_META") +
          PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
      }
    } else {
      elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
        PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }
  }
  if (elemMod.match("access")) {
    if (Services.appinfo.OS == "Darwin") {
      elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
        PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    } else {
      elemString += PlatformKeys.GetStringFromName("VK_ALT") +
        PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }
  }
  if (elemMod.match("shift")) {
    elemString += PlatformKeys.GetStringFromName("VK_SHIFT") +
      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
  }
  if (elemMod.match("alt")) {
    elemString += PlatformKeys.GetStringFromName("VK_ALT") +
      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
  }
  if (elemMod.match("ctrl") || elemMod.match("control")) {
    elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
  }
  if (elemMod.match("meta")) {
    elemString += PlatformKeys.GetStringFromName("VK_META") +
      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
  }

  return elemString +
    (aElemKey.getAttribute("keycode").replace(/^.*VK_/, "") ||
     aElemKey.getAttribute("key")).toUpperCase();
}
