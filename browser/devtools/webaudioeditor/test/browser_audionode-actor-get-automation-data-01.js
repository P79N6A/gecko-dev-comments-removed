







add_task(function*() {
  let { target, front } = yield initBackend(SIMPLE_CONTEXT_URL);
  let [_, [destNode, oscNode, gainNode]] = yield Promise.all([
    front.setup({ reload: true }),
    get3(front, "create-node")
  ]);

  let t0 = 0, t1 = 0.1, t2 = 0.2, t3 = 0.3, t4 = 0.4, t5 = 0.6, t6 = 0.7, t7 = 1;
  let curve = [-1, 0, 1];
  yield oscNode.addAutomationEvent("frequency", "setValueAtTime", [0.2, t0]);
  yield oscNode.addAutomationEvent("frequency", "setValueAtTime", [0.3, t1]);
  yield oscNode.addAutomationEvent("frequency", "setValueAtTime", [0.4, t2]);
  yield oscNode.addAutomationEvent("frequency", "linearRampToValueAtTime", [1, t3]);
  yield oscNode.addAutomationEvent("frequency", "linearRampToValueAtTime", [0.15, t4]);
  yield oscNode.addAutomationEvent("frequency", "exponentialRampToValueAtTime", [0.75, t5]);
  yield oscNode.addAutomationEvent("frequency", "exponentialRampToValueAtTime", [0.05, t6]);
  
  
  yield oscNode.addAutomationEvent("frequency", "setValueCurveAtTime", [curve, t6, t7 - t6]);

  let { events, values } = yield oscNode.getAutomationData("frequency");

  is(events.length, 8, "8 recorded events returned.");
  is(values.length, 2000, "2000 value points returned.");

  checkAutomationValue(values, 0.05, 0.2);
  checkAutomationValue(values, 0.1, 0.3);
  checkAutomationValue(values, 0.15, 0.3);
  checkAutomationValue(values, 0.2, 0.4);
  checkAutomationValue(values, 0.25, 0.7);
  checkAutomationValue(values, 0.3, 1);
  checkAutomationValue(values, 0.35, 0.575);
  checkAutomationValue(values, 0.4, 0.15);
  checkAutomationValue(values, 0.45, 0.15 * Math.pow(0.75/0.15,0.05/0.2));
  checkAutomationValue(values, 0.5, 0.15 * Math.pow(0.75/0.15,0.5));
  checkAutomationValue(values, 0.55, 0.15 * Math.pow(0.75/0.15,0.15/0.2));
  checkAutomationValue(values, 0.6, 0.75);
  checkAutomationValue(values, 0.65, 0.75 * Math.pow(0.05/0.75, 0.5));
  checkAutomationValue(values, 0.705, -1); 
  checkAutomationValue(values, 0.8, 0);
  checkAutomationValue(values, 0.9, 1);
  checkAutomationValue(values, 1, 1);

  yield removeTab(target.tab);
});
