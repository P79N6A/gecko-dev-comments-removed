




const TEST_URI = "http://example.com/browser/browser/devtools/commandline/" +
                 "test/browser_cmd_jsb_script.jsi";

let scratchpadWin = null;
let scratchpad = null;
let tests = {};

function test() {
  helpers.addTabWithToolbar("about:blank", function(options) {
    return helpers.runTests(options, tests);
  }).then(finish);
}

tests.jsbTest = function(options) {
  let deferred = Promise.defer();

  let observer = {
    onReady: function() {
      scratchpad.removeObserver(observer);

      let result = scratchpad.getText();
      result = result.replace(/[\r\n]]*/g, "\n");
      let correct = "function somefunc() {\n" +
                "  if (true) // Some comment\n" +
                "  doSomething();\n" +
                "  for (let n = 0; n < 500; n++) {\n" +
                "    if (n % 2 == 1) {\n" +
                "      console.log(n);\n" +
                "      console.log(n + 1);\n" +
                "    }\n" +
                "  }\n" +
                "}";
      is(result, correct, "JS has been correctly prettified");

      if (scratchpadWin) {
        scratchpadWin.close();
        scratchpadWin = null;
      }
      deferred.resolve();
    },
  };

  let onLoad = function GDT_onLoad() {
    scratchpadWin.removeEventListener("load", onLoad, false);
    scratchpad = scratchpadWin.Scratchpad;

    scratchpad.addObserver(observer);
  };

  let onNotify = function(subject, topic, data) {
    if (topic == "domwindowopened") {
      Services.ww.unregisterNotification(onNotify);

      scratchpadWin = subject.QueryInterface(Ci.nsIDOMWindow);
      scratchpadWin.addEventListener("load", onLoad, false);
    }
  };

  Services.ww.registerNotification(onNotify);

  helpers.audit(options, [
    {
      setup: 'jsb',
      check: {
        input:  'jsb',
        hints:     ' <url> [options]',
        markup: 'VVV',
        status: 'ERROR'
      }
    },
    {
      setup: 'jsb ' + TEST_URI,
      
      exec: {
        completed: false
      }
    }
  ]);

  return deferred.promise;
};
