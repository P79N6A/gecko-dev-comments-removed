


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


startTestCommon(function() {
  
  is(mobileConnection.lastKnownNetwork, "310-260");
  
  is(mobileConnection.lastKnownHomeNetwork, "310-260-Android");
}, ["mobilenetwork"]);
