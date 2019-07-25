











































var body = "This frame was navigated.";
var target_url = "data:text/html,<html><body>" + body + "</body></html>";

var popup_body = "This is a popup";
var target_popup_url = "data:text/html,<html><body>" + popup_body + "</body></html>";





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





function xpcEnumerateContentWindows(callback) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var Ci = Components.interfaces;
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Ci.nsIWindowWatcher);
  var enumerator = ww.getWindowEnumerator();

  var contentWindows = [];

  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    if (typeof ChromeWindow != "undefined" && win instanceof ChromeWindow) {
      var docshellTreeNode = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIDocShellTreeNode);
      var childCount = docshellTreeNode.childCount;
      for (var i = 0; i < childCount; ++i) {
        var childTreeNode = docshellTreeNode.getChildAt(i);

        
        if (childTreeNode.itemType != Ci.nsIDocShellTreeItem.typeContent)
          continue;

        var webNav = childTreeNode.QueryInterface(Ci.nsIWebNavigation);
        contentWindows.push(webNav.document.defaultView);
      }
    } else {
      contentWindows.push(win);
    }
  }

  while (contentWindows.length > 0)
    callback(contentWindows.pop());
}


function xpcGetFramesByName(name) {
  var results = [];

  xpcEnumerateContentWindows(function(win) {
    if (win.name == name)
      results.push(win);
  });

  return results;
}

function xpcCleanupWindows() {
  xpcEnumerateContentWindows(function(win) {
    if (win.location && win.location.protocol == "data:")
      win.close();
  });
}

function xpcWaitForFinishedFrames(callback, numFrames) {
  var finishedFrameCount = 0;
  function frameFinished() {
    finishedFrameCount++;

    if (finishedFrameCount == numFrames) {
      clearInterval(frameWaitInterval);
      setTimeout(callback, 0);
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
    if ((escape(unescape(win.location)) == escape(target_url) ||
         escape(unescape(win.location)) == escape(target_popup_url)) && 
        win.document && 
        win.document.body && 
        (win.document.body.textContent == body ||
         win.document.body.textContent == popup_body) && 
        win.document.readyState == "complete") {
      if (!contains(win, finishedWindows)) {
        finishedWindows.push(win);
        frameFinished();
      }
    }
    for (var i = 0; i < win.frames.length; i++)
      searchForFinishedFrames(win.frames[i]);
  }

  function poll() {
    try {
      
      
      xpcEnumerateContentWindows(searchForFinishedFrames);
    } catch(ex) {
      
      
    }
  }

  var frameWaitInterval = setInterval(poll, 500);
}

