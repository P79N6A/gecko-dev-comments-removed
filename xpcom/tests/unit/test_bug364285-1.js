const Ci = Components.interfaces;
const Cc = Components.classes;

var nameArray = [
 "ascii",                                           
 "fran\u00E7ais",                                   
 "\u0420\u0443\u0441\u0441\u043A\u0438\u0439",      
 "\u65E5\u672C\u8A9E",                              
 "\u4E2D\u6587",                                    
 "\uD55C\uAD6D\uC5B4",                              
 "\uD801\uDC0F\uD801\uDC2D\uD801\uDC3B\uD801\uDC2B" 
];

function getTempDir()
{
    var dirService = Cc["@mozilla.org/file/directory_service;1"]
	.getService(Ci.nsIProperties);
    return dirService.get("TmpD", Ci.nsILocalFile);
}

function create_file(fileName)
{
    var outFile = getTempDir();
    outFile.append(fileName);
    outFile.createUnique(outFile.NORMAL_FILE_TYPE, 0600);

    var stream = Cc["@mozilla.org/network/file-output-stream;1"]
	.createInstance(Ci.nsIFileOutputStream);
    stream.init(outFile, 0x02 | 0x08 | 0x20, 0600, 0);
    stream.write("foo", 3);
    stream.close();

    do_check_eq(outFile.leafName.substr(0, fileName.length), fileName);

    return outFile;
}

function test_create(fileName)
{
    var file1 = create_file(fileName);
    var file2 = create_file(fileName);
    file1.remove(false);
    file2.remove(false);
}

function run_test()
{
    for (var i = 0; i < nameArray.length; ++i) {
	test_create(nameArray[i]);
    }
}
