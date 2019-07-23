



const TESTDIR = "modules/libpr0n/test/unit/";
const Ci = Components.interfaces;
const Cc = Components.classes;








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







function getFileInputStream(aFile) {
    var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    
    inputStream.init(aFile, 0x01, -1, null);

    
    
    var bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
              createInstance(Ci.nsIBufferedInputStream);
    bis.init(inputStream, 1024);

    return bis;
}







function streamToArray(aStream) {
    var size = aStream.available();

    
    var bis = Cc["@mozilla.org/binaryinputstream;1"].
              createInstance(Ci.nsIBinaryInputStream);
    bis.setInputStream(aStream);

    var bytes = bis.readByteArray(size);
    if (size != bytes.length)
        throw "Didn't read expected number of bytes";

    return bytes;
}







function compareArrays(aArray1, aArray2) {
    do_check_eq(aArray1.length, aArray2.length);

    for (var i = 0; i < aArray1.length; i++)
        if (aArray1[i] != aArray2[i])
            throw "arrays differ at index " + i;
}


function run_test() {


return;

try {



var testnum = 0;
var testdesc = "imgITools setup";

var imgTools = Cc["@mozilla.org/image/tools;1"].
               getService(Ci.imgITools);

if (!imgTools)
    throw "Couldn't get imgITools service"



testnum++;
testdesc = "test decoding a PNG";


var imgName = "image1.png";
var inMimeType = "image/png";
var imgFile = do_get_file(TESTDIR + imgName);

var istream = getFileInputStream(imgFile);
do_check_eq(istream.available(), 10698);

var outParam = { value: null };
imgTools.decodeImageData(istream, inMimeType, outParam);
var container = outParam.value;



do_check_eq(container.width,  64);
do_check_eq(container.height, 64);



testnum++;
testdesc = "test encoding a scaled JPEG";


istream = imgTools.encodeScaledImage(container, "image/jpeg", 16, 16);

var encodedBytes = streamToArray(istream);

var refName = "image1png16x16.jpg";
var refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 733);
var referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test encoding an unscaled JPEG";


istream = imgTools.encodeImage(container, "image/jpeg");
encodedBytes = streamToArray(istream);


refName = "image1png64x64.jpg";
refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 1593);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test decoding a JPEG";


imgName = "image2.jpg";
inMimeType = "image/jpeg";
imgFile = do_get_file(TESTDIR + imgName);

istream = getFileInputStream(imgFile);
do_check_eq(istream.available(), 3494);

outParam = {};
imgTools.decodeImageData(istream, inMimeType, outParam);
container = outParam.value;



do_check_eq(container.width,  32);
do_check_eq(container.height, 32);



testnum++;
testdesc = "test encoding a scaled PNG";


istream = imgTools.encodeScaledImage(container, "image/png", 16, 16);

encodedBytes = streamToArray(istream);

refName = "image2jpg16x16.png";
refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 948);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test encoding an unscaled PNG";


istream = imgTools.encodeImage(container, "image/png");
encodedBytes = streamToArray(istream);


refName = "image2jpg32x32.png";
refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 3105);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test decoding a ICO";


imgName = "image3.ico";
inMimeType = "image/x-icon";
imgFile = do_get_file(TESTDIR + imgName);

istream = getFileInputStream(imgFile);
do_check_eq(istream.available(), 1406);

outParam = { value: null };
imgTools.decodeImageData(istream, inMimeType, outParam);
container = outParam.value;



do_check_eq(container.width,  16);
do_check_eq(container.height, 16);



testnum++;
testdesc = "test encoding a scaled PNG"; 


istream = imgTools.encodeScaledImage(container, "image/png", 32, 32);
encodedBytes = streamToArray(istream);


refName = "image3ico32x32.png";
refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 2281);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test encoding an unscaled PNG";


istream = imgTools.encodeImage(container, "image/png");
encodedBytes = streamToArray(istream);


refName = "image3ico16x16.png";
refFile = do_get_file(TESTDIR + refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 330);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
