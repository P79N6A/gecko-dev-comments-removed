



"use strict"

this.EXPORTED_SYMBOLS = ["PromiseUtils"];

Components.utils.import("resource://gre/modules/Timer.jsm");

this.PromiseUtils = {
  





  defer : function() {
    return new Deferred();
  },
}





function Deferred() {
  






  this.resolve = null;

  






  this.reject = null;

  


  this.promise = new Promise((resolve, reject) => {
    this.resolve = resolve;
    this.reject = reject;
  });
}