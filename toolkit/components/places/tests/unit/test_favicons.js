



const TESTDIR = "toolkit/components/places/tests/unit/";







function dumpToFile(aData) {
    const path = "/tmp";

    var outputFile = Cc["@mozilla.org/file/local;1"].
                     createInstance(Ci.nsILocalFile);
    outputFile.initWithPath(path);
    outputFile.append("testdump.png");

    var outputStream = Cc["@mozilla.org/network/file-output-stream;1"].
                       createInstance(Ci.nsIFileOutputStream);
    
    outputStream.init(outputFile, 0x02 | 0x08 | 0x20, 0644, null);

    var bos = Cc["@mozilla.org/binaryoutputstream;1"].
              createInstance(Ci.nsIBinaryOutputStream);
    bos.setOutputStream(outputStream);

    bos.writeByteArray(aData, aData.length);

    outputStream.close();
}






function readFileData(aFile) {
    var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    
    inputStream.init(aFile, 0x01, -1, null);
    var size = inputStream.available();

    
    var bis = Cc["@mozilla.org/binaryinputstream;1"].
              createInstance(Ci.nsIBinaryInputStream);
    bis.setInputStream(inputStream);

    var bytes = bis.readByteArray(size);

    if (size != bytes.length)
        throw "Didn't read expected number of bytes";

    return bytes;
}









function setAndGetFaviconData(aFilename, aData, aMimeType) {
    var iconURI = uri("http://places.test/" + aFilename);

    iconsvc.setFaviconData(iconURI,
                           aData, aData.length, aMimeType,
                           Number.MAX_VALUE);

    var mimeTypeOutparam = {};

    var outData = iconsvc.getFaviconData(iconURI,
                           mimeTypeOutparam, {});

    return [outData, mimeTypeOutparam.value];
}







function compareArrays(aArray1, aArray2) {
    do_check_eq(aArray1.length, aArray2.length);

    for (var i = 0; i < aArray1.length; i++)
        if (aArray1[i] != aArray2[i])
            throw "arrays differ at index " + i;
}


var iconsvc;

function run_test() {
try {



var testnum = 0;
var testdesc = "nsIFaviconService setup";

iconsvc = Cc["@mozilla.org/browser/favicon-service;1"].
          getService(Ci.nsIFaviconService);

if (!iconsvc)
    throw "Couldn't get nsIFaviconService service"



testnum++;
testdesc = "test storing a normal 16x16 icon";


var iconName = "favicon-normal16.png";
var inMimeType = "image/png";
var iconFile = do_get_file(TESTDIR + iconName);

var inData = readFileData(iconFile);
do_check_eq(inData.length, 286);

var [outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


do_check_eq(inMimeType, outMimeType);
compareArrays(inData, outData);
                    


testnum++;
testdesc = "test storing a normal 32x32 icon";


iconName = "favicon-normal32.png";
inMimeType = "image/png";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 344);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


do_check_eq(inMimeType, outMimeType);
compareArrays(inData, outData);



testnum++;
testdesc = "test storing an oversize 16x16 icon ";



iconName = "favicon-big16.ico";
inMimeType = "image/x-icon";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 1406);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test storing an oversize 4x4 icon ";



iconName = "favicon-big4.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 4751);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);



testnum++;
testdesc = "test storing an oversize 32x32 icon ";



iconName = "favicon-big32.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 3494);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test storing an oversize 48x48 icon ";





iconName = "favicon-big48.ico";
inMimeType = "image/x-icon";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 56646);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test storing an oversize 64x64 icon ";



iconName = "favicon-big64.png";
inMimeType = "image/png";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 10698);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test scaling an oversize 160x3 icon ";



iconName = "favicon-scale160x3.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 5095);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test scaling an oversize 3x160 icon ";



iconName = "favicon-scale3x160.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(TESTDIR + iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 5059);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file(TESTDIR + "expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);





} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
