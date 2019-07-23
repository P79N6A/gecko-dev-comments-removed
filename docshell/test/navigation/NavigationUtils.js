











































var body = "This frame was navigated.";
var target_url = "data:text/html,<html><body>" + body + "</body></html>";





function navigateByLocation(wnd) {
  try {
    wnd.location = target_url;
  } catch(ex) {
    
    
    window.open(target_url, "_blank", "width=10,height=10");
  }
}

function navigateByOpen(name) {
  window.open(target_url, name, "width=10,height=10");
}

function navigateByForm(name) {
  var form = document.createElement("form");
  form.action = target_url;
  form.method = "POST";
  form.target = name; document.body.appendChild(form);
  form.submit();
}

var hyperlink_count = 0;

function navigateByHyperlink(name) {
  var link = document.createElement("a");
  link.href = target_url;
  link.target = name;
  link.id = "navigation_hyperlink_" + hyperlink_count++;
  document.body.appendChild(link);
  sendMouseEvent({type:"click"}, link.id);
}





function isNavigated(wnd, message) {
  var result = null;
  try {
    result = wnd.document.body.innerHTML;
  } catch(ex) {
    result = ex;
  }
  is(result, body, message);
}

function isBlank(wnd, message) {
  var result = null;
  try {
    result = wnd.document.body.innerHTML;
  } catch(ex) {
    result = ex;
  }
  is(result, "This is a blank document.", message);
}

function isAccessible(wnd, message) {
  try {
    wnd.document.body.innerHTML;
    ok(true, message);
  } catch(ex) {
    ok(false, message);
  }
}

function isInaccessible(wnd, message) {
  try {
    wnd.document.body.innerHTML;
    ok(false, message);
  } catch(ex) {
    ok(true, message);
  }
}






function xpcGetFramesByName(name) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  var enumerator = ww.getWindowEnumerator();

  var results = [];

  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    var tabs = win.gBrowser.browsers;

    for (var i = 0; i < tabs.length; i++) {
      var domwindow = tabs[i].docShell.document.defaultView;

      if (domwindow.name == name)
        results.push(domwindow);
    }
  }

  return results;
}

function xpcCleanupWindows() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  var enumerator = ww.getWindowEnumerator();

  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    var tabs = win.gBrowser.browsers;

    for (var i = 0; i < tabs.length; i++) {
      var domwindow = tabs[i].docShell.document.defaultView;

      if (domwindow.location.protocol == "data:")
        domwindow.close();
    }
  }
}

function xpcWaitForFinishedFrames(callback, numFrames) {
  var finishedFrameCount = 0;
  function frameFinished() {
    finishedFrameCount++;

    if (finishedFrameCount == numFrames) {
      clearInterval(frameWaitInterval);
      setTimeout(callback, 1);
      return;
    }

    if (finishedFrameCount > numFrames)
      throw "Too many frames loaded.";
  }

  var finishedWindows = [];

  function contains(obj, arr) {
    for (var i = 0; i < arr.length; i++) {
      if (obj === arr[i])
        return true;
    }
    return false;
  }

  function searchForFinishedFrames(win) {
    if (escape(unescape(win.location)) == escape(target_url)) {
      if (!contains(win, finishedWindows)) {
        finishedWindows.push(win);
        frameFinished();
      }
    }
    for (var i = 0; i < win.frames.length; i++)
      searchForFinishedFrames(win.frames[i]);
  }

  function poll() {
    
    
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                       .getService(Components.interfaces.nsIWindowWatcher);
    var enumerator = ww.getWindowEnumerator();
    while (enumerator.hasMoreElements()) {
      var win = enumerator.getNext();
      if (!win.gBrowser)
        continue;  
      var tabs = win.gBrowser.browsers;

      for (var i = 0; i < tabs.length; i++) {
        searchForFinishedFrames(tabs[i].docShell.document.defaultView);
      }
    }
  }

  var frameWaitInterval = setInterval(poll, 500);
}

