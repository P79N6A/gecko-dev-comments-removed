



var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
var uri = ioService.newURI("http://localhost:8088/async-notification-never-here.jpg", null, null);

load('async_load_tests.js');
