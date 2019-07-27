


"use strict";




































function createTask (self, name, fn) {
  
  if (!self._tasks) {
    self._tasks = {};
  }

  
  if (!self.onmessage) {
    self.onmessage = createHandler(self);
  }

  
  self._tasks[name] = fn;
}

exports.createTask = createTask;







function createHandler (self) {
  return function (e) {
    let { id, task, data } = e.data;
    let taskFn = self._tasks[task];

    if (!taskFn) {
      self.postMessage({ id, error: `Task "${task}" not found in worker.` });
      return;
    }

    try {
      let results;
      handleResponse(taskFn(data));
    } catch (e) {
      handleError(e);
    }

    function handleResponse (response) {
      
      if (response && typeof response.then === "function") {
        response.then(val => self.postMessage({ id, response: val }), handleError);
      }
      
      else if (response instanceof Error) {
        handleError(response);
      }
      
      else {
        self.postMessage({ id, response });
      }
    }

    function handleError (e="Error") {
      self.postMessage({ id, error: e.message || e });
    }
  }
}
