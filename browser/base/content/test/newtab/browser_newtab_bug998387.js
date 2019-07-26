


function runTests() {
  yield setLinks("0");
  yield addNewTabPageTab();

  
  let cell = getCell(0);
  let clicked = false;
  cell.site.onClick = e => {
    clicked = true;
    executeSoon(TestRunner.next);
  };

  
  let block = getContentDocument().querySelector(".newtab-control-block");
  yield EventUtils.synthesizeMouseAtCenter(block, {button: 1}, getContentWindow());
  ok(clicked, "middle click triggered click listener");

  
  checkGrid("0");
}
