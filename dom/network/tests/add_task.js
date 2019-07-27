

SimpleTest.waitForExplicitFinish();
(function(scope) {
  var pendingTasks = [];
  var pendingPromise = null;

  
  
  
  function spawn(generatorFunc) {
    return new Promise(function(resolve, reject) {
      try {
        var iterator = generatorFunc();
      }
      catch (ex) {
        ok(false, 'Problem invoking generator func: ' + ex + ': ' + ex.stack);
        return;
      }
      var stepResolved = function(result) {
        try {
          var iterStep = iterator.next(result);
        }
        catch (ex) {
          ok(false, 'Problem invoking iterator step: ' + ex + ': ' + ex.stack);
          return;
        }
        if (iterStep.done) {
          resolve(iterStep.value);
          return;
        }
        if (!iterStep.value || !iterStep.value.then) {
          ok(false, 'Iterator step returned non-Promise: ' + iterStep.value);
        }
        iterStep.value.then(stepResolved, generalErrback);
      };
      stepResolved();
    });
  }

  function maybeSpawn(promiseOrGenerator) {
    if (promiseOrGenerator.then) {
      return promiseOrGenerator;
    }
    return spawn(promiseOrGenerator);
  }

  scope.add_task = function(thing) {
    pendingTasks.push(thing);
  };

  function generalErrback(ex) {
    ok(false,
       'A rejection happened: ' +
       (ex ? (ex + ': ' + ex.stack) : ''));
  }

  function runNextTask() {
    if (pendingTasks.length) {
      pendingPromise = maybeSpawn(pendingTasks.shift());
      pendingPromise.then(runNextTask, generalErrback);
    } else {
      SimpleTest.finish();
    }
  }

  
  
  
  
  var running = false;
  function maybeStartRunning() {
    if (!running && document.readyState === 'complete') {
      running = true;
      document.removeEventListener('readystateChange', maybeStartRunning);
      
      
      window.setTimeout(runNextTask, 0);
    }
  }
  document.addEventListener('readystatechange', maybeStartRunning);
  maybeStartRunning();
})(this);
