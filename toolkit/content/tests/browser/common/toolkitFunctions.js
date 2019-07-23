









































function createTemporarySaveDirectory() {
  var saveDir = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists())
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  return saveDir;
}











function callSaveWithMockObjects(aSaveFunction) {
  
  
  
  
  mockFilePickerRegisterer.register();
  try {
    mockTransferForContinuingRegisterer.register();
    try {
      aSaveFunction();
    }
    finally {
      mockTransferForContinuingRegisterer.unregister();
    }
  }
  finally {
    mockFilePickerRegisterer.unregister();
  }
}










function readShortFile(aFile) {
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
  inputStream.init(aFile, -1, 0, 0);
  try {
    var scrInputStream = Cc["@mozilla.org/scriptableinputstream;1"].
                         createInstance(Ci.nsIScriptableInputStream);
    scrInputStream.init(inputStream);
    try {
      
      return scrInputStream.read(1048576);
    }
    finally {
      
      
      scrInputStream.close();
    }
  }
  finally {
    
    
    inputStream.close();
  }
}
