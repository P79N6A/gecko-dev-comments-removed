



var ScratchpadManager = Scratchpad.ScratchpadManager;




function asyncMap(items, iterator, callback)
{
  let expected = items.length;
  let results = [];

  items.forEach(function(item) {
    iterator(item, function(result) {
      results.push(result);
      if (results.length == expected) {
        callback(results);
      }
    });
  });
}

function test()
{
  waitForExplicitFinish();
  testRestore();
}

function testRestore()
{
  let states = [
    {
      filename: "testfile",
      text: "test1",
      executionContext: 2
    },
    {
      text: "text2",
      executionContext: 1
    },
    {
      text: "text3",
      executionContext: 1
    }
  ];

  asyncMap(states, function(state, done) {
    
    let win = ScratchpadManager.openScratchpad(state);
    win.addEventListener("load", function onScratchpadLoad() {
      removeEventListener("load", onScratchpadLoad, false);
      done(win);
    }, false)
  }, function(wins) {
    
    ScratchpadManager.saveOpenWindows();

    
    let session = ScratchpadManager.getSessionState();

    
    wins.forEach(function(win) {
      win.close();
    });

    
    ScratchpadManager.saveOpenWindows();

    
    let restoredWins = ScratchpadManager.restoreSession(session);

    is(restoredWins.length, 3, "Three scratchad windows restored");

    asyncMap(restoredWins, function(restoredWin, done) {
      restoredWin.addEventListener("load", function onScratchpadLoad() {
        restoredWin.removeEventListener("load", onScratchpadLoad, false);
        let state = restoredWin.Scratchpad.getState();
        restoredWin.close();
        done(state);
      }, false);
    }, function(restoredStates) {
      
      ok(statesMatch(restoredStates, states),
        "All scratchpad window states restored correctly");

      
      finish();
    });
  });
}

function statesMatch(restoredStates, states)
{
  return states.every(function(state) {
    return restoredStates.some(function(restoredState) {
      return state.filename == restoredState.filename
        && state.text == restoredState.text
        && state.executionContext == restoredState.executionContext;
    })
  });
}
