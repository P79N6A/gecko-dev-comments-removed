



function MarionetteLogObj() {
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
    return this.logs;
  },

  


  clearLogs: function ML_clearLogs() {
    this.logs = [];
  },
}
