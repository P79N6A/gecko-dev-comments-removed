


"use strict";

const { Cu, Ci, Cc, CC } = require("chrome");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

XPCOMUtils.defineLazyGetter(this, "dirService", function() {
  return Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
});

XPCOMUtils.defineLazyGetter(this, "ZipWriter", function() {
  return CC("@mozilla.org/zipwriter;1", "nsIZipWriter");
});

XPCOMUtils.defineLazyGetter(this, "LocalFile", function() {
  return new CC("@mozilla.org/file/local;1", "nsILocalFile", "initWithPath");
});

XPCOMUtils.defineLazyGetter(this, "getMostRecentBrowserWindow", function() {
  return require("sdk/window/utils").getMostRecentBrowserWindow;
});

const nsIFilePicker = Ci.nsIFilePicker;

const OPEN_FLAGS = {
  RDONLY: parseInt("0x01"),
  WRONLY: parseInt("0x02"),
  CREATE_FILE: parseInt("0x08"),
  APPEND: parseInt("0x10"),
  TRUNCATE: parseInt("0x20"),
  EXCL: parseInt("0x80")
};




var HarUtils = {
  



  getTargetFile: function(fileName, jsonp, compress) {
    let browser = getMostRecentBrowserWindow();

    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(browser, null, nsIFilePicker.modeSave);
    fp.appendFilter("HTTP Archive Files", "*.har; *.harp; *.json; *.jsonp; *.zip");
    fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText);
    fp.filterIndex = 1;

    fp.defaultString = this.getHarFileName(fileName, jsonp, compress);

    let rv = fp.show();
    if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
      return fp.file;
    }

    return null;
  },

  getHarFileName: function(defaultFileName, jsonp, compress) {
    let extension = jsonp ? ".harp" : ".har";

    
    
    var now = new Date();
    var name = now.toLocaleFormat(defaultFileName);
    name = name.replace(/\:/gm, "-", "");
    name = name.replace(/\//gm, "_", "");

    let fileName = name + extension;

    
    if (compress) {
      fileName += ".zip";
    }

    return fileName;
  },

  








  saveToFile: function(file, jsonString, compress) {
    let openFlags = OPEN_FLAGS.WRONLY | OPEN_FLAGS.CREATE_FILE |
      OPEN_FLAGS.TRUNCATE;

    try {
      let foStream = Cc["@mozilla.org/network/file-output-stream;1"]
        .createInstance(Ci.nsIFileOutputStream);

      let permFlags = parseInt("0666", 8);
      foStream.init(file, openFlags, permFlags, 0);

      let convertor = Cc["@mozilla.org/intl/converter-output-stream;1"]
        .createInstance(Ci.nsIConverterOutputStream);
      convertor.init(foStream, "UTF-8", 0, 0);

      
      let chunkLength = 1024 * 1024;
      for (let i=0; i<=jsonString.length; i++) {
        let data = jsonString.substr(i, chunkLength+1);
        if (data) {
          convertor.writeString(data);
        }

        i = i + chunkLength;
      }

      
      convertor.close();
    } catch (err) {
      Cu.reportError(err);
      return false;
    }

    
    if (!compress) {
      return true;
    }

    
    let originalFilePath = file.path;
    let originalFileName = file.leafName;

    try {
      
      file.moveTo(null, "temp" + (new Date()).getTime() + "temphar");

      
      let zipFile = Cc["@mozilla.org/file/local;1"].
        createInstance(Ci.nsILocalFile);
      zipFile.initWithPath(originalFilePath);

      
      let fileName = originalFileName;
      if (fileName.indexOf(".zip") == fileName.length - 4) {
        fileName = fileName.substr(0, fileName.indexOf(".zip"));
      }

      let zip = new ZipWriter();
      zip.open(zipFile, openFlags);
      zip.addEntryFile(fileName, Ci.nsIZipWriter.COMPRESSION_DEFAULT,
        file, false);
      zip.close();

      
      file.remove(true);
      return true;
    } catch (err) {
      Cu.reportError(err);

      
      file.moveTo(null, originalFileName);
    }

    return false;
  },

  getLocalDirectory: function(path) {
    let dir;

    if (!path) {
      dir = dirService.get("ProfD", Ci.nsILocalFile);
      dir.append("har");
      dir.append("logs");
    } else {
      dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dir.initWithPath(path);
    }

    return dir;
  },
}


exports.HarUtils = HarUtils;
