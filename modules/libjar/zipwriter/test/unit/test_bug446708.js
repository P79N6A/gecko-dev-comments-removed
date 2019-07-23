function run_test() {
  var testBundle = do_get_file("modules/libjar/zipwriter/test/unit/data/test_bug446708");

  RecursivelyZipDirectory(testBundle);
}


function AddToZip(zipWriter, path, file)
{
  var currentPath = path + file.leafName;
  
  if (file.isDirectory()) {
    currentPath += "/";
  }
  
  
  zipWriter.addEntryFile(currentPath, Ci.nsIZipWriter.COMPRESSION_DEFAULT, file, false);
  
  
  if (file.isDirectory()) {
    var entries = file.QueryInterface(Components.interfaces.nsIFile).directoryEntries;
    while (entries.hasMoreElements()) {
      var entry = entries.getNext().QueryInterface(Components.interfaces.nsIFile);
      AddToZip(zipWriter, currentPath, entry);
    }
  }
  
  
}

function RecursivelyZipDirectory(bundle)
{
  
  var dirUtils = Components.classes["@mozilla.org/file/directory_service;1"]
      .createInstance(Components.interfaces.nsIProperties);
  
  
  var tempFile = dirUtils.get("TmpD", Components.interfaces.nsIFile).clone();

  
  tempFile.append(bundle.leafName + ".zip");
  tempFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 
                        0600);
  
  zipW.open(tempFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  
  AddToZip(zipW, "", bundle); 
  
  
  zipW.close();
}

