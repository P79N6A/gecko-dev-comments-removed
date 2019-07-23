



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








function checkExpectedError (aExpectedError, aActualError) {
  if (aExpectedError) {
      if (!aActualError)
          throw "Didn't throw as expected (" + aExpectedError + ")";

      if (!aExpectedError.test(aActualError))
          throw "Threw (" + aActualError + "), not (" + aExpectedError;

      
      dump("...that error was expected.\n\n");
  } else if (aActualError) {
      throw "Threw unexpected error: " + aActualError;
  }
}


function run_test() {

try {



var testnum = 0;
var testdesc = "imgITools setup";
var err = null;

var imgTools = Cc["@mozilla.org/image/tools;1"].
               getService(Ci.imgITools);

if (!imgTools)
    throw "Couldn't get imgITools service"






var isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);



testnum++;
testdesc = "test decoding a PNG";


var imgName = "image1.png";
var inMimeType = "image/png";
var imgFile = do_get_file(imgName);

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
var refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 1081);
var referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test encoding an unscaled JPEG";


istream = imgTools.encodeImage(container, "image/jpeg");
encodedBytes = streamToArray(istream);


refName = "image1png64x64.jpg";
refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 4493);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test decoding a JPEG";


imgName = "image2.jpg";
inMimeType = "image/jpeg";
imgFile = do_get_file(imgName);

istream = getFileInputStream(imgFile);
do_check_eq(istream.available(), 3494);

outParam = {};
imgTools.decodeImageData(istream, inMimeType, outParam);
container = outParam.value;



do_check_eq(container.width,  32);
do_check_eq(container.height, 32);



testnum++;
testdesc = "test encoding a scaled PNG";

if (!isWindows) {

istream = imgTools.encodeScaledImage(container, "image/png", 16, 16);

encodedBytes = streamToArray(istream);

refName = isWindows ? "image2jpg16x16-win.png" : "image2jpg16x16.png";
refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 948);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);
}



testnum++;
testdesc = "test encoding an unscaled PNG";

if (!isWindows) {

istream = imgTools.encodeImage(container, "image/png");
encodedBytes = streamToArray(istream);


refName = isWindows ? "image2jpg32x32-win.png" : "image2jpg32x32.png";
refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 3105);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);
}



testnum++;
testdesc = "test decoding a ICO";


imgName = "image3.ico";
inMimeType = "image/x-icon";
imgFile = do_get_file(imgName);

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
refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 2281);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);



testnum++;
testdesc = "test encoding an unscaled PNG";


istream = imgTools.encodeImage(container, "image/png");
encodedBytes = streamToArray(istream);


refName = "image3ico16x16.png";
refFile = do_get_file(refName);
istream = getFileInputStream(refFile);
do_check_eq(istream.available(), 330);
referenceBytes = streamToArray(istream);


compareArrays(encodedBytes, referenceBytes);





testnum = 363986;
testdesc = "test PNG and JPEG encoders' Read/ReadSegments methods";

var testData = 
    [{preImage: "image3.ico",
      preImageMimeType: "image/x-icon",
      refImage: "image3ico16x16.png",
      refImageMimeType: "image/png"},
     {preImage: "image1.png",
      preImageMimeType: "image/png",
      refImage: "image1png64x64.jpg",
      refImageMimeType: "image/jpeg"}];

for(var i=0; i<testData.length; ++i) {
    var dict = testData[i];

    var imgFile = do_get_file(dict["refImage"]);
    var istream = getFileInputStream(imgFile);
    var refBytes = streamToArray(istream);

    imgFile = do_get_file(dict["preImage"]);
    istream = getFileInputStream(imgFile);

    var outParam = { value: null };
    imgTools.decodeImageData(istream, dict["preImageMimeType"], outParam);
    var container = outParam.value;

    istream = imgTools.encodeImage(container, dict["refImageMimeType"]);

    var sstream = Cc["@mozilla.org/storagestream;1"].
	          createInstance(Ci.nsIStorageStream);
    sstream.init(4096, 4294967295, null);
    var ostream = sstream.getOutputStream(0);
    var bostream = Cc["@mozilla.org/network/buffered-output-stream;1"].
	           createInstance(Ci.nsIBufferedOutputStream);

    
    bostream.init(ostream, 8);

    bostream.writeFrom(istream, istream.available());
    bostream.flush(); bostream.close();

    var encBytes = streamToArray(sstream.newInputStream(0));

    compareArrays(refBytes, encBytes);
}



testnum = 413512;
testdesc = "test decoding bad favicon (bug 413512)";

imgName = "bug413512.ico";
inMimeType = "image/x-icon";
imgFile = do_get_file(imgName);

istream = getFileInputStream(imgFile);
do_check_eq(istream.available(), 17759);
var errsrc = "none";

try {
  outParam = { value: null };
  imgTools.decodeImageData(istream, inMimeType, outParam);
  container = outParam.value;

  
  
  try {
      istream = imgTools.encodeImage(container, "image/png");
  } catch (e) {
      err = e;
      errsrc = "encode";
  }
} catch (e) {
  err = e;
  errsrc = "decode";
}

do_check_eq(errsrc, "decode");
checkExpectedError(/NS_ERROR_FAILURE/, err);




} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
