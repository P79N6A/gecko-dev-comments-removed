

let win;

function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function(newWin) {
    win = newWin;

    registerCleanupFunction(function() {
      win.close();
    });

    is(win.gBrowser.tabContainer.getAttribute("overflow"), "",
       "The tabstrip should not be overflowing");

    finish();
  }, function(newWin) {
    
    var longTitle = "this is a very long title for the new tab ";
    longTitle = longTitle + longTitle + longTitle + longTitle + longTitle;
    longTitle = longTitle + longTitle + longTitle + longTitle + longTitle;
    newWin.gBrowser.addTab("data:text/html,<head><title>" + longTitle);
  });
}
