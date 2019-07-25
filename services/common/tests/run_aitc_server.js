













Cu.import("resource://testing-common/services-common/aitcserver.js");

initTestLogging();

let server = new AITCServer10Server();
server.autoCreateUsers = true;
server.start(SERVER_PORT);

_("AITC server started on port " + SERVER_PORT);


_do_main();
