





function run_test() {
  let testFile = do_get_file("xpcshell_debugging_script.js");

  
  let testResumed = false;
  let DebuggerServer = _setupDebuggerServer([testFile.path], () => testResumed = true);
  let transport = DebuggerServer.connectPipe();
  let client = new DebuggerClient(transport);
  client.connect(() => {
    
    client.listTabs(response => {
      let chromeDebugger = response.chromeDebugger;
      client.attachThread(chromeDebugger, (response, threadClient) => {
        threadClient.addOneTimeListener("paused", (event, packet) => {
        equal(packet.why.type, "breakpoint",
              "yay - hit the breakpoint at the first line in our script");
          
          threadClient.addOneTimeListener("paused", (event, packet) => {
            equal(packet.why.type, "debuggerStatement",
                  "yay - hit the 'debugger' statement in our script");
            threadClient.resume(() => {
              finishClient(client);
            });
          });
          threadClient.resume();
        });
        
        
        threadClient.resume(response => {
          
          ok(testResumed);
          
          load(testFile.path);
          
        });
      });
    });
  });
  do_test_pending();
}
