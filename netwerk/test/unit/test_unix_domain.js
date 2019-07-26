

const CC = Components.Constructor;

const UnixServerSocket = CC("@mozilla.org/network/server-socket;1",
                            "nsIServerSocket",
                            "initWithFilename");

const ScriptableInputStream = CC("@mozilla.org/scriptableinputstream;1",
                                 "nsIScriptableInputStream",
                                 "init");

const IOService = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);
const socketTransportService = Cc["@mozilla.org/network/socket-transport-service;1"]
                               .getService(Ci.nsISocketTransportService);

const threadManager = Cc["@mozilla.org/thread-manager;1"].getService();

const allPermissions = parseInt("777", 8);

function run_test()
{
  
  if ("@mozilla.org/windows-registry-key;1" in Cc) {
    test_not_supported();
    return;
  }

  add_test(test_echo);
  add_test(test_name_too_long);
  add_test(test_no_directory);
  add_test(test_no_such_socket);
  add_test(test_address_in_use);
  add_test(test_file_in_way);
  add_test(test_create_permission);
  add_test(test_connect_permission);
  add_test(test_long_socket_name);
  add_test(test_keep_when_offline);

  run_next_test();
}


function test_not_supported()
{
  let socketName = do_get_tempdir();
  socketName.append('socket');
  do_print("creating socket: " + socketName.path);

  do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                               "NS_ERROR_SOCKET_ADDRESS_NOT_SUPPORTED");

  do_check_throws_nsIException(() => socketTransportService.createUnixDomainTransport(socketName),
                               "NS_ERROR_SOCKET_ADDRESS_NOT_SUPPORTED");
}


function test_echo()
{
  let log = '';

  let socketName = do_get_tempdir();
  socketName.append('socket');

  
  do_print("creating socket: " + socketName.path);
  let server = new UnixServerSocket(socketName, allPermissions, -1);
  server.asyncListen({
    onSocketAccepted: function(aServ, aTransport) {
      do_print("called test_echo's onSocketAccepted");
      log += 'a';

      do_check_eq(aServ, server);

      let connection = aTransport;

      
      let connectionSelfAddr = connection.getScriptableSelfAddr();
      do_check_eq(connectionSelfAddr.family, Ci.nsINetAddr.FAMILY_LOCAL);
      do_check_eq(connectionSelfAddr.address, socketName.path);

      
      
      do_check_eq(connection.host, '');
      do_check_eq(connection.port, 0);
      let connectionPeerAddr = connection.getScriptablePeerAddr();
      do_check_eq(connectionPeerAddr.family, Ci.nsINetAddr.FAMILY_LOCAL);
      do_check_eq(connectionPeerAddr.address, '');

      let serverAsyncInput = connection.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
      let serverOutput = connection.openOutputStream(0, 0, 0);

      serverAsyncInput.asyncWait(function (aStream) {
        do_print("called test_echo's server's onInputStreamReady");
        let serverScriptableInput = new ScriptableInputStream(aStream);

        
        do_check_eq(serverScriptableInput.readBytes(17), "Mervyn Murgatroyd");
        do_print("server has read message from client");
        serverOutput.write("Ruthven Murgatroyd", 18);
        do_print("server has written to client");
      }, 0, 0, threadManager.currentThread);
    },

    onStopListening: function(aServ, aStatus) {
      do_print("called test_echo's onStopListening");
      log += 's';

      do_check_eq(aServ, server);
      do_check_eq(log, 'acs');

      run_next_test();
    }
  });

  
  let client = socketTransportService.createUnixDomainTransport(socketName);
  do_check_eq(client.host, socketName.path);
  do_check_eq(client.port, 0);

  let clientAsyncInput = client.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
  let clientInput = new ScriptableInputStream(clientAsyncInput);
  let clientOutput = client.openOutputStream(0, 0, 0);

  clientOutput.write("Mervyn Murgatroyd", 17);
  do_print("client has written to server");

  clientAsyncInput.asyncWait(function (aStream) {
    do_print("called test_echo's client's onInputStreamReady");
    log += 'c';

    do_check_eq(aStream, clientAsyncInput);

    
    
    let clientSelfAddr = client.getScriptableSelfAddr();
    do_check_eq(clientSelfAddr.family, Ci.nsINetAddr.FAMILY_LOCAL);
    do_check_eq(clientSelfAddr.address, '');

    do_check_eq(client.host, socketName.path); 
    let clientPeerAddr = client.getScriptablePeerAddr();
    do_check_eq(clientPeerAddr.family, Ci.nsINetAddr.FAMILY_LOCAL);
    do_check_eq(clientPeerAddr.address, socketName.path);

    do_check_eq(clientInput.readBytes(18), "Ruthven Murgatroyd");
    do_print("client has read message from server");

    server.close();
  }, 0, 0, threadManager.currentThread);
}


