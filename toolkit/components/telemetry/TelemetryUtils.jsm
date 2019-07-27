



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryUtils"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

this.TelemetryUtils = {
  





  millisecondsToDays: function(aMsec) {
    return Math.floor(aMsec / MILLISECONDS_PER_DAY);
  },

  


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

  generateUUID: function() {
    let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
    
    return str.substring(1, str.length - 1);
  },

  





  getElapsedTimeInMonths: function(aStartDate, aEndDate) {
    return (aEndDate.getMonth() - aStartDate.getMonth())
           + 12 * (aEndDate.getFullYear() - aStartDate.getFullYear());
  },
};
