






add_task(function*() {
  let client = yield startTestDebuggerServer("promises-actor-test");

  let response = yield listTabs(client);
  let targetTab = findTab(response.tabs, "promises-actor-test");
  ok(targetTab, "Found our target tab.")

  
  client.request({ to: targetTab.actor, type: "attach" }, response => {
    ok(!("error" in response), "Expect no error in response.");
    ok(response.from, targetTab.actor,
      "Expect the target TabActor in response form field.");
    ok(response.type, "tabAttached",
      "Expect tabAttached in the response type.");
    is(typeof response.promisesActor === "string",
      "Should have a tab context PromisesActor.");
  });

  let chromeActors = yield getChromeActors(client);
  ok(typeof chromeActors.promisesActor === "string",
    "Should have a chrome context PromisesActor.");
});
