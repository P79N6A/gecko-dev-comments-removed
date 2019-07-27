const ALLOW_ACTION = SpecialPowers.Ci.nsIPermissionManager.ALLOW_ACTION;
const DENY_ACTION = SpecialPowers.Ci.nsIPermissionManager.DENY_ACTION;
const UNKNOWN_ACTION = SpecialPowers.Ci.nsIPermissionManager.UNKNOWN_ACTION;
const PROMPT_ACTION = SpecialPowers.Ci.nsIPermissionManager.PROMPT_ACTION;




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
