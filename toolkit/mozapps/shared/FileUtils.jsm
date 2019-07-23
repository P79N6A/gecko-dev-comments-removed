





































EXPORTED_SYMBOLS = [ "FileUtils" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

XPCOMUtils.defineLazyServiceGetter(this, "gDirService",
                                   "@mozilla.org/file/directory_service;1",
                                   "nsIProperties");

var FileUtils = {
  MODE_RDONLY   : 0x01,
  MODE_WRONLY   : 0x02,
  MODE_CREATE   : 0x08,
  MODE_APPEND   : 0x10,
  MODE_TRUNCATE : 0x20,

  PERMS_FILE      : 0644,
  PERMS_DIRECTORY : 0755,

  











  getFile: function FileUtils_getFile(key, pathArray, followLinks) {
    var file = this.getDir(key, pathArray.slice(0, -1), true, followLinks);
    file.append(pathArray[pathArray.length - 1]);
    return file;
  },

  
















  getDir: function FileUtils_getDir(key, pathArray, shouldCreate, followLinks) {
    var dir = gDirService.get(key, Ci.nsILocalFile);
    for (var i = 0; i < pathArray.length; ++i) {
      dir.append(pathArray[i]);
      if (shouldCreate && !dir.exists())
        dir.create(Ci.nsILocalFile.DIRECTORY_TYPE, this.PERMS_DIRECTORY);
    }
    if (!followLinks)
      dir.followLinks = false;
    return dir;
  },

  







  openSafeFileOutputStream: function FileUtils_openSafeFileOutputStream(file, modeFlags) {
    var fos = Cc["@mozilla.org/network/safe-file-output-stream;1"].
              createInstance(Ci.nsIFileOutputStream);
    if (modeFlags === undefined)
      modeFlags = this.MODE_WRONLY | this.MODE_CREATE | this.MODE_TRUNCATE;
    fos.init(file, modeFlags, this.PERMS_FILE, 0);
    return fos;
  },

  




  closeSafeFileOutputStream: function FileUtils_closeSafeFileOutputStream(stream) {
    if (stream instanceof Ci.nsISafeOutputStream) {
      try {
        stream.finish();
        return;
      }
      catch (e) {
      }
    }
    stream.close();
  }
};
