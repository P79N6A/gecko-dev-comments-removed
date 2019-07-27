


const PRELOAD_PREF = "browser.newtab.preload";

gDirectorySource = "data:application/json," + JSON.stringify({
  "enhanced": [{
    url: "http://example.com/",
    enhancedImageURI: "data:image/png;base64,helloWORLD",
    title: "title",
    type: "organic",
  }],
  "directory": [{
    url: "http://example1.com/",
    enhancedImageURI: "data:image/png;base64,helloWORLD2",
    title: "title1",
    type: "organic"
  }],
  "suggested": [{
    url: "http://example1.com/2",
    imageURI: "data:image/png;base64,helloWORLD3",
    title: "title2",
    type: "affiliate",
    frecent_sites: ["test.com"]
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

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("classic");
  let {type, enhanced, title} = getData(0);
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "site#-1");

  is(getData(1), null, "there is only one link and it's a history link");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced");
  ({type, enhanced, title} = getData(0));
  is(type, "organic", "directory link is organic");
  isnot(enhanced, "", "directory link has enhanced image");
  is(title, "title1");

  ({type, enhanced, title} = getData(1));
  is(type, "enhanced", "history link is enhanced");
  isnot(enhanced, "", "history link has enhanced image");
  is(title, "title");

  is(getData(2), null, "there are only 2 links, directory and enhanced history");

  
  setPinnedLinks("-1");
  yield addNewTabPageTab();
  ({type, enhanced, title} = getData(0));
  is(type, "enhanced", "pinned history link is enhanced");
  isnot(enhanced, "", "pinned history link has enhanced image");
  is(title, "title");

  ({type, enhanced, title} = getData(1));
  is(type, "organic", "directory link is organic");
  isnot(enhanced, "", "directory link has enhanced image");
  is(title, "title1");

  is(getData(2), null, "directory link pushed out by pinned history link");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("classic");
  ({type, enhanced, title} = getData(0));
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "site#-1");

  is(getData(1), null, "directory link still pushed out by pinned history link");

  ok(getContentDocument().getElementById("newtab-intro-what"),
     "'What is this page?' link exists");

  yield unpinCell(0);



  
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;
  yield setLinks("-1,2,3,4,5,6,7,8");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("classic");
  ({type, enhanced, title} = getData(0));
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "site#-1");

  isnot(getData(7), null, "there are 8 history links");
  is(getData(8), null, "there are 8 history links");


  
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced");

  
  ({type, enhanced, title} = getData(0));
  is(type, "affiliate", "suggested link is affiliate");
  is(enhanced, "", "suggested link has no enhanced image");
  is(title, "title2");

  
  ({type, enhanced, title} = getData(1));
  is(type, "enhanced", "pinned history link is enhanced");
  isnot(enhanced, "", "pinned history link has enhanced image");
  is(title, "title");

  is(getData(9), null, "there is a suggested link followed by an enhanced history link and the remaining history links");
}
