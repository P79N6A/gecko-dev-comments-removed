


function runTests() {
  
  return;

  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();

  let receivedError = false;
  let block = cw.document.querySelector(".newtab-control-block");

  function onError() {
    receivedError = true;
  }

  cw.addEventListener("error", onError);

  for (let i = 0; i < 3; i++) {
    EventUtils.synthesizeMouseAtCenter(block, {}, cw);
    yield executeSoon(TestRunner.next);
  }

  yield whenPagesUpdated();
  ok(!receivedError, "we got here without any errors");
  cw.removeEventListener("error", onError);
}
