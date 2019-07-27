






"use strict";





add_task(function* test_select_profile() {
  
  let { uiWindow, promiseResult } = yield FormAutofillTest.showUI(
                                          TestData.requestEmailOnly);

  
  let acceptButton = uiWindow.document.getElementById("accept");
  EventUtils.synthesizeMouseAtCenter(acceptButton, {}, uiWindow);

  let result = yield promiseResult;
  Assert.equal(result.fields.length, 1);
  Assert.equal(result.fields[0].section, "");
  Assert.equal(result.fields[0].addressType, "");
  Assert.equal(result.fields[0].contactType, "");
  Assert.equal(result.fields[0].fieldName, "email");
  Assert.equal(result.fields[0].value, "email@example.org");
});




add_task(function* test_cancel() {
  
  let { uiWindow, promiseResult } = yield FormAutofillTest.showUI(
                                          TestData.requestEmailOnly);

  
  let acceptButton = uiWindow.document.getElementById("cancel");
  EventUtils.synthesizeMouseAtCenter(acceptButton, {}, uiWindow);

  let result = yield promiseResult;
  Assert.ok(result.canceled);
});

add_task(terminationTaskFn);
