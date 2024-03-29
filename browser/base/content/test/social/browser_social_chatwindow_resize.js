



function test() {
  requestLongerTimeout(2); 
  waitForExplicitFinish();

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/general/moz.png",
    
    chatURL: "https://example.com/browser/browser/base/content/test/social/social_chat.html"
  };
  let oldwidth = window.outerWidth; 
  let oldleft = window.screenX;
  window.moveTo(0, window.screenY)
  let postSubTest = function(cb) {
    let chats = document.getElementById("pinnedchats");
    ok(chats.children.length == 0, "no chatty children left behind");
    cb();
  };

  runSocialTestWithProvider(manifest, function (finishcb) {
    SocialSidebar.show();
    let port = SocialSidebar.provider.getWorkerPort();
    ok(port, "provider has a port");
    port.postMessage({topic: "test-init"});
    
    waitForCondition(function() {
      let sbrowser = document.getElementById("social-sidebar-browser");
      return SocialSidebar.provider &&
             SocialSidebar.provider.profile &&
             SocialSidebar.provider.profile.displayName &&
             sbrowser.docShellIsActive;
    }, function() {
      
      runSocialTests(tests, undefined, postSubTest, function() {
        window.moveTo(oldleft, window.screenY)
        window.resizeTo(oldwidth, window.outerHeight);
        port.close();
        finishcb();
      });
    },
    "waitForProviderLoad: provider profile was not set", 100);
  });
}

var tests = {

  
  testBrowserResize: function(next, mode) {
    let chats = document.getElementById("pinnedchats");
    get3ChatsForCollapsing(mode || "normal", function(first, second, third) {
      let chatWidth = chats.getTotalChildWidth(first);
      ok(chatWidth, "have a chatwidth");
      let popupWidth = getPopupWidth();
      ok(popupWidth, "have a popupwidth");
      info("starting resize tests - each chat's width is " + chatWidth +
           " and the popup width is " + popupWidth);
      
      
      resizeAndCheckWidths(first, second, third, [
        [chatWidth-2, 1, "to < 1 chat width - only last should be visible."],
        [chatWidth+2, 1, "2 pixels more then one fully exposed (not counting popup) - still only 1."],
        [chatWidth+popupWidth+2, 1, "2 pixels more than one fully exposed (including popup) - still only 1."],
        [chatWidth*2-2, 1, "second not showing by 2 pixels (not counting popup) - only 1 exposed."],
        [chatWidth*2+popupWidth-2, 1, "second not showing by 2 pixelx (including popup) - only 1 exposed."],
        [chatWidth*2+popupWidth+2, 2, "big enough to fit 2 - nub remains visible as first is still hidden"],
        [chatWidth*3+popupWidth-2, 2, "one smaller than the size necessary to display all three - first still hidden"],
        [chatWidth*3+popupWidth+2, 3, "big enough to fit all - all exposed (which removes the nub)"],
        [chatWidth*3+2, 3, "now the nub is hidden we can resize back down to chatWidth*3 before overflow."],
        [chatWidth*3-2, 2, "2 pixels less and the first is again collapsed (and the nub re-appears)"],
        [chatWidth*2+popupWidth+2, 2, "back down to just big enough to fit 2"],
        [chatWidth*2+popupWidth-2, 1, "back down to just not enough to fit 2"],
        [chatWidth*3+popupWidth+2, 3, "now a large jump to make all 3 visible (ie, affects 2)"],
        [chatWidth*1.5, 1, "and a large jump back down to 1 visible (ie, affects 2)"],
      ], function() {
        closeAllChats();
        next();
      });
    });
  },

  testBrowserResizeMinimized: function(next) {
    this.testBrowserResize(next);
  }
}
