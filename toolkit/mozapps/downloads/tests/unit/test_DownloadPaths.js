












const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/DownloadPaths.jsm");






function createTemporarySaveDirectory()
{
  var saveDir = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists()) {
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  }
  return saveDir;
}

function testSplitBaseNameAndExtension(aLeafName, [aBase, aExt])
{
  var [base, ext] = DownloadPaths.splitBaseNameAndExtension(aLeafName);
  do_check_eq(base, aBase);
  do_check_eq(ext, aExt);

  
  
  
  
  
  
  [base, ext] = DownloadPaths.splitBaseNameAndExtension("(" + base + ")" + ext);
  do_check_eq(base, "(" + aBase + ")");
  do_check_eq(ext, aExt);
}

function testCreateNiceUniqueFile(aTempFile, aExpectedLeafName)
{
  var createdFile = DownloadPaths.createNiceUniqueFile(aTempFile);
  do_check_eq(createdFile.leafName, aExpectedLeafName);
}

function run_test()
{
  
  testSplitBaseNameAndExtension("base",             ["base", ""]);
  testSplitBaseNameAndExtension("base.ext",         ["base", ".ext"]);
  testSplitBaseNameAndExtension("base.application", ["base", ".application"]);
  testSplitBaseNameAndExtension("base.x.Z",         ["base", ".x.Z"]);
  testSplitBaseNameAndExtension("base.ext.Z",       ["base", ".ext.Z"]);
  testSplitBaseNameAndExtension("base.ext.gz",      ["base", ".ext.gz"]);
  testSplitBaseNameAndExtension("base.ext.Bz2",     ["base", ".ext.Bz2"]);
  testSplitBaseNameAndExtension("base..ext",        ["base.", ".ext"]);
  testSplitBaseNameAndExtension("base..Z",          ["base.", ".Z"]);
  testSplitBaseNameAndExtension("base. .Z",         ["base. ", ".Z"]);
  testSplitBaseNameAndExtension("base.base.Bz2",    ["base.base", ".Bz2"]);
  testSplitBaseNameAndExtension("base  .ext",       ["base  ", ".ext"]);

  
  
  
  
  
  
  testSplitBaseNameAndExtension("base.",            ["base", "."]);
  testSplitBaseNameAndExtension(".ext",             ["", ".ext"]);

  
  testSplitBaseNameAndExtension("base. ",           ["base", ". "]);
  testSplitBaseNameAndExtension("base ",            ["base ", ""]);
  testSplitBaseNameAndExtension("",                 ["", ""]);
  testSplitBaseNameAndExtension(" ",                [" ", ""]);
  testSplitBaseNameAndExtension(" . ",              [" ", ". "]);
  testSplitBaseNameAndExtension(" .. ",             [" .", ". "]);
  testSplitBaseNameAndExtension(" .ext",            [" ", ".ext"]);
  testSplitBaseNameAndExtension(" .ext. ",          [" .ext", ". "]);
  testSplitBaseNameAndExtension(" .ext.gz ",        [" .ext", ".gz "]);

  var destDir = createTemporarySaveDirectory();
  try {
    
    var tempFile = destDir.clone();
    tempFile.append("test.txt");
    testCreateNiceUniqueFile(tempFile, "test.txt");
    testCreateNiceUniqueFile(tempFile, "test(1).txt");
    testCreateNiceUniqueFile(tempFile, "test(2).txt");

    
    tempFile.leafName = "test.tar.gz";
    testCreateNiceUniqueFile(tempFile, "test.tar.gz");
    testCreateNiceUniqueFile(tempFile, "test(1).tar.gz");
    testCreateNiceUniqueFile(tempFile, "test(2).tar.gz");

    
    
    
    tempFile.leafName = new Array(256).join("T") + ".txt";
    var newFile = DownloadPaths.createNiceUniqueFile(tempFile);
    do_check_true(newFile.leafName.length < tempFile.leafName.length);
    do_check_eq(newFile.leafName.slice(-4), ".txt");

    
    tempFile.append("file-under-long-directory.txt");
    try {
      DownloadPaths.createNiceUniqueFile(tempFile);
      do_throw("Exception expected with a long parent directory name.")
    } catch (e) {
      
    }
  } finally {
    
    destDir.remove(true);
  }
}
