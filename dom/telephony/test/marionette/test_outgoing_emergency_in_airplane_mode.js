


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startTestWithPermissions(['mobileconnection'], function() {
  let connection = navigator.mozMobileConnections[0];
  ok(connection instanceof MozMobileConnection,
     "connection is instanceof " + connection.constructor);

  let outCall;

  gSetRadioEnabled(connection, false)
    .then(() => gDial("112"))
    .then(call => { outCall = call; })
    .then(() => gRemoteAnswer(outCall))
    .then(() => gDelay(1000))  
    .then(() => gRemoteHangUp(outCall))
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
