



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryUtils"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

this.TelemetryUtils = {
  


  truncateToDays: function(date) {
    return new Date(date.getFullYear(),
                    date.getMonth(),
                    date.getDate(),
                    0, 0, 0, 0);
  },

  







  areTimesClose: function(t1, t2, tolerance) {
    return Math.abs(t1 - t2) <= tolerance;
  },

  




  getNextMidnight: function(date) {
    let nextMidnight = new Date(this.truncateToDays(date));
    nextMidnight.setDate(nextMidnight.getDate() + 1);
    return nextMidnight;
  },

  






  getNearestMidnight: function(date, tolerance) {
    let lastMidnight = this.truncateToDays(date);
    if (this.areTimesClose(date.getTime(), lastMidnight.getTime(), tolerance)) {
      return lastMidnight;
    }

    const nextMidnightDate = this.getNextMidnight(date);
    if (this.areTimesClose(date.getTime(), nextMidnightDate.getTime(), tolerance)) {
      return nextMidnightDate;
    }
    return null;
  },
};
