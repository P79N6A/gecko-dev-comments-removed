






"use strict";





add_task(function* test_select_profile() {
  
  let data = { "": { "": { "email": null } } };
  let { uiWindow, promiseResult } = yield FormAutofillTest.showUI(data);

  
  let acceptButton = uiWindow.document.getElementById("accept");
  EventUtils.synthesizeMouseAtCenter(acceptButton, {}, uiWindow);

  let result = yield promiseResult;
  Assert.equal(result.email, "email@example.org");
});




add_task(function* test_cancel() {
  
  let data = { "": { "": { "email": null } } };
  let { uiWindow, promiseResult } = yield FormAutofillTest.showUI(data);

  
  let acceptButton = uiWindow.document.getElementById("cancel");
  EventUtils.synthesizeMouseAtCenter(acceptButton, {}, uiWindow);

  let result = yield promiseResult;
  Assert.ok(result.canceled);
});

add_task(terminationTaskFn);
