function create_subdir(dir, subdirname) {
  let subdir = dir.clone();
  subdir.append(subdirname);
  if (subdir.exists()) {
    subdir.remove(true);
  }
  subdir.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  return subdir;
}


let _provider = null;

function make_fake_appdir() {
  
  
  
  let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  let profD = dirSvc.get("ProfD", Components.interfaces.nsILocalFile);
  
  let appD = create_subdir(profD, "UAppData");

  let crashesDir = create_subdir(appD, "Crash Reports");
  create_subdir(crashesDir, "pending");
  create_subdir(crashesDir, "submitted");

  _provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "UAppData") {
        return appD.clone();
      }
      throw Components.results.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.nsIDirectoryProvider) ||
          iid.equals(Components.interfaces.nsISupports)) {
        return this;
      }
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };
  
  dirSvc.QueryInterface(Components.interfaces.nsIDirectoryService)
        .registerProvider(_provider);
  
  try {
    dirSvc.undefine("UAppData");
  } catch(ex) {} 
  return appD.clone();
}

function cleanup_fake_appdir() {
  let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  dirSvc.unregisterProvider(_provider);
  
  try {
    dirSvc.undefine("UAppData");
  } catch(ex) {
    dump("cleanup_fake_appdir: dirSvc.undefine failed: " + ex.message +"\n");
  }
}

function add_fake_crashes(crD, count) {
  let results = [];
  let uuidGenerator = Components.classes["@mozilla.org/uuid-generator;1"]
                        .getService(Components.interfaces.nsIUUIDGenerator);
  let submitdir = crD.clone();
  submitdir.append("submitted");
  
  
  let date = Date.now() - count * 60000;
  for (let i = 0; i < count; i++) {
    let uuid = uuidGenerator.generateUUID().toString();
    
    uuid = uuid.substring(1,uuid.length-2);
    let fn = "bp-" + uuid + ".txt";
    let file = submitdir.clone();
    file.append(fn);
    file.create(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0666);
    file.lastModifiedTime = date;
    results.push({'id': uuid, 'date': date, 'pending': false});

    date += 60000;
  }
  
  
  results.sort(function(a,b) b.date - a.date);
  return results;
}
