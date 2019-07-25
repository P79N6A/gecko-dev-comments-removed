










"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let PageThumbsWorker = {
  handleMessage: function Worker_handleMessage(aEvent) {
    let msg = aEvent.data;
    let data = {result: null, data: null};

    switch (msg.type) {
      case "removeFiles":
        data.result = this.removeFiles(msg);
        break;
      case "getFilesInDirectory":
        data.result = this.getFilesInDirectory(msg);
        break;
      default:
        data.result = false;
        data.detail = "message not understood";
        break;
    }

    self.postMessage(data);
  },

  getFilesInDirectory: function Worker_getFilesInDirectory(msg) {
    let iter = new OS.File.DirectoryIterator(msg.path);
    let entries = [];

    for (let entry in iter) {
      if (!entry.isDir && !entry.isSymLink) {
        entries.push(entry.name);
      }
    }

    iter.close();
    return entries;
  },

  removeFiles: function Worker_removeFiles(msg) {
    for (let file of msg.paths) {
      try {
        OS.File.remove(file);
      } catch (e) {
        
        
      }
    }
    return true;
  }
};

self.onmessage = PageThumbsWorker.handleMessage.bind(PageThumbsWorker);
