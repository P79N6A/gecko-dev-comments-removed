


const Cc = Components.classes;
const Ci = Components.interfaces;

const URI_BLOCKLIST_DIALOG = "chrome://mozapps/content/extensions/blocklist.xul";

Components.utils.import("resource://gre/modules/Services.jsm");




let args = {
  restart: false,
  list: [{
    name: "Bug 523784 softblocked addon",
    version: "1",
    icon: "chrome://mozapps/skin/plugins/pluginGeneric.png",
    disable: false,
    blocked: false,
  }],
};

function test() {
  waitForExplicitFinish();

  let windowObserver = function(aSubject, aTopic, aData) {
    if (aTopic != "domwindowopened")
      return;

    Services.ww.unregisterNotification(windowObserver);

    let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
    win.addEventListener("load", function() {
      win.removeEventListener("load", arguments.callee, false);

      executeSoon(function() bug523784_test1(win));
    }, false);
  };
  Services.ww.registerNotification(windowObserver);

  args.wrappedJSObject = args;
  Services.ww.openWindow(null, URI_BLOCKLIST_DIALOG, "",
                         "chrome,centerscreen,dialog,titlebar", args);
}

function bug523784_test1(win) {
  let bundle = Services.strings.
              createBundle("chrome://mozapps/locale/update/updates.properties");
  let cancelButton = win.document.documentElement.getButton("cancel");

  is(cancelButton.getAttribute("label"),
     bundle.GetStringFromName("restartLaterButton"),
     "Text should be changed on Cancel button");
  is(cancelButton.getAttribute("accesskey"),
     bundle.GetStringFromName("restartLaterButton.accesskey"),
     "Accesskey should also be changed on Cancel button");

  let windowObserver = function(aSubject, aTopic, aData) {
    if (aTopic != "domwindowclosed")
      return;

    Services.ww.unregisterNotification(windowObserver);

    ok(args.list[0].disable, "Should be blocking add-on");
    ok(!args.restart, "Should not restart browser immediately");

    executeSoon(finish);
  };
  Services.ww.registerNotification(windowObserver);

  cancelButton.doCommand();
}
