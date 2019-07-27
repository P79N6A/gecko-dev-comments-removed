


"use strict";




function runTests() {
  
  
  
  
  yield whenPagesUpdatedAnd(resolve => setLinks([], resolve));

  
  yield fillHistoryAndWaitForPageUpdate([1]);
  yield addNewTabPageTab();
  checkGrid("1,,,,,,,,");

  yield fillHistoryAndWaitForPageUpdate([2]);
  yield addNewTabPageTab();
  checkGrid("2,1,,,,,,,");

  yield fillHistoryAndWaitForPageUpdate([1]);
  yield addNewTabPageTab();
  checkGrid("1,2,,,,,,,");

  yield fillHistoryAndWaitForPageUpdate([2, 3, 4]);
  yield addNewTabPageTab();
  checkGrid("2,1,3,4,,,,,");

  
  is(getCell(1).site.link.type, "history", "added link is history");
}

function fillHistoryAndWaitForPageUpdate(links) {
  return whenPagesUpdatedAnd(resolve => fillHistory(links.map(link), resolve));
}

function whenPagesUpdatedAnd(promiseConstructor) {
  let promise1 = new Promise(whenPagesUpdated);
  let promise2 = new Promise(promiseConstructor);
  return Promise.all([promise1, promise2]).then(TestRunner.next);
}

function link(id) {
  return { url: "http://example" + id + ".com/", title: "site#" + id };
}
