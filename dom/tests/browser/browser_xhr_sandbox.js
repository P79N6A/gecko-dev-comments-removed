
let sandboxCode = (function() {
  let req = new XMLHttpRequest();
  req.open("GET", "http://mochi.test:8888/browser/dom/tests/browser/", true);
  req.onreadystatechange = function() {
    if (req.readyState === 4) {
      
      
      
      let result;
      if (req.status != 200) {
        result = "ERROR: got request status of " + req.status;
      } else if (req.responseText.length == 0) {
        result = "ERROR: got zero byte response text";
      } else {
        result = "ok";
      }
      postMessage({result: result}, "*");
    }
  };
  req.send(null);
}).toSource() + "();";

function test() {
  waitForExplicitFinish();
  let appShell = Cc["@mozilla.org/appshell/appShellService;1"]
                  .getService(Ci.nsIAppShellService);
  let doc = appShell.hiddenDOMWindow.document;
  let frame = doc.createElement("iframe");
  frame.setAttribute("type", "content");
  frame.setAttribute("src", "http://mochi.test:8888/browser/dom/tests/browser/browser_xhr_sandbox.js");

  frame.addEventListener("load", function () {
    let workerWindow = frame.contentWindow;
    workerWindow.addEventListener("message", function(evt) {
      is(evt.data.result, "ok", "check the sandbox code was happy");
      finish();
    }, true);
    let sandbox = new Cu.Sandbox(workerWindow);
    
    
    sandbox.importFunction(workerWindow.postMessage.bind(workerWindow), "postMessage");
    sandbox.importFunction(workerWindow.XMLHttpRequest, "XMLHttpRequest");
    Cu.evalInSandbox(sandboxCode, sandbox, "1.8");
  }, true);

  let container = doc.body ? doc.body : doc.documentElement;
  container.appendChild(frame);
}
