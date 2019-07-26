






function runTests() {
  
  
  
  
  
  
  
  
  
  setLinks([]);
  whenPagesUpdated(null, true);
  yield null;
  yield null;

  
  fillHistory([link(1)]);
  yield whenPagesUpdated(null, true);
  yield addNewTabPageTab();
  checkGrid("1,,,,,,,,");

  fillHistory([link(2)]);
  yield whenPagesUpdated(null, true);
  yield addNewTabPageTab();
  checkGrid("2,1,,,,,,,");

  fillHistory([link(1)]);
  yield whenPagesUpdated(null, true);
  yield addNewTabPageTab();
  checkGrid("1,2,,,,,,,");

  
  yield fillHistory([link(2), link(3), link(4)], TestRunner.next);
  yield whenPagesUpdated(null, true);
  yield addNewTabPageTab();
  checkGrid("2,1,3,4,,,,,");
}

function link(id) {
  return { url: "http://example.com/#" + id, title: "site#" + id };
}
