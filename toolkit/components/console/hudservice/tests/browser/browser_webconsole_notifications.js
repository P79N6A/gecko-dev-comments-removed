




































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  observer.init();
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function webConsoleCreated(aID)
{
  Services.obs.removeObserver(observer, "web-console-created");
  executeSoon(function (){
    ok(HUDService.hudReferences[aID], "We have a hud reference");
    let console = browser.contentWindow.wrappedJSObject.console;
    console.log("adding a log message");
  });
}

function webConsoleDestroyed(aID)
{
  Services.obs.removeObserver(observer, "web-console-destroyed");
  ok(!HUDService.hudReferences[aID], "We do not have a hud reference");
  finishTest();
}

function webConsoleMessage(aID, aNodeID)
{
  Services.obs.removeObserver(observer, "web-console-message-created");
  ok(aID, "we have a console ID");
  ok(typeof aNodeID == 'string', "message node id is not null");
  closeConsole();
}

let observer = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function observe(aSubject, aTopic, aData)
  {
    aSubject = aSubject.QueryInterface(Ci.nsISupportsString);

    switch(aTopic) {
      case "web-console-created":
        webConsoleCreated(aSubject);
        break;
      case "web-console-destroyed":
        webConsoleDestroyed(aSubject);
        break;
      case "web-console-message-created":
        webConsoleMessage(aSubject, aData);
        break;
      default:
        break;
    }
  },

  init: function init()
  {
    Services.obs.addObserver(this, "web-console-created", false);
    Services.obs.addObserver(this, "web-console-destroyed", false);
    Services.obs.addObserver(this, "web-console-message-created", false);
  }
};

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole();
}
