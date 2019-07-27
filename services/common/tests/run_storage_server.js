













Cu.import("resource://testing-common/services/common/storageserver.js");

initTestLogging();

let server = new StorageServer();
server.allowAllUsers = true;
server.startSynchronous(SERVER_PORT);
_("Storage server started on port " + SERVER_PORT);


_do_main();
