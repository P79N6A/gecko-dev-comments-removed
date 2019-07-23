





































function run_test()
{
  
  
  
  
  
  var invalidArchives = ["emptyfile.txt", "smallfile.txt", "test.png"];

  invalidArchives.forEach(function(invalidArchive) {
    
    var invalidFile = do_get_file(DATA_DIR + invalidArchive);

    
    try {
      zipW.open(invalidFile, PR_RDWR);
      do_throw("Should have thrown NS_ERROR_FILE_CORRUPTED on " +
               invalidArchive + " !");
    } catch (e if (e instanceof Ci.nsIException &&
                   e.result == Components.results.NS_ERROR_FILE_CORRUPTED)) {
      
    }
  });
}
