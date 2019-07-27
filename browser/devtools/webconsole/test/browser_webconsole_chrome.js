






"use strict";

function test() {
  Task.spawn(function*() {
    const {tab} = yield loadTab("about:config");
    ok(tab, "tab loaded");

    const hud = yield openConsole(tab);
    ok(hud, "we have a console");
    ok(hud.iframeWindow, "we have the console UI window");

    let jsterm = hud.jsterm;
    ok(jsterm, "we have a jsterm");

    let input = jsterm.inputNode;
    ok(hud.outputNode, "we have an output node");

    
    input.value = "docu";
    input.setSelectionRange(4, 4);

    let deferred = promise.defer();

    jsterm.complete(jsterm.COMPLETE_HINT_ONLY, function() {
      is(jsterm.completeNode.value, "    ment", "'docu' completion");
      deferred.resolve(null);
    });

    yield deferred.promise;
  }).then(finishTest);
}
