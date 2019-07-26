








"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [
  "getManager",
  "sleep",
  "TestingCrashManager",
];

Cu.import("resource://gre/modules/CrashManager.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);

this.sleep = function (wait) {
  let deferred = Promise.defer();

  setTimeout(() => {
    deferred.resolve();
  }, wait);

  return deferred.promise;
};

this.TestingCrashManager = function (options) {
  CrashManager.call(this, options);
}

this.TestingCrashManager.prototype = {
  __proto__: CrashManager.prototype,

  createDummyDump: function (submitted=false, date=new Date(), hr=false) {
    let uuid = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator)
                .generateUUID()
                .toString();
    uuid = uuid.substring(1, uuid.length - 1);

    let path;
    let mode;
    if (submitted) {
      if (hr) {
        path = OS.Path.join(this._submittedDumpsDir, "bp-hr-" + uuid + ".txt");
      } else {
        path = OS.Path.join(this._submittedDumpsDir, "bp-" + uuid + ".txt");
      }
      mode = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR |
            OS.Constants.libc.S_IRGRP | OS.Constants.libc.S_IROTH;
    } else {
      path = OS.Path.join(this._pendingDumpsDir, uuid + ".dmp");
      mode = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR;
    }

    return Task.spawn(function* () {
      let f = yield OS.File.open(path, {create: true}, {unixMode: mode});
      yield f.setDates(date, date);
      yield f.close();
      dump("Created fake crash: " + path + "\n");

      return uuid;
    });
  },

  createIgnoredDumpFile: function (filename, submitted=false) {
    let path;
    if (submitted) {
      path = OS.Path.join(this._submittedDumpsDir, filename);
    } else {
      path = OS.Path.join(this._pendingDumpsDir, filename);
    }

    return Task.spawn(function* () {
      let mode = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR;
      yield OS.File.open(path, {create: true}, {unixMode: mode});
      dump ("Create ignored dump file: " + path + "\n");
    });
  },

  createEventsFile: function (filename, name, content, index=0, date=new Date()) {
    let path = OS.Path.join(this._eventsDirs[index], filename);

    let data = name + "\n" + content;
    let encoder = new TextEncoder();
    let array = encoder.encode(data);

    return Task.spawn(function* () {
      yield OS.File.writeAtomic(path, array);
      yield OS.File.setDates(path, date, date);
    });
  },

  




  _handleEventFilePayload: function (entry, type, payload) {
    if (type == "test.1") {
      if (payload == "malformed") {
        return this.EVENT_FILE_ERROR_MALFORMED;
      } else if (payload == "success") {
        return this.EVENT_FILE_SUCCESS;
      } else {
        
        this._store._data.crashes.set(payload, {id: payload, crashDate: entry.date});

        return this.EVENT_FILE_SUCCESS;
      }
    }

    return CrashManager.prototype._handleEventFilePayload.call(this, type,
                                                               payload);
  },
};

let DUMMY_DIR_COUNT = 0;

this.getManager = function () {
  return Task.spawn(function* () {
    const dirMode = OS.Constants.libc.S_IRWXU;
    let baseFile = OS.Constants.Path.profileDir;

    function makeDir() {
      return Task.spawn(function* () {
        let path = OS.Path.join(baseFile, "dummy-dir-" + DUMMY_DIR_COUNT++);
        dump("Creating directory: " + path + "\n");
        yield OS.File.makeDir(path, {unixMode: dirMode});

        return path;
      });
    }

    let pendingD = yield makeDir();
    let submittedD = yield makeDir();
    let eventsD1 = yield makeDir();
    let eventsD2 = yield makeDir();
    let storeD = yield makeDir();

    let m = new TestingCrashManager({
      pendingDumpsDir: pendingD,
      submittedDumpsDir: submittedD,
      eventsDirs: [eventsD1, eventsD2],
      storeDir: storeD,
    });

    return m;
  });
};
