





"use strict";

let { Ci } = require("chrome");
let Services = require("Services");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
loader.lazyRequireGetter(this, "DebuggerSocket",
  "devtools/toolkit/security/socket", true);
loader.lazyRequireGetter(this, "AuthenticationResult",
  "devtools/toolkit/security/auth", true);

DevToolsUtils.defineLazyGetter(this, "bundle", () => {
  const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";
  return Services.strings.createBundle(DBG_STRINGS_URI);
});

let Client = exports.Client = {};
let Server = exports.Server = {};

























Client.defaultSendOOB = ({ authResult, oob }) => {
  
  if (authResult != AuthenticationResult.PENDING) {
    throw new Error("Expected PENDING result, got " + authResult);
  }
  let title = bundle.GetStringFromName("clientSendOOBTitle");
  let header = bundle.GetStringFromName("clientSendOOBHeader");
  let hashMsg = bundle.formatStringFromName("clientSendOOBHash",
                                            [oob.sha256], 1);
  let token = oob.sha256.replace(/:/g, "").toLowerCase() + oob.k;
  let tokenMsg = bundle.formatStringFromName("clientSendOOBToken",
                                             [token], 1);
  let msg =`${header}\n\n${hashMsg}\n${tokenMsg}`;
  let prompt = Services.prompt;
  let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_CANCEL;

  
  let promptWindow;
  let windowListener = {
    onOpenWindow(xulWindow) {
      let win = xulWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindow);
      win.addEventListener("load", function listener() {
        win.removeEventListener("load", listener, false);
        if (win.document.documentElement.getAttribute("id") != "commonDialog") {
          return;
        }
        
        promptWindow = win;
        Services.wm.removeListener(windowListener);
      }, false);
    },
    onCloseWindow() {},
    onWindowTitleChange() {}
  };
  Services.wm.addListener(windowListener);

  
  DevToolsUtils.executeSoon(() => {
    prompt.confirmEx(null, title, msg, flags, null, null, null, null,
                     { value: false });
  });

  return {
    close() {
      if (!promptWindow) {
        return;
      }
      promptWindow.document.documentElement.acceptDialog();
      promptWindow = null;
    }
  };
};

























Server.defaultAllowConnection = ({ client, server }) => {
  let title = bundle.GetStringFromName("remoteIncomingPromptTitle");
  let header = bundle.GetStringFromName("remoteIncomingPromptHeader");
  let clientEndpoint = `${client.host}:${client.port}`;
  let clientMsg =
    bundle.formatStringFromName("remoteIncomingPromptClientEndpoint",
                                [clientEndpoint], 1);
  let serverEndpoint = `${server.host}:${server.port}`;
  let serverMsg =
    bundle.formatStringFromName("remoteIncomingPromptServerEndpoint",
                                [serverEndpoint], 1);
  let footer = bundle.GetStringFromName("remoteIncomingPromptFooter");
  let msg =`${header}\n\n${clientMsg}\n${serverMsg}\n\n${footer}`;
  let disableButton = bundle.GetStringFromName("remoteIncomingPromptDisable");
  let prompt = Services.prompt;
  let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_OK +
              prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_CANCEL +
              prompt.BUTTON_POS_2 * prompt.BUTTON_TITLE_IS_STRING +
              prompt.BUTTON_POS_1_DEFAULT;
  let result = prompt.confirmEx(null, title, msg, flags, null, null,
                                disableButton, null, { value: false });
  if (result === 0) {
    return AuthenticationResult.ALLOW;
  }
  if (result === 2) {
    return AuthenticationResult.DISABLE_ALL;
  }
  return AuthenticationResult.DENY;
};
















Server.defaultReceiveOOB = () => {
  let title = bundle.GetStringFromName("serverReceiveOOBTitle");
  let msg = bundle.GetStringFromName("serverReceiveOOBBody");
  let input = { value: null };
  let prompt = Services.prompt;
  let result = prompt.prompt(null, title, msg, input, null, { value: false });
  if (!result) {
    return null;
  }
  
  input = input.value.trim();
  let sha256 = input.substring(0, 64);
  sha256 = sha256.replace(/\w{2}/g, "$&:").slice(0, -1).toUpperCase();
  let k = input.substring(64);
  return { sha256, k };
};
