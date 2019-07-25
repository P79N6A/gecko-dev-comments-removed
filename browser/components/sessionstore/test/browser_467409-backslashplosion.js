

















function test() {
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  let blankState = { windows: [{ tabs: [{ entries: [{ url: "about:blank" }] }]}]};
  let crashState = { windows: [{ tabs: [{ entries: [{ url: "about:mozilla" }] }]}]};

  let pagedata = { url: "about:sessionrestore",
                   formdata: { "#sessionData": crashState } };
  let state = { windows: [{ tabs: [{ entries: [pagedata] }] }] };

  
  test1(state);


  function test1(aState) {
    waitForBrowserState(aState, function() {
      checkState("test1", test2);
    });
  }

  function test2(aState) {
    let pagedata2 = { url: "about:sessionrestore",
                      formdata: { "#sessionData": aState } };
    let state2 = { windows: [{ tabs: [{ entries: [pagedata2] }] }] };

    waitForBrowserState(state2, function() {
      checkState("test2", test3);
    });
  }

  function test3(aState) {
    let pagedata3 = { url: "about:sessionrestore",
                      formdata: { "#sessionData": JSON.stringify(crashState) } };
    let state3 = { windows: [{ tabs: [{ entries: [pagedata3] }] }] };
    waitForBrowserState(state3, function() {
      
      
      
      checkState("test3", function() waitForBrowserState(blankState, finish));
    });
  }

  function checkState(testName, callback) {
    let curState = JSON.parse(ss.getBrowserState());
    let formdata = curState.windows[0].tabs[0].entries[0].formdata;

    ok(formdata["#sessionData"], testName + ": we have form data for about:sessionrestore");

    let sessionData_raw = JSON.stringify(formdata["#sessionData"]);
    ok(!/\\/.test(sessionData_raw), testName + ": #sessionData contains no backslashes");
    info(sessionData_raw);

    let gotError = false;
    try {
      JSON.parse(formdata["#sessionData"]);
    }
    catch (e) {
      info(testName + ": got error: " + e);
      gotError = true;
    }
    ok(gotError, testName + ": attempting to JSON.parse form data threw error");

    
    
    
    delete curState.windows[0].extData;
    delete curState.windows[0].tabs[0].extData;
    callback(curState);
  }

}

