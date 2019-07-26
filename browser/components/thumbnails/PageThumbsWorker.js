










"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let PageThumbsWorker = {
  handleMessage: function Worker_handleMessage(aEvent) {
    let msg = aEvent.data;
    let data = {result: null, data: null};

    switch (msg.type) {
      case "removeFile":
        data.result = this.removeFile(msg);
        break;
      case "expireFilesInDirectory":
        data.result = this.expireFilesInDirectory(msg);
        break;
      case "moveOrDeleteAllThumbnails":
        data.result = this.moveOrDeleteAllThumbnails(msg);
        break;
      default:
        data.result = false;
        data.detail = "message not understood";
        break;
    }

    self.postMessage(data);
  },

  removeFile: function Worker_removeFile(msg) {
    try {
      OS.File.remove(msg.path);
      return true;
    } catch (e) {
      return false;
    }
  },

  expireFilesInDirectory: function Worker_expireFilesInDirectory(msg) {
    let entries = this.getFileEntriesInDirectory(msg.path, msg.filesToKeep);
    let limit = Math.max(msg.minChunkSize, Math.round(entries.length / 2));

    for (let entry of entries) {
      this.removeFile(entry);

      
      if (--limit <= 0) {
        break;
      }
    }

    return true;
  },

  getFileEntriesInDirectory:
  function Worker_getFileEntriesInDirectory(aPath, aSkipFiles) {
    let skip = new Set(aSkipFiles);
    let iter = new OS.File.DirectoryIterator(aPath);

    return [entry
            for (entry in iter)
            if (!entry.isDir && !entry.isSymLink && !skip.has(entry.name))];
  },

  moveOrDeleteAllThumbnails:
  function Worker_moveOrDeleteAllThumbnails(msg) {
    if (!OS.File.exists(msg.from))
      return true;

    let iter = new OS.File.DirectoryIterator(msg.from);
    for (let entry in iter) {
      if (entry.isDir || entry.isSymLink) {
        continue;
      }

      let from = OS.Path.join(msg.from, entry.name);
      let to = OS.Path.join(msg.to, entry.name);

      try {
        OS.File.move(from, to, {noOverwrite: true, noCopy: true});
      } catch (e) {
        OS.File.remove(from);
      }
    }
    iter.close();

    try {
      OS.File.removeEmptyDir(msg.from);
    } catch (e) {
      
      
    }

    return true;
  }
};

self.onmessage = PageThumbsWorker.handleMessage.bind(PageThumbsWorker);
