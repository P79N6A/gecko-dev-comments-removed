






"use strict";




add_task_in_parent_process(function* test_cancel_init() {
  FormAutofillTest.requestAutocompleteResponse = { canceled: true };
});




add_task(function* test_cancel() {
  let promise = TestUtils.waitForEvent($("form"), "autocompleteerror");
  $("form").requestAutocomplete();
  let errorEvent = yield promise;

  Assert.equal(errorEvent.reason, "cancel");
});
