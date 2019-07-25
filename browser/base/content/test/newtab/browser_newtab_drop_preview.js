






function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("0,1,2,,,5");

  yield addNewTabPageTab();
  checkGrid("0p,1p,2p,3,4,5p,6,7,8");

  let cw = getContentWindow();
  cw.gDrag._draggedSite = getCell(0).site;
  let sites = cw.gDropPreview.rearrange(getCell(4));
  cw.gDrag._draggedSite = null;

  checkGrid("3,1p,2p,4,0p,5p,6,7,8", sites);
}
