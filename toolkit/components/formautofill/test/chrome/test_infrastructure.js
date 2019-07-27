






"use strict";




add_task(function* test_assert_truth() {
  Assert.ok(1 != 2);
});




add_task(function* test_assert_equality() {
  Assert.equal(1 + 1, 2);
});




add_task(function* test_utility_functions() {
  
  let randomString = "R" + Math.floor(Math.random() * 10);
  Output.print("The random contents will be '" + randomString + "'.");

  
  let path = yield TestUtils.getTempFile("test-infrastructure.txt");
  yield OS.File.writeAtomic(path, new TextEncoder().encode(randomString));

  
  yield TestUtils.waitForTick();
  yield TestUtils.waitMs(50);

  let promiseMyNotification = TestUtils.waitForNotification("my-topic");
  Services.obs.notifyObservers(null, "my-topic", "");
  yield promiseMyNotification;

  
  Assert.equal((yield OS.File.stat(path)).size, randomString.length);
});




add_task(function* test_content() {
  Assert.equal($("paragraph").innerHTML, "Paragraph contents.");

  let promiseMyEvent = TestUtils.waitForEvent($("paragraph"), "MyEvent");

  let event = document.createEvent("CustomEvent");
  event.initCustomEvent("MyEvent", true, false, {});
  $("paragraph").dispatchEvent(event);

  yield promiseMyEvent;
});
