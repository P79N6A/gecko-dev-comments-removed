









































var testRunner = {
  _testIterator: null,
  _lastEventResult: undefined,
  _testRunning: false,
  _eventRaised: false,

  

  










  runTest: function TR_runTest(aGenerator) {
    waitForExplicitFinish();
    testRunner._testIterator = aGenerator();
    testRunner.continueTest();
  },

  





  continueTest: function TR_continueTest(aEventResult) {
    
    testRunner._lastEventResult = aEventResult;

    
    if (testRunner._testRunning) {
      testRunner._eventRaised = true;
      return;
    }

    
    testRunner._testRunning = true;
    try {
      do {
        
        
        testRunner._eventRaised = false;
        testRunner._testIterator.send(testRunner._lastEventResult);
      } while (testRunner._eventRaised);
    }
    catch (e) {
      
      
      
      if (!(e instanceof StopIteration))
        ok(false, e);
      
      finish();
    }

    
    testRunner._testRunning = false;
  },

  

  _isFirstEvent: true,

  


  continueAfterTwoEvents: function TR_continueAfterTwoEvents() {
    if (testRunner._isFirstEvent) {
      testRunner._isFirstEvent = false;
      return;
    }
    testRunner._isFirstEvent = true;
    testRunner.continueTest();
  },

  

  










  chainGenerator: function TR_chainGenerator(aArrayOfGenerators) {
    
    for (let [, curGenerator] in Iterator(aArrayOfGenerators)) {
      var curIterator = curGenerator();
      
      
      try {
        var value = undefined;
        while (true) {
          value = yield curIterator.send(value);
        }
      }
      catch(e if e instanceof StopIteration) {
        
      }
    }
  },

  










  runTests: function TR_runTests(aArrayOfTestGenerators) {
    testRunner.runTest(function() {
      return testRunner.chainGenerator(aArrayOfTestGenerators);
    });
  }
};
