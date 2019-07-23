(function() {

  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);

  function determinePrefKind(branch, prefName) {
    switch (branch.getPrefType(prefName)) {
    case branch.PREF_STRING:    return "CharPref";
    case branch.PREF_INT:       return "IntPref";
    case branch.PREF_BOOL:      return "BoolPref";
    default:  return "ComplexValue";
    }
  }
  
  function memoize(fn, obj) {
    var cache = {}, sep = '___',
        join = Array.prototype.join;
    return function() {
      var key = join.call(arguments, sep);
      if (!(key in cache))
        cache[key] = fn.apply(obj, arguments);
      return cache[key];
    };
  }
  
  var makeAccessor = memoize(function(pref) {
    var splat = pref.split('.'),
        basePref = splat.pop(),
        branch, kind;
    
    try {
      branch = prefService.getBranch(splat.join('.') + '.')
    } catch (e) {
      alert("Calling prefService.getBranch failed: " + 
        "did you read the NOTE in mozprefs.js?");
      throw e;
    }
    
    kind = determinePrefKind(branch, basePref);
    
    return function(value) {
      var oldValue = branch['get' + kind](basePref);
      if (arguments.length > 0)
        branch['set' + kind](basePref, value);
      return oldValue;
    };
  });

  
























  function pref(name,  value, fn, obj) {
    var acc = makeAccessor(name);
    switch (arguments.length) {
    case 1: return acc();
    case 2: return acc(value);
    default:
      var oldValue = acc(value),
          extra_args = [].slice.call(arguments, 4);
      try { return fn.apply(obj, extra_args) }
      finally { acc(oldValue) } 
    }
  };
  
  window.pref = pref; 
 
})();
