



"use strict";

this.EXPORTED_SYMBOLS = ["ShortcutUtils"];

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "PlatformKeys", function() {
  return Services.strings.createBundle(
    "chrome://global-platform/locale/platformKeys.properties");
});

XPCOMUtils.defineLazyGetter(this, "Keys", function() {
  return Services.strings.createBundle(
    "chrome://global/locale/keys.properties");
});

let ShortcutUtils = {
  









  prettifyShortcut: function(aElemKey, aAllowCloverleaf) {
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
    if (elemMod.match("os")) {
      elemString += PlatformKeys.GetStringFromName("VK_WIN") +
        PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
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

    let key;
    let keyCode = aElemKey.getAttribute("keycode");
    if (keyCode) {
      try {
        
        key = Keys.GetStringFromName(keyCode.toUpperCase());
      } catch (ex) {
        Cu.reportError("Error finding " + keyCode + ": " + ex);
        key = keyCode.replace(/^VK_/, '');
      }
    } else {
      key = aElemKey.getAttribute("key");
    }
    return elemString + key;
  }
};

Object.freeze(ShortcutUtils);
