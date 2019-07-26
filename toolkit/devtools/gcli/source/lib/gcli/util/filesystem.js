















'use strict';

var fs = require('fs');
var path = require('path');

var promise = require('./promise');
var util = require('./util');





exports.join = path.join.bind(path);
exports.dirname = path.dirname.bind(path);
exports.sep = path.sep;
exports.home = process.env.HOME;










exports.split = function(pathname) {
  pathname = path.normalize(pathname);
  var parts = pathname.split(exports.sep);
  return parts.filter(function(part) {
    return part !== '';
  });
};











exports.ls = function(pathname, matches) {
  var deferred = promise.defer();

  fs.readdir(pathname, function(err, files) {
    if (err) {
      deferred.reject(err);
    }
    else {
      if (matches) {
        files = files.filter(function(file) {
          return matches.test(file);
        });
      }

      var statsPromise = util.promiseEach(files, function(filename) {
        var filepath = path.join(pathname, filename);
        return exports.stat(filepath).then(function(stats) {
          stats.filename = filename;
          stats.pathname = filepath;
          return stats;
        });
      });

      statsPromise.then(deferred.resolve, deferred.reject);
    }
  });

  return deferred.promise;
};







exports.stat = function(pathname) {
  var deferred = promise.defer();

  fs.stat(pathname, function(err, stats) {
    if (err) {
      if (err.code === 'ENOENT') {
        deferred.resolve({
          exists: false,
          isDir: false,
          isFile: false
        });
      }
      else {
        deferred.reject(err);
      }
    }
    else {
      deferred.resolve({
        exists: true,
        isDir: stats.isDirectory(),
        isFile: stats.isFile()
      });
    }
  });

  return deferred.promise;
};





exports.describe = function(pathname) {
  return promise.resolve('');
};
