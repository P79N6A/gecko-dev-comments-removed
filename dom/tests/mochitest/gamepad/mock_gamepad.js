

var GamepadService = (function() {
  return SpecialPowers.Cc["@mozilla.org/gamepad-test;1"].getService(SpecialPowers.Ci.nsIGamepadServiceTest);
})();
