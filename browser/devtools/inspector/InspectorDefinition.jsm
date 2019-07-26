





"use strict";

this.EXPORTED_SYMBOLS = ["InspectorDefinition"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
const properties = "chrome://browser/locale/devtools/inspector.properties";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "osString",
  function() Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS);

XPCOMUtils.defineLazyGetter(this, "Strings",
  function() Services.strings.createBundle(properties));

function l10n(aName) Strings.GetStringFromName(aName);

Cu.import("resource:

XPCOMUtils.defineLazyModuleGetter(this, "InspectorPanel", "resource:///modules/devtools/InspectorPanel.jsm");

this.InspectorDefinition = {
  id: "inspector",
  accesskey: l10n("inspector.accesskey"),
  key: l10n("inspector.commandkey"),
  ordinal: 2,
  modifiers: osString == "Darwin" ? "accel,alt" : "accel,shift",
  icon: "chrome://browser/skin/devtools/tools-icons-small.png",
  url: "chrome://browser/content/devtools/inspector/inspector.xul",
  label: l10n("inspector.label"),

  isTargetSupported: function(target) {
    return !target.isRemote;
  },

  build: function(iframeWindow, toolbox) {
    return new InspectorPanel(iframeWindow, toolbox);
  }
};
