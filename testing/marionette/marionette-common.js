





























this.createStackMessage = function createStackMessage(error, fnName, pythonFile,
  pythonLine, script) {
  let python_stack = fnName + " @" + pythonFile;
  if (pythonLine !== null) {
    python_stack += ", line " + pythonLine;
  }
  let trace, msg;
  if (typeof(error) == "object" && 'name' in error && 'stack' in error) {
    let stack = error.stack.split("\n");
    let line = stack[0].substr(stack[0].lastIndexOf(':') + 1);
    msg = error.name + ('message' in error ? ": " + error.message : "");
    trace = python_stack +
                "\ninline javascript, line " + line +
                "\nsrc: \"" + script.split("\n")[line] + "\"";
  }
  else {
    trace = python_stack;
    msg = error + "";
  }
  return [msg, trace];
}

this.MarionetteLogObj = function MarionetteLogObj() {
  this.logs = [];
}
MarionetteLogObj.prototype = {
  






  log: function ML_log(msg, level) {
    let lev = level ? level : "INFO";
    this.logs.push( [lev, msg, (new Date()).toString()]);
  },

  




  addLogs: function ML_addLogs(msgs) {
    for (let i = 0; i < msgs.length; i++) {
      this.logs.push(msgs[i]);
    }
  },
  
  


  getLogs: function ML_getLogs() {
    let logs = this.logs;
    this.clearLogs();
    return logs;
  },

  


  clearLogs: function ML_clearLogs() {
    this.logs = [];
  },
}
