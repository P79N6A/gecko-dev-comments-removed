


"use strict";

devtools.lazyRequireGetter(this, "cert",
  "devtools/toolkit/security/cert");



function run_test() {
  
  do_get_profile();
  
  Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);
  run_next_test();
}

function connectClient(client) {
  let deferred = promise.defer();
  client.connect(() => {
    client.listTabs(deferred.resolve);
  });
  return deferred.promise;
}

add_task(function*() {
  initTestDebuggerServer();
});


add_task(function*() {
  equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");

  
  let serverCert = yield cert.local.getOrCreate();

  let oobData = promise.defer();
  let AuthenticatorType = DebuggerServer.Authenticators.get("OOB_CERT");
  let serverAuth = new AuthenticatorType.Server();
  serverAuth.allowConnection = () => {
    return DebuggerServer.AuthenticationResult.ALLOW;
  };
  serverAuth.receiveOOB = () => oobData.promise; 

  let listener = DebuggerServer.createListener();
  ok(listener, "Socket listener created");
  listener.portOrPath = -1 ;
  listener.encryption = true ;
  listener.authenticator = serverAuth;
  yield listener.open();
  equal(DebuggerServer.listeningSockets, 1, "1 listening socket");

  let clientAuth = new AuthenticatorType.Client();
  clientAuth.sendOOB = ({ oob }) => {
    do_print(oob);
    
    oobData.resolve(oob);
  };

  let transport = yield DebuggerClient.socketConnect({
    host: "127.0.0.1",
    port: listener.port,
    encryption: true,
    authenticator: clientAuth,
    cert: {
      sha256: serverCert.sha256Fingerprint
    }
  });
  ok(transport, "Client transport created");

  let client = new DebuggerClient(transport);
  let onUnexpectedClose = () => {
    do_throw("Closed unexpectedly");
  };
  client.addListener("closed", onUnexpectedClose);
  yield connectClient(client);

  
  let message = "secrets";
  let reply = yield client.request({
    to: "root",
    type: "echo",
    message
  });
  equal(reply.message, message, "Encrypted echo matches");

  client.removeListener("closed", onUnexpectedClose);
  transport.close();
  listener.close();
  equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");
});


add_task(function*() {
  equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");

  let oobData = promise.defer();
  let AuthenticatorType = DebuggerServer.Authenticators.get("OOB_CERT");
  let serverAuth = new AuthenticatorType.Server();
  serverAuth.allowConnection = () => {
    return DebuggerServer.AuthenticationResult.ALLOW;
  };
  serverAuth.receiveOOB = () => oobData.promise; 

  let listener = DebuggerServer.createListener();
  ok(listener, "Socket listener created");
  listener.portOrPath = -1 ;
  listener.encryption = true ;
  listener.authenticator = serverAuth;
  yield listener.open();
  equal(DebuggerServer.listeningSockets, 1, "1 listening socket");

  
  
  let transport = yield DebuggerClient.socketConnect({
    host: "127.0.0.1",
    port: listener.port,
    encryption: true
    
  });

  
  let deferred = promise.defer();
  let client = new DebuggerClient(transport);
  client.onPacket = packet => {
    
    
    ok(!packet.from && packet.authResult, "Got auth packet instead of data");
    deferred.resolve();
  };
  client.connect();
  yield deferred.promise;

  
  let message = "secrets";
  try {
    yield client.request({
      to: "root",
      type: "echo",
      message
    });
  } catch(e) {
    ok(true, "Sending a message failed");
    transport.close();
    listener.close();
    equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");
    return;
  }

  do_throw("Connection unexpectedly succeeded");
});


add_task(function*() {
  equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");

  
  let serverCert = yield cert.local.getOrCreate();

  let oobData = promise.defer();
  let AuthenticatorType = DebuggerServer.Authenticators.get("OOB_CERT");
  let serverAuth = new AuthenticatorType.Server();
  serverAuth.allowConnection = () => {
    return DebuggerServer.AuthenticationResult.ALLOW;
  };
  serverAuth.receiveOOB = () => oobData.promise; 

  let clientAuth = new AuthenticatorType.Client();
  clientAuth.sendOOB = ({ oob }) => {
    do_print(oob);
    do_print("Modifying K value, should fail");
    
    oobData.resolve({
      k: oob.k + 1,
      sha256: oob.sha256
    });
  };

  let listener = DebuggerServer.createListener();
  ok(listener, "Socket listener created");
  listener.portOrPath = -1 ;
  listener.encryption = true ;
  listener.authenticator = serverAuth;
  yield listener.open();
  equal(DebuggerServer.listeningSockets, 1, "1 listening socket");

  try {
    yield DebuggerClient.socketConnect({
      host: "127.0.0.1",
      port: listener.port,
      encryption: true,
      authenticator: clientAuth,
      cert: {
        sha256: serverCert.sha256Fingerprint
      }
    });
  } catch(e) {
    ok(true, "Client failed to connect as expected");
    listener.close();
    equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");
    return;
  }

  do_throw("Connection unexpectedly succeeded");
});


add_task(function*() {
  equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");

  
  let serverCert = yield cert.local.getOrCreate();

  let oobData = promise.defer();
  let AuthenticatorType = DebuggerServer.Authenticators.get("OOB_CERT");
  let serverAuth = new AuthenticatorType.Server();
  serverAuth.allowConnection = () => {
    return DebuggerServer.AuthenticationResult.ALLOW;
  };
  serverAuth.receiveOOB = () => oobData.promise; 

  let clientAuth = new AuthenticatorType.Client();
  clientAuth.sendOOB = ({ oob }) => {
    do_print(oob);
    do_print("Modifying cert hash, should fail");
    
    oobData.resolve({
      k: oob.k,
      sha256: oob.sha256 + 1
    });
  };

  let listener = DebuggerServer.createListener();
  ok(listener, "Socket listener created");
  listener.portOrPath = -1 ;
  listener.encryption = true ;
  listener.authenticator = serverAuth;
  yield listener.open();
  equal(DebuggerServer.listeningSockets, 1, "1 listening socket");

  try {
    yield DebuggerClient.socketConnect({
      host: "127.0.0.1",
      port: listener.port,
      encryption: true,
      authenticator: clientAuth,
      cert: {
        sha256: serverCert.sha256Fingerprint
      }
    });
  } catch(e) {
    ok(true, "Client failed to connect as expected");
    listener.close();
    equal(DebuggerServer.listeningSockets, 0, "0 listening sockets");
    return;
  }

  do_throw("Connection unexpectedly succeeded");
});

add_task(function*() {
  DebuggerServer.destroy();
});
