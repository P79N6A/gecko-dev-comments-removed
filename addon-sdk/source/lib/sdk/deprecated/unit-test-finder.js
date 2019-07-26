



"use strict";

module.metadata = {
  "stability": "deprecated"
};

const file = require("../io/file");
const memory = require('./memory');
const suites = require('@test/options').allTestModules;
const { Loader } = require("sdk/test/loader");
const cuddlefish = require("sdk/loader/cuddlefish");

const NOT_TESTS = ['setup', 'teardown'];

var TestFinder = exports.TestFinder = function TestFinder(options) {
  memory.track(this);
  this.filter = options.filter;
  this.testInProcess = options.testInProcess === false ? false : true;
  this.testOutOfProcess = options.testOutOfProcess === true ? true : false;
};

TestFinder.prototype = {
  findTests: function findTests(cb) {
    var self = this;
    var tests = [];
    var filter;
    
    
    
    if (this.filter) {
      var colonPos = this.filter.indexOf(':');
      var filterFileRegex, filterNameRegex;
      if (colonPos === -1) {
        filterFileRegex = new RegExp(self.filter);
      } else {
        filterFileRegex = new RegExp(self.filter.substr(0, colonPos));
        filterNameRegex = new RegExp(self.filter.substr(colonPos + 1));
      }
      
      
      
      filter = function(filename, testname) {
        return filterFileRegex.test(filename) &&
               ((testname && filterNameRegex) ? filterNameRegex.test(testname)
                                              : true);
      };
    } else
      filter = function() {return true};

    suites.forEach(
      function(suite) {
        
        
        var loader = Loader(module);
        var module = cuddlefish.main(loader, suite);

        if (self.testInProcess)
          for each (let name in Object.keys(module).sort()) {
            if(NOT_TESTS.indexOf(name) === -1 && filter(suite, name)) {
              tests.push({
                           setup: module.setup,
                           teardown: module.teardown,
                           testFunction: module[name],
                           name: suite + "." + name
                         });
            }
          }
      });

    cb(tests);
  }
};
