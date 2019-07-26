function test() {
  waitForExplicitFinish();

  
  
  testLink(0, true, false, function() {
    
    
    testLink(1, true, true, function() {
      
      
      testLink(2, true, true, function() {
        
        
        testLink(2, false, false, function() {
          
          
          testLink(3, true, true, function() {
            
            
            testLink(0, true, false, function() {
              
              
              testLink(4, true, false, function() {
                
                
                testLink(5, true, false, function() {
                  
                  
                  testLink(6, true, false, finish);
                });
              });
            }, true);
          });
        });
      });
    });
  });
}

function testLink(aLinkIndex, pinTab, expectNewTab, nextTest, testSubFrame) {
  let appTab = gBrowser.addTab("http://example.com/browser/browser/base/content/test/general/app_bug575561.html", {skipAnimation: true});
  if (pinTab)
    gBrowser.pinTab(appTab);
  gBrowser.selectedTab = appTab;
  appTab.linkedBrowser.addEventListener("load", onLoad, true);

  let loadCount = 0;
  function onLoad() {
    loadCount++;
    if (loadCount < 2)
      return;

    appTab.linkedBrowser.removeEventListener("load", onLoad, true);

    let browser = gBrowser.getBrowserForTab(appTab);
    if (testSubFrame)
      browser = browser.contentDocument.getElementsByTagName("iframe")[0];

    let links = browser.contentDocument.getElementsByTagName("a");

    if (expectNewTab)
      gBrowser.tabContainer.addEventListener("TabOpen", onTabOpen, true);
    else
      browser.addEventListener("load", onPageLoad, true);

    info("Clicking " + links[aLinkIndex].textContent);
    EventUtils.sendMouseEvent({type:"click"}, links[aLinkIndex], browser.contentWindow);
    let linkLocation = links[aLinkIndex].href;

    function onPageLoad() {
      browser.removeEventListener("load", onPageLoad, true);
      is(browser.contentDocument.location.href, linkLocation, "Link should not open in a new tab");
      executeSoon(function(){
        gBrowser.removeTab(appTab);
        nextTest();
      });
    }

    function onTabOpen(event) {
      gBrowser.tabContainer.removeEventListener("TabOpen", onTabOpen, true);
      ok(true, "Link should open a new tab");
      executeSoon(function(){
        gBrowser.removeTab(appTab);
        gBrowser.removeCurrentTab();
        nextTest();
      });
    }
  }
}
