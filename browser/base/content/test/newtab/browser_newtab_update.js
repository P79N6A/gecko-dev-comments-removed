






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

  
  is(getCell(1).site.link.type, "history", "added link is history");
}

function link(id) {
  return { url: "http://example" + id + ".com/", title: "site#" + id };
}
