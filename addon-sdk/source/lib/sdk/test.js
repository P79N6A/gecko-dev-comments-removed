



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cu } = require("chrome");
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { defer } = require("sdk/core/promise");
const BaseAssert = require("sdk/test/assert").Assert;
const { isFunction, isObject } = require("sdk/lang/type");

exports.Assert = BaseAssert;

function extend(target) {
  let descriptor = {}
  Array.slice(arguments, 1).forEach(function(source) {
    Object.getOwnPropertyNames(source).forEach(function onEach(name) {
      descriptor[name] = Object.getOwnPropertyDescriptor(source, name);
    });
  });
  return Object.create(target, descriptor);
}







function defineTestSuite(target, suite, prefix) {
  prefix = prefix || "";
  
  
  
  let Assert = suite.Assert || BaseAssert;
  
  
  Object.keys(suite).forEach(function(key) {
     
    if (key.indexOf("test") === 0) {
      let test = suite[key];

      
      
      if (isFunction(test)) {

        
        
        target[prefix + key] = function(options) {

          
          let assert = Assert(options);
          assert.end = () => options.done();

          
          
          if (test.isGenerator && test.isGenerator()) {
            options.waitUntilDone();
            Task.spawn(test.bind(null, assert)).
                then(null, assert.fail).
                then(assert.end);
          }

          
          
          
          else if (1 < test.length) {

            
            
            
            options.waitUntilDone();
            test(assert, function() {
              options.done();
            });
          }

          
          
          else {
            test(assert);
          }
        }
      }

      
      
      
      else if (isObject(test)) {
        
        
        test = extend(Object.prototype, test, {
          Assert: test.Assert || Assert
        });
        defineTestSuite(target, test, prefix + key + ".");
      }
    }
  });
}







exports.run = function run(exports) {

  
  
  let suite = {};
  Object.keys(exports).forEach(function(key) {
    suite[key] = exports[key];
    delete exports[key];
  });

  
  
  
  defineTestSuite(exports, suite);
};
