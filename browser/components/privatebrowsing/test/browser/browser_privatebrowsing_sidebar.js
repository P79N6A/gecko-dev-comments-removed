








function test() {
  waitForExplicitFinish();

  let { utils: Cu } = Components;

  let { Promise: { defer } } = Cu.import("resource://gre/modules/Promise.jsm", {});

  
  function openSidebar(win) {
    let { promise, resolve } = defer();
    let doc = win.document;

    let sidebarID = 'viewBookmarksSidebar';

    let sidebar = doc.getElementById('sidebar');

    let sidebarurl = doc.getElementById(sidebarID).getAttribute('sidebarurl');

    sidebar.addEventListener('load', function onSidebarLoad() {
      if (sidebar.contentWindow.location.href != sidebarurl)
        return;
      sidebar.removeEventListener('load', onSidebarLoad, true);

      resolve(win);
    }, true);

    win.SidebarUI.show(sidebarID);

    return promise;
  }

  let windowCache = [];
  function cacheWindow(w) {
    windowCache.push(w);
    return w;
  }
  function closeCachedWindows () {
    windowCache.forEach(function(w) w.close());
  }

  
  openWindow(window, {}, 1).
    then(cacheWindow).
    then(openSidebar).
    then(function(win) openWindow(win, { private: true })).
    then(cacheWindow).
    then(function({ document }) {
      let sidebarBox = document.getElementById("sidebar-box");
      is(sidebarBox.hidden, true, 'Opening a private window from reg window does not open the sidebar');
    }).
    
    then(function() openWindow(window)).
    then(cacheWindow).
    then(openSidebar).
    then(function(win) openWindow(win)).
    then(cacheWindow).
    then(function({ document }) {
      let sidebarBox = document.getElementById("sidebar-box");
      is(sidebarBox.hidden, false, 'Opening a reg window from reg window does open the sidebar');
    }).
    
    then(function() openWindow(window, { private: true })).
    then(cacheWindow).
    then(openSidebar).
    then(function(win) openWindow(win)).
    then(cacheWindow).
    then(function({ document }) {
      let sidebarBox = document.getElementById("sidebar-box");
      is(sidebarBox.hidden, true, 'Opening a reg window from a private window does not open the sidebar');
    }).
    
    then(function() openWindow(window, { private: true })).
    then(cacheWindow).
    then(openSidebar).
    then(function(win) openWindow(win, { private: true })).
    then(cacheWindow).
    then(function({ document }) {
      let sidebarBox = document.getElementById("sidebar-box");
      is(sidebarBox.hidden, false, 'Opening a private window from private window does open the sidebar');
    }).
    then(closeCachedWindows).
    then(finish);
}
