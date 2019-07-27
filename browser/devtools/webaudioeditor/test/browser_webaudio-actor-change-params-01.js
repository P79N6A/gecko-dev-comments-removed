






function spawnTest () {
  let [target, debuggee, front] = yield initBackend(CHANGE_PARAM_URL);
  let [_, nodes] = yield Promise.all([
    front.setup({ reload: true }),
    getN(front, "create-node", 3)
  ]);

  let osc = nodes[1];
  let eventCount = 0;

  yield front.enableChangeParamEvents(osc, 20);

  front.on("change-param", onChangeParam);

  yield getN(front, "change-param", 3);
  yield front.disableChangeParamEvents();

  let currEventCount = eventCount;

  
  ok(eventCount >= 3, "Calling `enableChangeParamEvents` should allow front to emit `change-param`.");

  yield wait(100);

  ok((eventCount - currEventCount) <= 2, "Calling `disableChangeParamEvents` should turn off the listener.");

  front.off("change-param", onChangeParam);

  yield removeTab(target.tab);
  finish();

  function onChangeParam ({ newValue, oldValue, param, actorID }) {
    is(actorID, osc.actorID, "correct `actorID` in `change-param`.");
    is(param, "detune", "correct `param` property in `change-param`.");
    ok(newValue > oldValue,
      "correct `newValue` (" + newValue + ") and `oldValue` (" + oldValue + ") in `change-param`");
    eventCount++;
  }
}
