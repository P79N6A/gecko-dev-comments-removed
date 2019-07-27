







gDirectorySource = "data:application/json," + JSON.stringify({
  "suggested": [{
    url: "http://example.com/hardlanding/page.html",
    imageURI: "data:image/png;base64,helloWORLD3",
    enhancedImageURI: "data:image/png;base64,helloWORLD2",
    title: "title",
    type: "affiliate",
    frecent_sites: ["example0.com"],
  }]
});

function runTests() {
  let origGetFrecentSitesName = DirectoryLinksProvider.getFrecentSitesName;
  DirectoryLinksProvider.getFrecentSitesName = () => "";

  function getData(cellNum) {
    let cell = getCell(cellNum);
    if (!cell.site)
      return null;
    let siteNode = cell.site.node;
    return {
      type: siteNode.getAttribute("type"),
      thumbnail: siteNode.querySelector(".newtab-thumbnail").style.backgroundImage,
      enhanced: siteNode.querySelector(".enhanced-content").style.backgroundImage,
      title: siteNode.querySelector(".newtab-title").textContent,
      suggested: siteNode.getAttribute("suggested"),
      url: siteNode.querySelector(".newtab-link").getAttribute("href"),
    };
  }

  yield setLinks("0,1,2,3,4,5,6,7,8,9");
  setPinnedLinks("");

  yield addNewTabPageTab();
  
  yield addNewTabPageTab();
  checkGrid("http://example.com/hardlanding/page.html,0,1,2,3,4,5,6,7,8,9");
  
  let tileData = getData(0);
  is(tileData.type, "affiliate", "unpinned type");
  is(tileData.thumbnail, "url(\"data:image/png;base64,helloWORLD3\")", "unpinned thumbnail");
  is(tileData.enhanced, "url(\"data:image/png;base64,helloWORLD2\")", "unpinned enhanced");
  is(tileData.suggested, "true", "has suggested set", "unpinned suggested exists");
  is(tileData.url, "http://example.com/hardlanding/page.html", "unpinned landing page");

  
  is(NewTabUtils.pinnedLinks.isPinned({url: "http://example.com/hardlanding/page.html"}), false, "suggested tile is not pinned");

  
  whenPagesUpdated();
  let siteNode = getCell(0).node.querySelector(".newtab-site");
  let pinButton = siteNode.querySelector(".newtab-control-pin");
  EventUtils.synthesizeMouseAtCenter(pinButton, {}, getContentWindow());
  
  yield null;

  
  is(NewTabUtils.pinnedLinks.isPinned({url: "http://example.com/hardlanding/page.html"}), true, "suggested tile is pinned");
  tileData = getData(0);
  is(tileData.type, "history", "pinned type");
  is(tileData.suggested, null, "no suggested attribute");
  is(tileData.url, "http://example.com/hardlanding/page.html", "original landing page");

  
  siteNode = getCell(0).node.querySelector(".newtab-site");
  EventUtils.synthesizeMouseAtCenter(siteNode, {}, getContentWindow());
  
  yield addNewTabPageTab();
  yield addNewTabPageTab();

  
  tileData = getData(0);
  is(tileData.type, "history", "pinned type");
  is(tileData.suggested, null, "no suggested attribute");

  DirectoryLinksProvider.getFrecentSitesName = origGetFrecentSitesName;
}
