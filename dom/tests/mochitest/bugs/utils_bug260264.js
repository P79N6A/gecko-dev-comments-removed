(function() {
  
  document.getElementsByTagName("head").item(0)
    .appendChild(document.createElement("script")).src =
      "/tests/SimpleTest/EventUtils.js";
})();




function send(element, event, handler) {
  function unique_handler() { return handler.apply(this, arguments) }
  element.addEventListener(event, unique_handler, false);
  try { sendMouseEvent({ type: event }, element.id) }
  finally { element.removeEventListener(event, unique_handler, false) }
}





(function(originalOpen) {
  var wins = [];
  (window.open = function() {
    var win = originalOpen.apply(window, arguments);
    if (win)
      wins[wins.length] = win;
    return win;
  }).close = function(n) {
    if (arguments.length < 1)
      n = wins.length;
    while (n --> 0) {
      var win = wins.pop();
      if (win) win.close();
      else break;
    }
  };
})(window.open);

function _alter_helper(uri, fn) {
  var hash_splat = uri.split("#"),
      splat = hash_splat.shift().split("/");
  fn(splat);
  hash_splat.unshift(splat.join("/"));
  return hash_splat.join("#");
}

function alter_host(uri, host) {
  return _alter_helper(uri, function(splat) {
    splat.splice(2, 1, host);
  });
}

function alter_file(uri, file) {
  return _alter_helper(uri, function(splat) {
    splat[splat.length - 1] = file;
  });
}

(function() {

  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService),
      pm = Components.classes["@mozilla.org/permissionmanager;1"]
                     .getService(Components.interfaces.nsIPermissionManager),
      ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);

  ALLOW_ACTION = pm.ALLOW_ACTION;
  DENY_ACTION = pm.DENY_ACTION;
  UNKNOWN_ACTION = pm.UNKNOWN_ACTION;

  




  makePrefAccessor = function(pref) {
    var splat = pref.split('.'),
        basePref = splat.pop(),
        branch, kind;

    try {
      branch = prefService.getBranch(splat.join('.') + '.');
    } catch (x) {
      alert("Calling prefService.getBranch failed: " + 
        "did you forget to enable UniversalXPConnect?");
      throw x;
    }

    switch (branch.getPrefType(basePref)) {
    case branch.PREF_STRING:  kind = "CharPref"; break;
    case branch.PREF_INT:     kind = "IntPref"; break;
    case branch.PREF_BOOL:    kind = "BoolPref"; break;
    case branch.PREF_INVALID: kind = "ComplexValue";
    }

    return function(value) {
      var oldValue = branch['get' + kind](basePref);
      if (arguments.length > 0)
        branch['set' + kind](basePref, value);
      return oldValue;
    };
  };

  makePopupPrivAccessor = function(uri) {
    uri = ioService.newURI(uri, null, null);
    return function(permission) {
      var old = pm.testPermission(uri, "popup");
      if (arguments.length) {
        pm.remove(uri.host, "popup");
        pm.add(uri, "popup", permission);
      }
      return old;
    };
  };

})();
















function hold(accessor, value, body) {
  var old_value = accessor(value);
  try { return body(value, old_value) }
  finally {
    old_value = accessor(old_value);
    if (old_value !== value)
      throw [accessor, value, old_value];
  }
}
