



"use strict";


















function test() {
  const TEST_URI = "data:text/html;charset=utf-8,   \
    <html>                                          \
      <head>                                        \
        <title>Test document for bug 977500</title> \
      </head>                                       \
      <body>                                        \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      <div></div>                                   \
      </body>                                       \
    </html>";

  let jsterm;

  function* runner() {
    const typedArrayTypes = ["Int8Array", "Int16Array", "Int32Array",
                             "Uint8Array", "Uint16Array", "Uint32Array",
                             "Uint8ClampedArray", "Float32Array",
                             "Float64Array"];

    const {tab} = yield loadTab(TEST_URI);
    const hud = yield openConsole(tab);
    jsterm = hud.jsterm;

    
    
    jsterm.execute("let buf = ArrayBuffer(80);");

    
    yield testNotSorted("Array(0,1,2,3,4,5,6,7,8,9,10)");
    
    yield testNotSorted("document.querySelectorAll('div')");

    
    for (let type of typedArrayTypes) {
      yield testNotSorted(type + "(buf)");
    }
  }

  







  function testNotSorted(aObject) {
    info("Testing " + aObject);
    let deferred = promise.defer();
    jsterm.once("variablesview-fetched", (_, aVar) => deferred.resolve(aVar));
    jsterm.execute("inspect(" + aObject + ")");

    let variableScope = yield deferred.promise;
    ok(variableScope, "Variables view opened");

    
    
    let keyIterator = variableScope._store.keys();
    is(keyIterator.next().value, "0", "First key is 0");
    is(keyIterator.next().value, "1", "Second key is 1");

    
    is(keyIterator.next().value, "2", "Third key is 2, not 10");
  }

  Task.spawn(runner).then(finishTest);
}
