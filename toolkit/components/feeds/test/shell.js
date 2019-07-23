





















































function trimString(s)
{
  return(s.replace(/^\s+/,'').replace(/\s+$/,''));
}

var tests = new Array();
const ioService = Components.classes['@mozilla.org/network/io-service;1'].getService(Components.interfaces.nsIIOService);



if (0 < arguments.length) {

  
  
  var topDir = Components.classes["@mozilla.org/file/local;1"]
    .createInstance(Components.interfaces.nsILocalFile);
  
  topDir.initWithPath(arguments[0]);
}
else {
  const nsIDirectoryServiceProvider
    = Components.interfaces.nsIDirectoryServiceProvider;
  const nsIDirectoryServiceProvider_CONTRACTID
    = "@mozilla.org/file/directory_service;1";

  var dirServiceProvider 
    = Components.classes[nsIDirectoryServiceProvider_CONTRACTID]
    .getService(nsIDirectoryServiceProvider);

  var persistent = new Object();

  var topDir = dirServiceProvider.getFile("CurWorkD", persistent);
}

var entries = topDir.directoryEntries;
var xmlDir;
while(entries.hasMoreElements()){
  xmlDir = entries.getNext();
  xmlDir.QueryInterface(Components.interfaces.nsILocalFile);
  if(xmlDir.leafName == "xml") 
    break;
  else
    xmlDir = null;
}


var testDir;
if(xmlDir){
  entries = xmlDir.directoryEntries;
  while(entries.hasMoreElements()){
    testDir = entries.getNext();
    testDir.QueryInterface(Components.interfaces.nsILocalFile);
    if(testDir.exists() && testDir.isDirectory()){
      var testfiles = testDir.directoryEntries;
      while(testfiles.hasMoreElements()){ 
        var test = testfiles.getNext();
        test.QueryInterface(Components.interfaces.nsILocalFile);
        if(test.exists() && test.isFile()){
          
          
          
          
          
          

          var istream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                                  .createInstance(Components.interfaces.nsIFileInputStream);
          
          try{
            
            istream.init(test, 0x01, 0444, 0);
            istream.QueryInterface(Components.interfaces.nsILineInputStream);
            var line = {}, hasmore, testcase = {}, expectIndex, descIndex;

            do {
              hasmore = istream.readLine(line);
              expectIndex = line.value.indexOf('Expect:');
              descIndex = line.value.indexOf('Description:');
              baseIndex = line.value.indexOf('Base:');
              if(descIndex > -1)
                testcase.desc = trimString(line.value.substring(line.value.indexOf(':')+1));
              if(expectIndex > -1)
                testcase.expect = trimString(line.value.substring(line.value.indexOf(':')+1));
              if(baseIndex > -1)
                testcase.base = ioService.newURI(trimString(line.value.substring(line.value.indexOf(':')+1)),null,null);
              if(testcase.expect && testcase.desc){
                
                testcase.path = xmlDir.leafName + '/' + testDir.leafName + '/' + test.leafName;
                testcase.file = test;
                tests.push(testcase);
                break; 
              }
              
            } while(hasmore);

          }catch(e){
            dump("FAILED! Error reading test case in file " + test.leafName  + " " + e + "\n"); 
          }finally{
            istream.close();
          }
        }
      }
    }
  }
}

load(topDir.path+'/test.js');
