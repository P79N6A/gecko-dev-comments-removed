










"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let PromiseWorker = require("resource://gre/modules/workers/PromiseWorker.js");

let File = OS.File;
let Type = OS.Shared.Type;

let worker = new PromiseWorker.AbstractWorker();
worker.dispatch = function(method, args = []) {
  return Agent[method](...args);
};
worker.postMessage = function(message, ...transfers) {
  self.postMessage(message, ...transfers);
};
worker.close = function() {
  self.close();
};

self.addEventListener("message", msg => worker.handleMessage(msg));


let Agent = {
  
  
  isFileRecent: function Agent_isFileRecent(path, maxAge) {
    try {
      let stat = OS.File.stat(path);
      let maxDate = new Date();
      maxDate.setSeconds(maxDate.getSeconds() - maxAge);
      return stat.lastModificationDate > maxDate;
    } catch (ex if ex instanceof OS.File.Error) {
      
      return false;
    }
  },

  remove: function Agent_removeFile(path) {
    try {
      OS.File.remove(path);
      return true;
    } catch (e) {
      return false;
    }
  },

  expireFilesInDirectory:
  function Agent_expireFilesInDirectory(path, filesToKeep, minChunkSize) {
    let entries = this.getFileEntriesInDirectory(path, filesToKeep);
    let limit = Math.max(minChunkSize, Math.round(entries.length / 2));

    for (let entry of entries) {
      this.remove(entry.path);

      
      if (--limit <= 0) {
        break;
      }
    }

    return true;
  },

  getFileEntriesInDirectory:
  function Agent_getFileEntriesInDirectory(path, skipFiles) {
    let iter = new OS.File.DirectoryIterator(path);
    if (!iter.exists()) {
      return [];
    }

    let skip = new Set(skipFiles);

    return [entry
            for (entry in iter)
            if (!entry.isDir && !entry.isSymLink && !skip.has(entry.name))];
  },

  moveOrDeleteAllThumbnails:
  function Agent_moveOrDeleteAllThumbnails(pathFrom, pathTo) {
    OS.File.makeDir(pathTo, {ignoreExisting: true});
    if (pathFrom == pathTo) {
      return true;
    }
    let iter = new OS.File.DirectoryIterator(pathFrom);
    if (iter.exists()) {
      for (let entry in iter) {
        if (entry.isDir || entry.isSymLink) {
          continue;
        }


        let from = OS.Path.join(pathFrom, entry.name);
        let to = OS.Path.join(pathTo, entry.name);

        try {
          OS.File.move(from, to, {noOverwrite: true, noCopy: true});
        } catch (e) {
          OS.File.remove(from);
        }
      }
    }
    iter.close();

    try {
      OS.File.removeEmptyDir(pathFrom);
    } catch (e) {
      
      
    }

    return true;
  },

  writeAtomic: function Agent_writeAtomic(path, buffer, options) {
    return File.writeAtomic(path,
      buffer,
      options);
  },

  makeDir: function Agent_makeDir(path, options) {
    return File.makeDir(path, options);
  },

  copy: function Agent_copy(source, dest, options) {
    return File.copy(source, dest, options);
  },

  wipe: function Agent_wipe(path) {
    let iterator = new File.DirectoryIterator(path);
    try {
      for (let entry in iterator) {
        try {
          File.remove(entry.path);
        } catch (ex) {
          
          
          
          
          
          
          
        }
      }
    } finally {
      iterator.close();
    }
  },

  exists: function Agent_exists(path) {
    return File.exists(path);
  },
};

