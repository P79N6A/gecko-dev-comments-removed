netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

var gWindowUtils;

try {
  gWindowUtils = window.QueryInterface(CI.nsIInterfaceRequestor).getInterface(CI.nsIDOMWindowUtils);
  if (gWindowUtils && !gWindowUtils.compareCanvases)
    gWindowUtils = null;
} catch (e) {
  gWindowUtils = null;
}

function snapshotWindow(win, withCaret) {
  
  
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  var el = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  el.width = win.innerWidth;
  el.height = win.innerHeight;

  var ctx = el.getContext("2d");
  ctx.drawWindow(win, win.scrollX, win.scrollY,
                 win.innerWidth, win.innerHeight,
                 "rgb(255,255,255)",
                 withCaret ? ctx.DRAWWINDOW_DRAW_CARET : 0);
  return el;
}




function compareSnapshots(s1, s2, expected) {
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  var s1Str, s2Str;
  var correct = false;
  if (gWindowUtils) {
    correct = ((gWindowUtils.compareCanvases(s1, s2, {}) == 0) == expected);
  }

  if (!correct) {
    s1Str = s1.toDataURL();
    s2Str = s2.toDataURL();

    if (!gWindowUtils) {
	correct = ((s1Str == s2Str) == expected);
    }
  }

  return [correct, s1Str, s2Str];
}
