


const PRELOAD_PREF = "browser.newtab.preload";

let suggestedLink = {
  url: "http://example1.com/2",
  imageURI: "data:image/png;base64,helloWORLD3",
  title: "title2",
  type: "affiliate",
  frecent_sites: ["classroom.google.com", "codeacademy.org", "codecademy.com", "codeschool.com", "codeyear.com", "elearning.ut.ac.id", "how-to-build-websites.com", "htmlcodetutorial.com", "htmldog.com", "htmlplayground.com", "learn.jquery.com", "quackit.com", "roseindia.net", "teamtreehouse.com", "tizag.com", "tutorialspoint.com", "udacity.com", "w3schools.com", "webdevelopersnotes.com"]
};

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
  "suggested": [suggestedLink]
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
      suggested: siteNode.querySelector(".newtab-suggested").innerHTML
    };
  }

  
  yield setLinks("-1");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("classic");
  yield customizeNewTabPage("enhanced"); 
  let {type, enhanced, title, suggested} = getData(0);
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "example.com");
  is(suggested, "", "There is no suggested explanation");

  is(getData(1), null, "there is only one link and it's a history link");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced"); 
  ({type, enhanced, title, suggested} = getData(0));
  is(type, "organic", "directory link is organic");
  isnot(enhanced, "", "directory link has enhanced image");
  is(title, "title1");
  is(suggested, "", "There is no suggested explanation");

  ({type, enhanced, title, suggested} = getData(1));
  is(type, "enhanced", "history link is enhanced");
  isnot(enhanced, "", "history link has enhanced image");
  is(title, "title");
  is(suggested, "", "There is no suggested explanation");

  is(getData(2), null, "there are only 2 links, directory and enhanced history");

  
  setPinnedLinks("-1");
  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  is(type, "enhanced", "pinned history link is enhanced");
  isnot(enhanced, "", "pinned history link has enhanced image");
  is(title, "title");
  is(suggested, "", "There is no suggested explanation");

  ({type, enhanced, title, suggested} = getData(1));
  is(type, "organic", "directory link is organic");
  isnot(enhanced, "", "directory link has enhanced image");
  is(title, "title1");
  is(suggested, "", "There is no suggested explanation");

  is(getData(2), null, "directory link pushed out by pinned history link");

  
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced"); 
  ({type, enhanced, title, suggested} = getData(0));
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "example.com");
  is(suggested, "", "There is no suggested explanation");

  is(getData(1), null, "directory link still pushed out by pinned history link");

  yield unpinCell(0);



  
  let origIsTopPlacesSite = NewTabUtils.isTopPlacesSite;
  NewTabUtils.isTopPlacesSite = () => true;
  yield setLinks("-1,2,3,4,5,6,7,8");

  
  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  isnot(type, "enhanced", "history link is not enhanced");
  is(enhanced, "", "history link has no enhanced image");
  is(title, "example.com");
  is(suggested, "", "There is no suggested explanation");

  isnot(getData(7), null, "there are 8 history links");
  is(getData(8), null, "there are 8 history links");


  
  yield addNewTabPageTab();
  yield customizeNewTabPage("enhanced");

  
  ({type, enhanced, title, suggested} = getData(0));
  is(type, "affiliate", "suggested link is affiliate");
  is(enhanced, "", "suggested link has no enhanced image");
  is(title, "title2");
  ok(suggested.indexOf("Suggested for <strong> webdev education </strong> visitors") > -1, "Suggested for 'webdev education'");

  
  ({type, enhanced, title, suggested} = getData(1));
  is(type, "enhanced", "pinned history link is enhanced");
  isnot(enhanced, "", "pinned history link has enhanced image");
  is(title, "title");
  is(suggested, "", "There is no suggested explanation");

  is(getData(9), null, "there is a suggested link followed by an enhanced history link and the remaining history links");



  
  suggestedLink.adgroup_name = "Technology";
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE,
    "data:application/json," + JSON.stringify({"suggested": [suggestedLink]}));
  yield watchLinksChangeOnce().then(TestRunner.next);

  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  Cu.reportError("SUGGEST " + suggested);
  ok(suggested.indexOf("Suggested for <strong> Technology </strong> visitors") > -1, "Suggested for 'Technology'");


  
  suggestedLink.explanation = "Suggested for %1$S enthusiasts who visit sites like %2$S";
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE,
    "data:application/json," + encodeURIComponent(JSON.stringify({"suggested": [suggestedLink]})));
  yield watchLinksChangeOnce().then(TestRunner.next);

  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  Cu.reportError("SUGGEST " + suggested);
  ok(suggested.indexOf("Suggested for <strong> Technology </strong> enthusiasts who visit sites like <strong> classroom.google.com </strong>") > -1, "Suggested for 'Technology' enthusiasts");


  
  delete suggestedLink.adgroup_name;
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE,
    "data:application/json," + encodeURIComponent(JSON.stringify({"suggested": [suggestedLink]})));
  yield watchLinksChangeOnce().then(TestRunner.next);

  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  Cu.reportError("SUGGEST " + suggested);
  ok(suggested.indexOf("Suggested for <strong> webdev education </strong> enthusiasts who visit sites like <strong> classroom.google.com </strong>") > -1, "Suggested for 'webdev education' enthusiasts");



  
  suggestedLink.url = "http://example1.com/3";
  suggestedLink.adgroup_name = ">angles< & \"quotes\'";
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE,
    "data:application/json," + encodeURIComponent(JSON.stringify({"suggested": [suggestedLink]})));
  yield watchLinksChangeOnce().then(TestRunner.next);

  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  Cu.reportError("SUGGEST " + suggested);
  ok(suggested.indexOf("Suggested for <strong> &gt;angles&lt; &amp; \"quotes\' </strong> enthusiasts who visit sites like <strong> classroom.google.com </strong>") > -1, "Suggested for 'xml entities' enthusiasts");


  
  suggestedLink.explanation = "Testing junk explanation &<>\"'";
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE,
    "data:application/json," + encodeURIComponent(JSON.stringify({"suggested": [suggestedLink]})));
  yield watchLinksChangeOnce().then(TestRunner.next);

  yield addNewTabPageTab();
  ({type, enhanced, title, suggested} = getData(0));
  Cu.reportError("SUGGEST " + suggested);
  ok(suggested.indexOf("Testing junk explanation &amp;&lt;&gt;\"'") > -1, "Junk test");
}
