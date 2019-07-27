




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

    
    
    yield jsterm.execute("let buf = new ArrayBuffer(80);");

    
    yield testNotSorted("Array(0,1,2,3,4,5,6,7,8,9,10)");
    
    yield testNotSorted("document.querySelectorAll('div')");
    
    yield testSorted("Object({'hello':1,1:5,10:2,4:2,'abc':1})");

    
    for (let type of typedArrayTypes) {
      yield testNotSorted("new " + type + "(buf)");
    }
  }

  







  function* testNotSorted(aObject) {
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
  







  function* testSorted(aObject) {
    info("Testing " + aObject);
    let deferred = promise.defer();
    jsterm.once("variablesview-fetched", (_, aVar) => deferred.resolve(aVar));
    jsterm.execute("inspect(" + aObject + ")");

    let variableScope = yield deferred.promise;
    ok(variableScope, "Variables view opened");

    
    
    
    
    let keyIterator = variableScope._store.keys();
    is(keyIterator.next().value, "1", "First key should be 1");
    is(keyIterator.next().value, "4", "Second key should be 4");

    
    is(keyIterator.next().value, "10", "Third key is 10");
    
    is(keyIterator.next().value, "abc", "Fourth key is abc");
    is(keyIterator.next().value, "hello", "Fifth key is hello");
  }

  Task.spawn(runner).then(finishTest);
}
