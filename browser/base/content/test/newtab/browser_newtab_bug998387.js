


function runTests() {
  yield setLinks("0");
  yield addNewTabPageTab();

  
  let {site} = getCell(0);
  let origOnClick = site.onClick;
  let clicked = false;
  site.onClick = e => {
    origOnClick.call(site, e);
    clicked = true;
    executeSoon(TestRunner.next);
  };

  
  let block = getContentDocument().querySelector(".newtab-control-block");
  yield EventUtils.synthesizeMouseAtCenter(block, {button: 1}, getContentWindow());
  ok(clicked, "middle click triggered click listener");

  
  checkGrid("0");
}
