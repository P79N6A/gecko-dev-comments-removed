



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SelectContentHelper",
                                  "resource://gre/modules/SelectContentHelper.jsm");

addEventListener("mozshowdropdown", event => {
  if (!event.isTrusted)
    return;

  new SelectContentHelper(event.target, this);
});