function test_name_too_long()
{
  let socketName = do_get_tempdir();
  
  socketName.append(new Array(1000).join('x'));

  
  
  

  do_check_throws_nsIException(() => new UnixServerSocket(socketName, 0, -1),
                               "NS_ERROR_FILE_NAME_TOO_LONG");

  
  
  
  do_check_throws_nsIException(() => socketTransportService.createUnixDomainTransport(socketName),
                               "NS_ERROR_FILE_NAME_TOO_LONG");

  run_next_test();
}


function test_no_directory()
{
  let socketName = do_get_tempdir();
  socketName.append('directory-that-does-not-exist');
  socketName.append('socket');

  do_check_throws_nsIException(() => new UnixServerSocket(socketName, 0, -1),
                               "NS_ERROR_FILE_NOT_FOUND");

  run_next_test();
}


function test_no_such_socket()
{
  let socketName = do_get_tempdir();
  socketName.append('nonexistent-socket');

  let client = socketTransportService.createUnixDomainTransport(socketName);
  let clientAsyncInput = client.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
  clientAsyncInput.asyncWait(function (aStream) {
    do_print("called test_no_such_socket's onInputStreamReady");

    do_check_eq(aStream, clientAsyncInput);

    
    
    
    do_check_throws_nsIException(() => clientAsyncInput.available(),
                                 "NS_ERROR_FILE_NOT_FOUND");

    clientAsyncInput.close();
    client.close(Cr.NS_OK);

    run_next_test();
  }, 0, 0, threadManager.currentThread);
}



function test_address_in_use()
{
  let socketName = do_get_tempdir();
  socketName.append('socket-in-use');

  
  let server = new UnixServerSocket(socketName, allPermissions, -1);

  
  do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                               "NS_ERROR_SOCKET_ADDRESS_IN_USE");

  run_next_test();
}


function test_file_in_way()
{
  let socketName = do_get_tempdir();
  socketName.append('file_in_way');

  
  socketName.create(Ci.nsIFile.NORMAL_FILE_TYPE, allPermissions);

  
  do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                               "NS_ERROR_SOCKET_ADDRESS_IN_USE");

  
  socketName.append('socket');
  do_check_throws_nsIException(() => new UnixServerSocket(socketName, 0, -1),
                               "NS_ERROR_FILE_NOT_DIRECTORY");

  run_next_test();
}



function test_create_permission()
{
  let dirName = do_get_tempdir();
  dirName.append('unfriendly');

  let socketName = dirName.clone();
  socketName.append('socket');

  
  
  try {
    
    dirName.create(Ci.nsIFile.DIRECTORY_TYPE, 0);

    
    
    
    do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                                 "NS_ERROR_CONNECTION_REFUSED");

    
    dirName.permissions = parseInt("0555", 8);

    
    do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                                 "NS_ERROR_CONNECTION_REFUSED");

  } finally {
    
    dirName.permissions = allPermissions;
  }

  
  
  do_check_instanceof(new UnixServerSocket(socketName, allPermissions, -1),
                      Ci.nsIServerSocket);

  run_next_test();
}




function test_connect_permission()
{
  
  
  let log = '';

  
  let dirName = do_get_tempdir();
  dirName.append('inhospitable');
  dirName.create(Ci.nsIFile.DIRECTORY_TYPE, allPermissions);

  let socketName = dirName.clone();
  socketName.append('socket');

  
  
  let server = new UnixServerSocket(socketName, allPermissions, -1);
  server.asyncListen({ onSocketAccepted: socketAccepted, onStopListening: stopListening });

  
  dirName.permissions = 0;

  let client3;

  let client1 = socketTransportService.createUnixDomainTransport(socketName);
  let client1AsyncInput = client1.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
  client1AsyncInput.asyncWait(function (aStream) {
    do_print("called test_connect_permission's client1's onInputStreamReady");
    log += '1';

    
    
    do_check_throws_nsIException(() => client1AsyncInput.available(),
                                 "NS_ERROR_CONNECTION_REFUSED");

    client1AsyncInput.close();
    client1.close(Cr.NS_OK);

    
    dirName.permissions = allPermissions;
    socketName.permissions = 0;

    let client2 = socketTransportService.createUnixDomainTransport(socketName);
    let client2AsyncInput = client2.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
    client2AsyncInput.asyncWait(function (aStream) {
      do_print("called test_connect_permission's client2's onInputStreamReady");
      log += '2';

      do_check_throws_nsIException(() => client2AsyncInput.available(),
                                   "NS_ERROR_CONNECTION_REFUSED");

      client2AsyncInput.close();
      client2.close(Cr.NS_OK);

      
      socketName.permissions = allPermissions;

      client3 = socketTransportService.createUnixDomainTransport(socketName);

      let client3Output = client3.openOutputStream(0, 0, 0);
      client3Output.write("Hanratty", 8);

      let client3AsyncInput = client3.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
      client3AsyncInput.asyncWait(client3InputStreamReady, 0, 0, threadManager.currentThread);
    }, 0, 0, threadManager.currentThread);
  }, 0, 0, threadManager.currentThread);

  function socketAccepted(aServ, aTransport) {
    do_print("called test_connect_permission's onSocketAccepted");
    log += 'a';

    let serverInput = aTransport.openInputStream(0, 0, 0).QueryInterface(Ci.nsIAsyncInputStream);
    let serverOutput = aTransport.openOutputStream(0, 0, 0);

    serverInput.asyncWait(function (aStream) {
      do_print("called test_connect_permission's socketAccepted's onInputStreamReady");
      log += 'i';

      
      let serverScriptableInput = new ScriptableInputStream(serverInput);
      do_check_eq(serverScriptableInput.readBytes(8), "Hanratty");
      serverOutput.write("Ferlingatti", 11);
    }, 0, 0, threadManager.currentThread);
  }

  function client3InputStreamReady(aStream) {
    do_print("called client3's onInputStreamReady");
    log += '3';

    let client3Input = new ScriptableInputStream(aStream);

    do_check_eq(client3Input.readBytes(11), "Ferlingatti");

    client3.close(Cr.NS_OK);
    server.close();
  }

  function stopListening(aServ, aStatus) {
    do_print("called test_connect_permission's server's stopListening");
    log += 's';

    do_check_eq(log, '12ai3s');

    run_next_test();
  }
}


