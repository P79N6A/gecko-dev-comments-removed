












Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const BackgroundFileSaverOutputStream = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=outputstream",
      "nsIBackgroundFileSaver");

const StringInputStream = Components.Constructor(
      "@mozilla.org/io/string-input-stream;1",
      "nsIStringInputStream",
      "setData");

const TEST_FILE_NAME_1 = "test-backgroundfilesaver-1.txt";





function getTempFile(aLeafName) {
  let file = FileUtils.getFile("TmpD", [aLeafName]);
  do_register_cleanup(function GTF_cleanup() {
    if (file.exists()) {
      file.remove(false);
    }
  });
  return file;
}













function promiseSaverComplete(aSaver, aOnTargetChangeFn) {
  let deferred = Promise.defer();
  aSaver.observer = {
    onTargetChange: function BFSO_onSaveComplete(aSaver, aTarget)
    {
      if (aOnTargetChangeFn) {
        aOnTargetChangeFn(aTarget);
      }
    },
    onSaveComplete: function BFSO_onSaveComplete(aSaver, aStatus)
    {
      if (Components.isSuccessCode(aStatus)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception("Saver failed.", aStatus));
      }
    },
  };
  return deferred.promise;
}















function promiseCopyToSaver(aSourceString, aSaverOutputStream, aCloseWhenDone) {
  let deferred = Promise.defer();
  let inputStream = new StringInputStream(aSourceString, aSourceString.length);
  let copier = Cc["@mozilla.org/network/async-stream-copier;1"]
               .createInstance(Ci.nsIAsyncStreamCopier);
  copier.init(inputStream, aSaverOutputStream, null, false, true, 0x8000, true,
              aCloseWhenDone);
  copier.asyncCopy({
    onStartRequest: function () { },
    onStopRequest: function (aRequest, aContext, aStatusCode)
    {
      if (Components.isSuccessCode(aStatusCode)) {
        deferred.resolve();
      } else {
        deferred.reject(new Components.Exception(aResult));
      }
    },
  }, null);
  return deferred.promise;
}

let gStillRunning = true;




function run_test()
{
  run_next_test();
}

add_task(function test_setup()
{
  
  do_timeout(10 * 60 * 1000, function() {
    if (gStillRunning) {
      do_throw("Test timed out.");
    }
  })
});

function readFileToString(aFilename) {
  let f = do_get_file(aFilename);
  let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                 .createInstance(Ci.nsIFileInputStream);
  stream.init(f, -1, 0, 0);
  let buf = NetUtil.readInputStreamToString(stream, stream.available());
  return buf;
}

add_task(function test_signature()
{
  
  let destFile = getTempFile(TEST_FILE_NAME_1);

  let data = readFileToString("data/signed_win.exe");
  let saver = new BackgroundFileSaverOutputStream();
  let completionPromise = promiseSaverComplete(saver);

  try {
    let signatureInfo = saver.signatureInfo;
    do_throw("Can't get signature before saver is complete.");
  } catch (ex if ex.result == Cr.NS_ERROR_NOT_AVAILABLE) { }

  saver.enableSignatureInfo();
  saver.setTarget(destFile, false);
  yield promiseCopyToSaver(data, saver, true);

  saver.finish(Cr.NS_OK);
  yield completionPromise;

  
  do_check_eq(1, saver.signatureInfo.length);
  let certLists = saver.signatureInfo.enumerate();
  do_check_true(certLists.hasMoreElements());
  let certList = certLists.getNext().QueryInterface(Ci.nsIX509CertList);
  do_check_false(certLists.hasMoreElements());

  
  let certs = certList.getEnumerator();
  do_check_true(certs.hasMoreElements());
  let signer = certs.getNext().QueryInterface(Ci.nsIX509Cert);
  do_check_true(certs.hasMoreElements());
  let issuer = certs.getNext().QueryInterface(Ci.nsIX509Cert);
  do_check_true(certs.hasMoreElements());
  let root = certs.getNext().QueryInterface(Ci.nsIX509Cert);
  do_check_false(certs.hasMoreElements());

  
  let organization = "Microsoft Corporation";
  do_check_eq("Microsoft Corporation", signer.commonName);
  do_check_eq(organization, signer.organization);
  do_check_eq("Copyright (c) 2002 Microsoft Corp.", signer.organizationalUnit);

  do_check_eq("Microsoft Code Signing PCA", issuer.commonName);
  do_check_eq(organization, issuer.organization);
  do_check_eq("Copyright (c) 2000 Microsoft Corp.", issuer.organizationalUnit);

  do_check_eq("Microsoft Root Authority", root.commonName);
  do_check_false(root.organization);
  do_check_eq("Copyright (c) 1997 Microsoft Corp.", root.organizationalUnit);

  
  destFile.remove(false);
});

add_task(function test_teardown()
{
  gStillRunning = false;
});
