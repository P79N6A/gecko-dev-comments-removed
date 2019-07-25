






var doc = "data:text/html,<html><body onload='load()'>" +
 "<script>" +
 "  var iframe = document.createElement('iframe');" +
 "  iframe.id = 'iframe';" +
 "  document.documentElement.appendChild(iframe);" +
 "  function load() {" +
 "    iframe.src = 'data:text/html,Hello!';" +
 "  }" +
 "</script>" +
 "</body></html>"

function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab(doc);
  let tabBrowser = tab.linkedBrowser;

  tabBrowser.addEventListener('load', function(aEvent) {
    tabBrowser.removeEventListener('load', arguments.callee, true);

    
    let iframe = tabBrowser.contentWindow.document.getElementById('iframe');
    iframe.addEventListener('load', function(aEvent) {

      
      if (!iframe.src)
        return;

      iframe.removeEventListener('load', arguments.callee, true);
      let shistory = tabBrowser.contentWindow
                      .QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIWebNavigation)
                      .sessionHistory;

      is(shistory.count, 1, 'shistory count should be 1.');

      gBrowser.removeTab(tab);
      finish();

    }, true);
  }, true);
}
