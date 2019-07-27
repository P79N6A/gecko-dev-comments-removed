


const PRELOAD_PREF = "browser.newtab.preload";

gDirectorySource = "data:application/json," + JSON.stringify({
  "en-US": [{
    url: "http://example.com/",
    enhancedImageURI: "data:image/png;base64,helloWORLD",
    title: "title",
    type: "organic"
  }]
});

function runTests() {
  let origEnhanced = NewTabUtils.allPages.enhanced;
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref(PRELOAD_PREF);
    NewTabUtils.allPages.enhanced = origEnhanced;
  });

  Services.prefs.setBoolPref(PRELOAD_PREF, false);

  function getData(cellNum) {
    let cell = getCell(cellNum);
    if (!cell.site)
      return null;
    let siteNode = cell.site.node;
    return {
      type: siteNode.getAttribute("type"),
      enhanced: siteNode.querySelector(".enhanced-content").style.backgroundImage,
      title: siteNode.querySelector(".newtab-title").textContent,
    };
  }

  
  yield setLinks("-1");

  
  NewTabUtils.allPages.enhanced = false;
  yield addNewTabPageTab();
  let {type, enhanced, title} = getData(0);
  is(type, "organic", "directory link is organic");
  isnot(enhanced, "", "directory link has enhanced image");
  is(title, "title");

  is(getData(1), null, "history link pushed out by directory link");

  
  NewTabUtils.allPages.enhanced = true;
  yield addNewTabPageTab();
  let {type, enhanced, title} = getData(0);
  is(type, "organic", "directory link is still organic");
  isnot(enhanced, "", "directory link still has enhanced image");
  is(title, "title");

  is(getData(1), null, "history link still pushed out by directory link");

  
  setPinnedLinks("-1");
  yield addNewTabPageTab();
  let {type, enhanced, title} = getData(0);
  is(type, "enhanced", "pinned history link is enhanced");
  isnot(enhanced, "", "pinned history link has enhanced image");
  is(title, "title");

  is(getData(1), null, "directory link pushed out by pinned history link");

  
  NewTabUtils.allPages.enhanced = false;
  yield addNewTabPageTab();
  let {type, enhanced, title} = getData(0);
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "site#-1");

  is(getData(1), null, "directory link still pushed out by pinned history link");
}
