





function runTests() {
  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("5");

  yield addNewTabPageTab();
  checkGrid("5p,0,1,2,3,4,6,7,8");

  yield blockCell(4);
  yield blockCell(4);
  checkGrid("5p,0,1,2,6,7,8");

  yield undo();
  checkGrid("5p,0,1,2,4,6,7,8");

  
  yield blockCell(0);
  checkGrid("0,1,2,4,6,7,8");

  yield undo();
  checkGrid("5p,0,1,2,4,6,7,8");

  
  yield blockCell(1);
  checkGrid("5p,1,2,4,6,7,8");

  yield undoAll();
  checkGrid("5p,0,1,2,3,4,6,7,8");
}

function undo() {
  let cw = getContentWindow();
  let target = cw.document.getElementById("newtab-undo-button");
  EventUtils.synthesizeMouseAtCenter(target, {}, cw);
  whenPagesUpdated();
}

function undoAll() {
  let cw = getContentWindow();
  let target = cw.document.getElementById("newtab-undo-restore-button");
  EventUtils.synthesizeMouseAtCenter(target, {}, cw);
  whenPagesUpdated();
}