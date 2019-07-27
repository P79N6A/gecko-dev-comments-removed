





"use strict";

let Services = require("Services");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "DebuggerSocket",
  "devtools/toolkit/security/socket", true);

DevToolsUtils.defineLazyGetter(this, "bundle", () => {
  const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";
  return Services.strings.createBundle(DBG_STRINGS_URI);
});

let Server = exports.Server = {};









Server.defaultAllowConnection = () => {
  let title = bundle.GetStringFromName("remoteIncomingPromptTitle");
  let msg = bundle.GetStringFromName("remoteIncomingPromptMessage");
  let disableButton = bundle.GetStringFromName("remoteIncomingPromptDisable");
  let prompt = Services.prompt;
  let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_OK +
              prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_CANCEL +
              prompt.BUTTON_POS_2 * prompt.BUTTON_TITLE_IS_STRING +
              prompt.BUTTON_POS_1_DEFAULT;
  let result = prompt.confirmEx(null, title, msg, flags, null, null,
                                disableButton, null, { value: false });
  if (result === 0) {
    return true;
  }
  if (result === 2) {
    
    
    Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
  }
  return false;
};
