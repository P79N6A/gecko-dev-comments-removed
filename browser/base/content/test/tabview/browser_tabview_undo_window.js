




































function watchOne(node, event, callback) {
  node.addEventListener(event, function() {
    node.removeEventListener(event, arguments.callee, false);
    callback();
  }, false);
}


function showAndHide(window, callback) {
  watchOne(window, "tabviewshown", function() window.TabView.toggle());
  watchOne(window, "tabviewhidden", function() executeSoon(callback));
  window.TabView.toggle();
}

function test() {
  waitForExplicitFinish();

  
  let newWin = openDialog(location, "", "chrome,all,dialog=no", "about:");
  watchOne(newWin, "load", function() {
    
    showAndHide(newWin, function() {
      newWin.close();
      undoClose();
    });
  });
}

function undoClose() {
  
  let newWin = undoCloseWindow();
  watchOne(newWin, "load", function() {
    watchOne(newWin.gBrowser.tabContainer, "SSTabRestored", function() {
      is(newWin.gBrowser.visibleTabs.length, 1, "One tab restored");

      
      newWin.gBrowser.selectedTab = newWin.gBrowser.addTab();
      is(newWin.gBrowser.visibleTabs.length, 2, "Two tabs are visible");

      
      showAndHide(newWin, function() {
        is(newWin.gBrowser.visibleTabs.length, 2, "Two tabs are still visible");

        
        newWin.close();
        finish();
      });
    });
  });
}
