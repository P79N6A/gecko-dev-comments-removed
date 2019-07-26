



const CC = Components.Constructor;

const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init");

function testAddrInUse()
{
  
  
  if ("@mozilla.org/windows-registry-key;1" in Cc) {
    return;
  }

  
  
  let listener = ServerSocket(-1, true, -1);
  do_check_true(listener instanceof Ci.nsIServerSocket);

  
  do_check_throws_nsIException(() => ServerSocket(listener.port, true, -1),
                               "NS_ERROR_SOCKET_ADDRESS_IN_USE");
}

function run_test()
{
  testAddrInUse();
}
