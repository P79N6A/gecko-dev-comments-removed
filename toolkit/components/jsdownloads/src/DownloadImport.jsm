





"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadImport",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");







const DOWNLOAD_NOTSTARTED = -1;
const DOWNLOAD_DOWNLOADING = 0;
const DOWNLOAD_PAUSED = 4;
const DOWNLOAD_QUEUED = 5;












this.DownloadImport = function(aList, aPath) {
  this.list = aList;
  this.path = aPath;
}

this.DownloadImport.prototype = {
  









  import: function () {
    return Task.spawn(function task_DI_import() {
      let connection = yield Sqlite.openConnection({ path: this.path });

      try {
        let schemaVersion = yield connection.getSchemaVersion();
        
        
        
        
        
        
        
        if (schemaVersion < 7) {
          throw new Error("Unable to import in-progress downloads because "
                          + "the existing profile is too old.");
        }

        let rows = yield connection.execute("SELECT * FROM moz_downloads");

        for (let row of rows) {
          try {
            
            let source = row.getResultByName("source");
            let target = row.getResultByName("target");
            let tempPath = row.getResultByName("tempPath");
            let startTime = row.getResultByName("startTime");
            let state = row.getResultByName("state");
            let referrer = row.getResultByName("referrer");
            let maxBytes = row.getResultByName("maxBytes");
            let mimeType = row.getResultByName("mimeType");
            let preferredApplication = row.getResultByName("preferredApplication");
            let preferredAction = row.getResultByName("preferredAction");
            let entityID = row.getResultByName("entityID");

            let autoResume = false;
            try {
              autoResume = row.getResultByName("autoResume");
            } catch (ex) {
              
            }

            if (!source) {
              throw new Error("Attempted to import a row with an empty " +
                              "source column.");
            }

            let resumeDownload = false;

            switch (state) {
              case DOWNLOAD_NOTSTARTED:
              case DOWNLOAD_QUEUED:
              case DOWNLOAD_DOWNLOADING:
                resumeDownload = true;
                break;

              case DOWNLOAD_PAUSED:
                resumeDownload = autoResume;
                break;

              default:
                
                continue;
            }

            
            let targetPath = NetUtil.newURI(target)
                                    .QueryInterface(Ci.nsIFileURL).file.path;

            let launchWhenSucceeded = (preferredAction != Ci.nsIMIMEInfo.saveToDisk);

            let downloadOptions = {
              source: {
                url: source,
                referrer: referrer
              },
              target: {
                path: targetPath,
                partFilePath: tempPath,
              },
              saver: {
                type: "copy",
                entityID: entityID
              },
              startTime: startTime,
              totalBytes: maxBytes,
              hasPartialData: !!tempPath,
              tryToKeepPartialData: true,
              launchWhenSucceeded: launchWhenSucceeded,
              contentType: mimeType,
              launcherPath: preferredApplication
            };

            
            
            if (!resumeDownload) {
              downloadOptions.canceled = true;
            }

            let download = yield Downloads.createDownload(downloadOptions);

            this.list.add(download);

            if (resumeDownload) {
              download.start();
            } else {
              yield download.refresh();
            }

          } catch (ex) {
            Cu.reportError("Error importing download: " + ex);
          }
        }

      } catch (ex) {
        Cu.reportError(ex);
      } finally {
        yield connection.close();
      }
    }.bind(this));
  }
}

