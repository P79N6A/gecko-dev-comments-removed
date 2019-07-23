function run_test() {
  var testBundle = do_get_file("data/test_bug446708");

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
  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  AddToZip(zipW, "", bundle); 
  zipW.close();
}

