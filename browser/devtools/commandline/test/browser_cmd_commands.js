




let HUDService = (Cu.import("resource:///modules/HUDService.jsm", {})).HUDService;

const TEST_URI = "data:text/html;charset=utf-8,gcli-commands";

let tests = {};

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, tests);
  }).then(finish);
}

tests.testEcho = function(options) {
  return helpers.audit(options, [
    {
      setup: "echo message",
      exec: {
        output: "message",
      }
    }
  ]);
};

tests.testConsole = function(options) {
  let deferred = Promise.defer();
  let hud = null;

  let onWebConsoleOpen = function(subject) {
    Services.obs.removeObserver(onWebConsoleOpen, "web-console-created");

    subject.QueryInterface(Ci.nsISupportsString);
    hud = HUDService.getHudReferenceById(subject.data);
    ok(hud.hudId in HUDService.hudReferences, "console open");

    hud.jsterm.execute("pprint(window)", onExecute);
  }
  Services.obs.addObserver(onWebConsoleOpen, "web-console-created", false);

  let onExecute = function() {
    let labels = hud.outputNode.querySelectorAll(".webconsole-msg-output");
    ok(labels.length > 0, "output for pprint(window)");

    helpers.audit(options, [
      {
        setup: "console clear",
        exec: {
          output: ""
        },
        post: function() {
          let labels = hud.outputNode.querySelectorAll(".webconsole-msg-output");
          
          
        }
      },
      {
        setup: "console close",
        exec: {
          output: ""
        },
        post: function() {
          ok(!(hud.hudId in HUDService.hudReferences), "console closed");
        }
      }
    ]).then(function() {
      
      
      
      executeSoon(function() {
        deferred.resolve();
      });
    });
  };

  helpers.audit(options, [
    {
      setup: "console open",
      exec: { }
    }
  ]);

  return deferred.promise;
};
