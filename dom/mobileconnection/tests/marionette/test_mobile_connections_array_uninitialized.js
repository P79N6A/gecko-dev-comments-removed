


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

startTestCommon(function() {
  let connections =
    workingFrame.contentWindow.navigator.mozMobileConnections;

  let num = getNumOfRadioInterfaces();
  is(connections.length, num, "ril.numRadioInterfaces");
});
