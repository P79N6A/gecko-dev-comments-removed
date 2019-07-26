const {Ci, Cc, Cu, Cr} = require("chrome");
Cu.import("resource://gre/modules/osfile.jsm");
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm");
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const promise = require("sdk/core/promise");




const PR_USEC_PER_MSEC = 1000;
const PR_RDWR = 0x04;
const PR_CREATE_FILE = 0x08;
const PR_TRUNCATE = 0x20;

const CHUNK_SIZE = 10000;

const appTargets = new Map();

function addDirToZip(writer, dir, basePath) {
  let files = dir.directoryEntries;

  while (files.hasMoreElements()) {
    let file = files.getNext().QueryInterface(Ci.nsIFile);

    if (file.isHidden() ||
        file.isSymlink() ||
        file.isSpecial() ||
        file.equals(writer.file))
    {
      continue;
    }

    if (file.isDirectory()) {
      writer.addEntryDirectory(basePath + file.leafName + "/",
                               file.lastModifiedTime * PR_USEC_PER_MSEC,
                               true);
      addDirToZip(writer, file, basePath + file.leafName + "/");
    } else {
      writer.addEntryFile(basePath + file.leafName,
                          Ci.nsIZipWriter.COMPRESSION_DEFAULT,
                          file,
                          true);
    }
  }
}





function getResultTest(code) {
  let regexp =
    /^\[Exception... "(.*)"  nsresult: "0x[0-9a-fA-F]* \((.*)\)"  location: ".*"  data: .*\]$/;
  let ex = Cc["@mozilla.org/js/xpc/Exception;1"].
           createInstance(Ci.nsIXPCException);
  ex.initialize(null, code, null, null, null, null);
  let [, message, name] = regexp.exec(ex.toString());
  return { name: name, message: message };
}

function zipDirectory(zipFile, dirToArchive) {
  let deferred = promise.defer();
  let writer = Cc["@mozilla.org/zipwriter;1"].createInstance(Ci.nsIZipWriter);
  writer.open(zipFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  this.addDirToZip(writer, dirToArchive, "");

  writer.processQueue({
    onStartRequest: function onStartRequest(request, context) {},
    onStopRequest: (request, context, status) => {
      if (status == Cr.NS_OK) {
        writer.close();
        deferred.resolve(zipFile);
      }
      else {
        let { name, message } = getResultText(status);
        deferred.reject(name + ": " + message);
      }
    }
  }, null);

  return deferred.promise;
}

function uploadPackage(client, webappsActor, packageFile) {
  let deferred = promise.defer();

  let request = {
    to: webappsActor,
    type: "uploadPackage"
  };
  client.request(request, (res) => {
    openFile(res.actor);
  });

  function openFile(actor) {
    OS.File.open(packageFile.path)
      .then(function (file) {
        uploadChunk(actor, file);
      });
  }
  function uploadChunk(actor, file) {
    file.read(CHUNK_SIZE)
        .then(function (bytes) {
          
          
          let chunk = String.fromCharCode.apply(null, bytes);

          let request = {
            to: actor,
            type: "chunk",
            chunk: chunk
          };
          client.request(request, (res) => {
            if (bytes.length == CHUNK_SIZE) {
              uploadChunk(actor, file);
            } else {
              file.close().then(function () {
                endsUpload(actor);
              });
            }
          });
        });
  }
  function endsUpload(actor) {
    let request = {
      to: actor,
      type: "done"
    };
    client.request(request, (res) => {
      deferred.resolve(actor);
    });
  }
  return deferred.promise;
}

function removeServerTemporaryFile(client, fileActor) {
  let request = {
    to: fileActor,
    type: "remove"
  };
  client.request(request, function (aResponse) {
    console.error("Failed removing server side temporary package file", aResponse);
  });
}

function installPackaged(client, webappsActor, packagePath, appId) {
  let deferred = promise.defer();
  let file = FileUtils.File(packagePath);
  let packagePromise;
  if (file.isDirectory()) {
    let tmpZipFile = FileUtils.getDir("TmpD", [], true);
    tmpZipFile.append("application.zip");
    tmpZipFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));
    packagePromise = zipDirectory(tmpZipFile, file)
  } else {
    packagePromise = promise.resolve(file);
  }
  packagePromise.then((zipFile) => {
    uploadPackage(client, webappsActor, zipFile)
        .then((fileActor) => {
          let request = {
            to: webappsActor,
            type: "install",
            appId: appId,
            upload: fileActor
          };
          client.request(request, (res) => {
            
            
            
            if (res.error) {
              deferred.reject(res);
            }
            if ("error" in res)
              deferred.reject({error: res.error, message: res.message});
            else
              deferred.resolve({appId: res.appId});
          });
          
          
          if (zipFile != file)
            zipFile.remove(false);
          
          
          deferred.promise.then(removeServerTemporaryFile, removeServerTemporaryFile);
        });
  });
  return deferred.promise;
}
exports.installPackaged = installPackaged;

function installHosted(client, webappsActor, appId, metadata, manifest) {
  let deferred = promise.defer();
  let request = {
    to: webappsActor,
    type: "install",
    appId: appId,
    metadata: metadata,
    manifest: manifest
  };
  client.request(request, (res) => {
    if (res.error) {
      deferred.reject(res);
    }
    if ("error" in res)
      deferred.reject({error: res.error, message: res.message});
    else
      deferred.resolve({appId: res.appId});
  });
  return deferred.promise;
}
exports.installHosted = installHosted;

function getTargetForApp(client, webappsActor, manifestURL) {
  
  
  
  let existingTarget = appTargets.get(manifestURL);
  if (existingTarget)
    return promise.resolve(existingTarget);

  let deferred = promise.defer();
  let request = {
    to: webappsActor,
    type: "getAppActor",
    manifestURL: manifestURL,
  }
  client.request(request, (res) => {
    if (res.error) {
      deferred.reject(res.error);
    } else {
      let options = {
        form: res.actor,
        client: client,
        chrome: false
      };

      devtools.TargetFactory.forRemoteTab(options).then((target) => {
        target.isApp = true;
        appTargets.set(manifestURL, target);
        target.on("close", () => {
          appTargets.delete(manifestURL);
        });
        deferred.resolve(target)
      }, (error) => {
        deferred.reject(error);
      });
    }
  });
  return deferred.promise;
}
exports.getTargetForApp = getTargetForApp;

