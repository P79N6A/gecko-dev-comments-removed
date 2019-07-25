





const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const kDialog = "dialog";




































"use strict";

var gButtons = { };

function init() {
  var de = document.documentElement;
  var items = [];
  if (window.arguments[0] instanceof Components.interfaces.nsIDialogParamBlock) {
    
    var args = window.arguments[0];
    var softblocked = args.GetInt(0) == 1 ? true : false;

    var extensionsBundle = document.getElementById("extensionsBundle");
    try {
      var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                                .getService(Components.interfaces.nsIURLFormatter);
      var url = formatter.formatURLPref("extensions.blocklist.detailsURL");
    }
    catch (e) { }

    var params = {
      moreInfoURL: url,
    };

    if (softblocked) {
      params.title = extensionsBundle.getString("softBlockedInstallTitle");
      params.message1 = extensionsBundle.getFormattedString("softBlockedInstallMsg",
                                                           [args.GetString(0)]);
      var accept = de.getButton("accept");
      accept.label = extensionsBundle.getString("softBlockedInstallAcceptLabel");
      accept.accessKey = extensionsBundle.getString("softBlockedInstallAcceptKey");
      de.getButton("cancel").focus();
      document.addEventListener("dialogaccept", allowInstall, false);
    }
    else {
      params.title = extensionsBundle.getString("blocklistedInstallTitle2");
      params.message1 = extensionsBundle.getFormattedString("blocklistedInstallMsg2",
                                                           [args.GetString(0)]);
      de.buttons = "accept";
      de.getButton("accept").focus();
    }
  }
  else {
    items = window.arguments[0];
    params = window.arguments[1];
  }

  var addons = document.getElementById("addonsChildren");
  if (items.length > 0)
    document.getElementById("addonsTree").hidden = false;

  
  for (var item of items) {
    var treeitem = document.createElementNS(kXULNS, "treeitem");
    var treerow  = document.createElementNS(kXULNS, "treerow");
    var treecell = document.createElementNS(kXULNS, "treecell");
    treecell.setAttribute("label", item);
    treerow.appendChild(treecell);
    treeitem.appendChild(treerow);
    addons.appendChild(treeitem);
  }

  
  var messages = ["message1", "message2", "message3"];
  for (let messageEntry of messages) {
    if (messageEntry in params) {
      var message = document.getElementById(messageEntry);
      message.hidden = false;
      message.appendChild(document.createTextNode(params[messageEntry]));
    }
  }
  
  document.getElementById("infoIcon").className =
    params["iconClass"] ? "spaced " + params["iconClass"] : "spaced alert-icon";

  if ("moreInfoURL" in params && params["moreInfoURL"]) {
    message = document.getElementById("moreInfo");
    message.value = extensionsBundle.getString("moreInfoText");
    message.setAttribute("href", params["moreInfoURL"]);
    document.getElementById("moreInfoBox").hidden = false;
  }

  
  if ("title" in params)
    document.title = params.title;

  
  if ("buttons" in params) {
    gButtons = params.buttons;
    var buttonString = "";
    for (var buttonType in gButtons)
      buttonString += "," + buttonType;
    de.buttons = buttonString.substr(1);
    for (buttonType in gButtons) {
      var button = de.getButton(buttonType);
      button.label = gButtons[buttonType].label;
      if (gButtons[buttonType].focused)
        button.focus();
      document.addEventListener(kDialog + buttonType, handleButtonCommand, true);
    }
  }
}

function shutdown() {
  for (var buttonType in gButtons)
    document.removeEventListener(kDialog + buttonType, handleButtonCommand, true);
}

function allowInstall() {
  var args = window.arguments[0];
  args.SetInt(1, 1);
}






function handleButtonCommand(event) {
  window.arguments[1].result = event.type.substr(kDialog.length);
}