function test_long_socket_name()
{
  let socketName = do_get_tempdir();
  socketName.append(new Array(10000).join('long'));

  
  do_check_throws_nsIException(() => new UnixServerSocket(socketName, allPermissions, -1),
                               "NS_ERROR_FILE_NAME_TOO_LONG");

  
  do_check_throws_nsIException(() => socketTransportService.createUnixDomainTransport(socketName),
                               "NS_ERROR_FILE_NAME_TOO_LONG");

  run_next_test();
}


function test_keep_when_offline()
{
  let log = '';

  let socketName = do_get_tempdir();
  socketName.append('keep-when-offline');

  
  let listener = new UnixServerSocket(socketName, allPermissions, -1);
  listener.asyncListen({ onSocketAccepted: onAccepted, onStopListening: onStopListening });

  
  let client = socketTransportService.createUnixDomainTransport(socketName);
  let clientOutput = client.openOutputStream(0, 0, 0);
  let clientInput = client.openInputStream(0, 0, 0);
  clientInput.asyncWait(clientReady, 0, 0, threadManager.currentThread);
  let clientScriptableInput = new ScriptableInputStream(clientInput);

  let server, serverInput, serverScriptableInput, serverOutput;

  
  let count = 0;

  
  function onAccepted(aListener, aServer) {
    do_print("test_keep_when_offline: onAccepted called");
    log += 'a';
    do_check_eq(aListener, listener);
    server = aServer;

    
    serverInput = server.openInputStream(0, 0, 0);
    serverInput.asyncWait(serverReady, 0, 0, threadManager.currentThread);
    serverScriptableInput = new ScriptableInputStream(serverInput);

    
    serverOutput = server.openOutputStream(0, 0, 0);
    serverOutput.write("After you, Alphonse!", 20);
    count++;
  }

  
  function clientReady(aStream) {
    log += 'c';
    do_print("test_keep_when_offline: clientReady called: " + log);
    do_check_eq(aStream, clientInput);

    
    let available;
    try {
      available = clientInput.available();
    } catch (ex) {
      do_check_instanceof(ex, Ci.nsIException);
      do_check_eq(ex.result, Cr.NS_BASE_STREAM_CLOSED);

      do_print("client received end-of-stream; closing client output stream");
      log += ')';

      client.close(Cr.NS_OK);

      
      
      
      listener.close();
    }

    if (available) {
      
      do_check_eq(clientScriptableInput.readBytes(20), "After you, Alphonse!");

      
      clientOutput.write("No, after you, Gaston!", 22);

      
      clientInput.asyncWait(clientReady, 0, 0, threadManager.currentThread);
    }
  }

  function serverReady(aStream) {
    log += 's';
    do_print("test_keep_when_offline: serverReady called: " + log);
    do_check_eq(aStream, serverInput);

    
    do_check_eq(serverScriptableInput.readBytes(22), "No, after you, Gaston!");

    
    
    if (count == 5) {
      IOService.offline = true;
      log += 'o';
    }

    if (count < 10) {
      
      serverOutput.write("After you, Alphonse!", 20);
      count++;

      
      
      serverInput.asyncWait(serverReady, 0, 0, threadManager.currentThread);
    } else if (count == 10) {
      
      
      
      do_print("closing server transport");
      server.close(Cr.NS_OK);
      log += '(';
    }
  }

  
  function onStopListening(aServ, aStatus) {
    do_print("test_keep_when_offline: onStopListening called");
    log += 'L';
    do_check_eq(log, 'acscscscscsocscscscscs(c)L');

    do_check_eq(aServ, listener);
    do_check_eq(aStatus, Cr.NS_BINDING_ABORTED);

    run_next_test();
  }
}
