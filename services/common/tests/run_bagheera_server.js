













Cu.import("resource://testing-common/services/common/bagheeraserver.js");

initTestLogging();

let server = new BagheeraServer();
server.allowAllNamespaces = true;
server.start(SERVER_PORT);
_("Bagheera server started on port " + SERVER_PORT);


_do_main();

