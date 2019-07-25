


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);

let connection = navigator.mozMobileConnection;
ok(connection instanceof MozMobileConnection,
   "connection is instanceof " + connection.constructor);



is(connection.iccInfo.mcc, 310);
is(connection.iccInfo.mnc, 260);

SpecialPowers.removePermission("mobileconnection", document);
finish();
