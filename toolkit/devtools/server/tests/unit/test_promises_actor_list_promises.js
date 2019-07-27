







"use strict";

const { PromisesFront } = devtools.require("devtools/server/actors/promises");
const SECRET = "MyLittleSecret";

add_task(function*() {
  let client = yield startTestDebuggerServer("promises-actor-test");
  let chromeActors = yield getChromeActors(client);

  yield testListPromises(client, chromeActors, v =>
    new Promise(resolve => resolve(v)));

  let response = yield listTabs(client);
  let targetTab = findTab(response.tabs, "promises-actor-test");
  ok(targetTab, "Found our target tab.");

  yield testListPromises(client, targetTab, v => {
    const debuggee = DebuggerServer.getTestGlobal("promises-actor-test");
    return debuggee.Promise.resolve(v);
  });

  yield close(client);
});

function* testListPromises(client, form, makePromise) {
  let resolution = SECRET + Math.random();
  let promise = makePromise(resolution);
  let front = PromisesFront(client, form);

  yield front.attach();

  let promises = yield front.listPromises();

  let found = false;
  for (let p of promises) {
    equal(p.type, "object", "Expect type to be Object");
    equal(p.class, "Promise", "Expect class to be Promise");

    if (p.promiseState.state === "fulfilled" &&
        p.promiseState.value === resolution) {
      found = true;
    }
  }

  ok(found, "Found our promise");
  yield front.detach();
  
  void promise;
}
