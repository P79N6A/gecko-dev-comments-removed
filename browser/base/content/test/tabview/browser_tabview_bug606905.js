


function test() {
  waitForExplicitFinish();

  let newTabs = []
  
  do {
    let newTab = gBrowser.addTab("about:blank", {skipAnimation: true});
    newTabs.push(newTab);
  } while (gBrowser.visibleTabs[0].getBoundingClientRect().width > gBrowser.tabContainer.mTabClipWidth)

  
  executeSoon(function() {
    is(gBrowser.tabContainer.getAttribute("closebuttons"), "activetab", "Only show button on selected tab.");

    
    TabView._initFrame(function() {
      TabView.moveTabTo(newTabs[newTabs.length - 1], null);
      ok(gBrowser.visibleTabs[0].getBoundingClientRect().width > gBrowser.tabContainer.mTabClipWidth, 
         "Tab width is bigger than tab clip width");
      is(gBrowser.tabContainer.getAttribute("closebuttons"), "alltabs", "Show button on all tabs.")

      
      newTabs.forEach(function(tab) {
        gBrowser.removeTab(tab);
      });
      finish();
    });
  });
}
