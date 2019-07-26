



if (typeof XPCOMUtils == "undefined")
  Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
                                  "resource:///modules/CustomizableUI.jsm");
