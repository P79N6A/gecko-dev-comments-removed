










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadStore",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const LocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                         "nsIFile", "initWithPath");

XPCOMUtils.defineLazyGetter(this, "gTextDecoder", function () {
  return new TextDecoder();
});

XPCOMUtils.defineLazyGetter(this, "gTextEncoder", function () {
  return new TextEncoder();
});













function DownloadStore(aList, aPath)
{
  this.list = aList;
  this.path = aPath;
}

DownloadStore.prototype = {
  


  list: null,

  


  path: "",

  






  load: function DS_load()
  {
    return Task.spawn(function task_DS_load() {
      let bytes;
      try {
        bytes = yield OS.File.read(this.path);
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
        
        return;
      }

      let storeData = JSON.parse(gTextDecoder.decode(bytes));

      
      for (let downloadData of storeData) {
        try {
          let source = { uri: NetUtil.newURI(downloadData.source.uri) };
          if ("referrer" in downloadData.source) {
            source.referrer = NetUtil.newURI(downloadData.source.referrer);
          }
          let download = yield Downloads.createDownload({
            source: source,
            target: { file: new LocalFile(downloadData.target.file) },
            saver: downloadData.saver,
          });

          this.list.add(download);
        } catch (ex) {
          
          Cu.reportError(ex);
        }
      }
    }.bind(this));
  },

  








  save: function DS_save()
  {
    return Task.spawn(function task_DS_save() {
      let downloads = yield this.list.getAll();

      
      let storeData = [];
      let atLeastOneDownload = false;
      for (let download of downloads) {
        try {
          storeData.push({
            source: download.source.serialize(),
            target: download.target.serialize(),
            saver: download.saver.serialize(),
          });
          atLeastOneDownload = true;
        } catch (ex) {
          
          
          Cu.reportError(ex);
        }
      }

      if (atLeastOneDownload) {
        
        let bytes = gTextEncoder.encode(JSON.stringify(storeData));
        yield OS.File.writeAtomic(this.path, bytes,
                                  { tmpPath: this.path + ".tmp" });
      } else {
        
        try {
          yield OS.File.remove(this.path);
        } catch (ex if ex instanceof OS.File.Error &&
                 (ex.becauseNoSuchFile || ex.becauseAccessDenied)) {
          
          
        }
      }
    }.bind(this));
  },
};
