















'use strict';

var Cu = require('chrome').Cu;
var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;

var OS = Cu.import('resource://gre/modules/osfile.jsm', {}).OS;
var Promise = require('../util/promise').Promise;







exports.join = OS.Path.join;
exports.sep = OS.Path.sep;
exports.dirname = OS.Path.dirname;

var dirService = Cc['@mozilla.org/file/directory_service;1']
                           .getService(Ci.nsIProperties);
exports.home = dirService.get('Home', Ci.nsIFile).path;

if ('winGetDrive' in OS.Path) {
  exports.sep = '\\';
}
else {
  exports.sep = '/';
}






exports.split = function(pathname) {
  return OS.Path.split(pathname).components;
};











exports.ls = function(pathname, matches) {
  var iterator = new OS.File.DirectoryIterator(pathname);
  var entries = [];

  var iteratePromise = iterator.forEach(function(entry) {
    entries.push({
      exists: true,
      isDir: entry.isDir,
      isFile: !entry.isFile,
      filename: entry.name,
      pathname: entry.path
    });
  });

  return iteratePromise.then(function onSuccess() {
      iterator.close();
      return entries;
    },
    function onFailure(reason) {
      iterator.close();
      throw reason;
    }
  );
};







exports.stat = function(pathname) {
  var onResolve = function(stats) {
    return {
      exists: true,
      isDir: stats.isDir,
      isFile: !stats.isFile
    };
  };

  var onReject = function(err) {
    if (err instanceof OS.File.Error && err.becauseNoSuchFile) {
      return {
        exists: false,
        isDir: false,
        isFile: false
      };
    }
    throw err;
  };

  return OS.File.stat(pathname).then(onResolve, onReject);
};





exports.describe = function(pathname) {
  return Promise.resolve('');
};
