








gDirectorySource = "data:application/json," + JSON.stringify({
  "suggested": [{
    url: "http://suggested.com/",
    imageURI: "data:image/png;base64,helloWORLD3",
    title: "title",
    type: "affiliate",
    frecent_sites: ["example0.com"]
  }]
});

function runTests() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = (site) => false;

  
  
  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks("");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8");

  yield blockCell(4);
  checkGrid("0,1,2,3,5,6,7,8,9");

  yield blockCell(4);
  checkGrid("0,1,2,3,6,7,8,9,");

  yield blockCell(4);
  checkGrid("0,1,2,3,7,8,9,,");

  
  yield restore();
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks(",1");

  yield addNewTabPageTab();
  checkGrid("0,1p,2,3,4,5,6,7,8");

  yield blockCell(1);
  checkGrid("0,2,3,4,5,6,7,8,");

  
  
  yield restore();
  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield blockCell(8);
  checkGrid("0,1,2,3,4,5,6,7,9");

  
  
  yield restore();
  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks(",,,,,,,,8");

  yield addNewTabPageTab();
  checkGrid("0,1,2,3,4,5,6,7,8p");

  yield blockCell(0);
  checkGrid("1,2,3,4,5,6,7,9,8p");

  
  NewTabUtils.isTopPlacesSite = origIsTopPlacesSite;
  yield restore();
  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced");
  checkGrid("http://suggested.com/,0,1,2,3,4,5,6,7,8,9");

  yield blockCell(1);
  yield addNewTabPageTab();
  checkGrid("1,2,3,4,5,6,7,8,9");
  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
}
