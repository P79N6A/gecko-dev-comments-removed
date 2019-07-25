




































function test() {
  
  
  
  
  function tabOpenDance() {
    let tabs = [];
    function addTab(aURL,aReferrer)
      tabs.push(gBrowser.addTab(aURL, {referrerURI: aReferrer}));

    addTab("http:
    gBrowser.selectedTab = tabs[0];
    addTab("http://mochi.test:8888/#1");
    addTab("http://mochi.test:8888/#2",gBrowser.currentURI);
    addTab("http://mochi.test:8888/#3",gBrowser.currentURI);
    gBrowser.selectedTab = tabs[tabs.length - 1];
    gBrowser.selectedTab = tabs[0];
    addTab("http://mochi.test:8888/#4",gBrowser.currentURI);
    gBrowser.selectedTab = tabs[3];
    addTab("http://mochi.test:8888/#5",gBrowser.currentURI);
    gBrowser.removeTab(tabs.pop());
    addTab("about:blank",gBrowser.currentURI);
    gBrowser.moveTabTo(gBrowser.selectedTab, 1);
    addTab("http://mochi.test:8888/#6",gBrowser.currentURI);
    addTab();
    addTab("http://mochi.test:8888/#7");

    return tabs;
  }

  function cleanUp(aTabs)
    aTabs.forEach(gBrowser.removeTab, gBrowser);

  let tabs = tabOpenDance();

  is(tabs[0], gBrowser.tabs[3], "tab without referrer was opened to the far right");
  is(tabs[1], gBrowser.tabs[7], "tab without referrer was opened to the far right");
  is(tabs[2], gBrowser.tabs[5], "tab with referrer opened immediately to the right");
  is(tabs[3], gBrowser.tabs[1], "next tab with referrer opened further to the right");
  is(tabs[4], gBrowser.tabs[4], "tab selection changed, tab opens immediately to the right");
  is(tabs[5], gBrowser.tabs[6], "blank tab with referrer opens to the right of 3rd original tab where removed tab was");
  is(tabs[6], gBrowser.tabs[2], "tab has moved, new tab opens immediately to the right"); 
  is(tabs[7], gBrowser.tabs[8], "blank tab without referrer opens at the end"); 
  is(tabs[8], gBrowser.tabs[9], "tab without referrer opens at the end"); 

  cleanUp(tabs);
}
