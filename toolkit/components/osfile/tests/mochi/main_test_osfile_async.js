"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/commonjs/promise/core.js");



Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

let myok = ok;
let myis = is;
let myinfo = info;
let myisnot = isnot;

let isPromise = function ispromise(value) {
  return value != null && typeof value == "object" && "then" in value;
};









let always = function always(promise, fun) {
  let p2 = Promise.defer();
  let onsuccess = function(resolution) {
    fun();
    p2.resolve(resolution);
  };
  let onreject = function(rejection) {
    fun();
    p2.reject(rejection);
  };
  promise.then(onsuccess, onreject);
  return p2.promise;
};

let ensureSuccess = function ensureSuccess(promise, test) {
  let p2 = Promise.defer();
  promise.then(function onSuccess(x) {
    p2.resolve(x);
  }, function onFailure(err) {
    test.fail("Uncaught error " + err + "\n" + err.stack);
    p2.reject(err);
  });

  return p2.promise;
};

let maketest = function(prefix, test) {
  let utils = {
    ok: function ok(t, m) {
      myok(t, prefix + ": " + m);
    },
    is: function is(l, r, m) {
      myis(l, r, prefix + ": " + m);
    },
    isnot: function isnot(l, r, m) {
      myisnot(l, r, prefix + ": " + m);
    },
    info: function info(m) {
      myinfo(prefix + ": " + m);
    },
    fail: function fail(m) {
      utils.ok(false, m);
    },
    okpromise: function okpromise(t, m) {
      return t.then(
        function onSuccess() {
          util.ok(true, m);
        },
        function onFailure() {
          util.ok(false, m);
        }
      );
    }
  };
  return function runtest() {
    utils.info("Entering");
    try {
      let result = test.call(this, utils);
      if (!isPromise(result)) {
        throw new TypeError("The test did not return a promise");
      }
      utils.info("This was a promise");
      
      result = result.then(function test_complete() {
        utils.info("Complete");
      }, function catch_uncaught_errors(err) {
        utils.fail("Uncaught error " + err);
        if (err && typeof err == "object" && "stack" in err) {
          utils.fail("at " + err.stack);
        }
      });
      return result;
    } catch (x) {
      utils.fail("Error " + x + " at " + x.stack);
      return null;
    }
  };
};










let reference_fetch_file = function reference_fetch_file(path, test) {
  test.info("Fetching file " + path);
  let promise = Promise.defer();
  let file = new FileUtils.File(path);
  NetUtil.asyncFetch(file,
    function(stream, status) {
      if (!Components.isSuccessCode(status)) {
        promise.reject(status);
        return;
      }
      let result, reject;
      try {
        result = NetUtil.readInputStreamToString(stream, stream.available());
      } catch (x) {
        reject = x;
      }
      stream.close();
      if (reject) {
        promise.reject(reject);
      } else {
        promise.resolve(result);
      }
  });
  return promise.promise;
};











let reference_compare_files = function reference_compare_files(a, b, test) {
  test.info("Comparing files " + a + " and " + b);
  let promise = reference_fetch_file(a, test);

  let a_contents, b_contents;
  promise = promise.then(function got_a(contents) {
    a_contents = contents;
    return reference_fetch_file(b, test);
  });
  promise = promise.then(function got_b(contents) {
    b_contents = contents;
    is(a_contents, b_contents, "Contents of files " + a + " and " + b + " match");
  });
  return promise;
};

let test = maketest("Main",
  function main(test) {
    SimpleTest.waitForExplicitFinish();
    let tests = [test_constants, test_path, test_open, test_stat,
                 test_read_write, test_read_write_all,
                 test_position, test_copy,
                 test_iter, test_exists];
    let current = 0;
    let aux = function aux() {
      if (current >= tests.length) {
        info("Test is over");
        SimpleTest.finish();
        return null;
      }
      let test = tests[current++];
      let result = test();
      if (isPromise(result)) {
        
        return result.then(aux, aux);
      } else {
        return aux();
      }
    };
    return aux();
  }
);




let EXISTING_FILE = OS.Path.join("chrome", "toolkit", "components",
  "osfile", "tests", "mochi", "main_test_osfile_async.js");




let test_constants = maketest("constants",
  function constants(test) {
    test.isnot(OS.Constants, null, "OS.Constants exists");
    test.ok(OS.Constants.Win || OS.Constants.libc, "OS.Constants.Win exists or OS.Constants.Unix exists");
    test.isnot(OS.Constants.Path, null, "OS.Constants.Path exists");
    test.isnot(OS.Constants.Sys, null, "OS.Constants.Sys exists");
    return Promise.resolve(true);
});




let test_path = maketest("path",
  function path(test) {
    test.ok(OS.Path, "OS.Path exists");
    test.ok(OS.Constants.Path, "OS.Constants.Path exists");
    test.is(OS.Constants.Path.tmpDir, Services.dirsvc.get("TmpD", Components.interfaces.nsIFile).path, "OS.Constants.Path.tmpDir is correct");
    test.is(OS.Constants.Path.profileDir, Services.dirsvc.get("ProfD", Components.interfaces.nsIFile).path, "OS.Constants.Path.profileDir is correct");
    return Promise.resolve(true);
});







let test_open = maketest("open",
  function open(test) {
    let promise;

    
    
    promise = OS.File.open(OS.Path.join(".", "This file does not exist")).
      then(function onSuccess(fd) {
        test.ok(false, "File opening 1 succeeded (it should fail)" + fd);
      }, function onFailure(err) {
        test.ok(true, "File opening 1 failed " + err);
        test.ok(err instanceof OS.File.Error, "File opening 1 returned a file error");
        test.ok(err.becauseNoSuchFile, "File opening 1 informed that the file does not exist");
      });

    
    
    promise = promise.then(function open_with_wrong_args() {
      test.info("Attempting to open a file with wrong arguments");
      return OS.File.open(1, 2, 3);
    });
    promise = promise.then(
      function onSuccess(fd) {
        test.ok(false, "File opening 2 succeeded (it should fail)" + fd);
      }, function onFailure(err) {
        test.ok(true, "File opening 2 failed " + err);
        test.ok(!(err instanceof OS.File.Error), "File opening 2 returned something that is not a file error");
        test.ok(err.constructor.name == "TypeError", "File opening 2 returned a TypeError");
      }
    );

    
    promise = promise.then(function open_should_work() {
      test.info("Attempting to open a file correctly");
      return OS.File.open(EXISTING_FILE);
    });
    let openedFile;
    promise = promise.then(function open_has_worked(file) {
      test.ok(true, "File opened correctly");
      openedFile = file;
    });

    
    promise = promise.then(function close_1() {
      test.info("Attempting to close a file correctly");
      return openedFile.close();
    });

    
    promise = promise.then(function close_2() {
      test.info("Attempting to close a file again");
      return openedFile.close();
    });

    
    return promise;
});




let test_stat = maketest("stat",
  function stat(test) {
    let promise;
    let file;
    let stat;

    
    promise = OS.File.open(EXISTING_FILE);
    promise = promise.then(function open_has_worked(aFile) {
      file = aFile;
      test.info("Stating file");
      return file.stat();
    });

    promise = promise.then(
      function stat_has_worked(aStat) {
        test.ok(true, "stat has worked " + aStat);
        test.ok(aStat, "stat is not empty");
        stat = aStat;
      }
    );

    promise = always(promise,
      function close() {
        if (file) {
          file.close();
        }
      }
    );

    
    promise = promise.then(
      function stat_without_opening() {
        test.info("Stating a file without opening it");
        return OS.File.stat(EXISTING_FILE);
      }
    );

    
    promise = promise.then(
      function stat_has_worked_2(aStat) {
        test.ok(true, "stat 2 has worked " + aStat);
        test.ok(aStat, "stat 2 is not empty");
        for (let key in aStat) {
          test.is("" + stat[key], "" + aStat[key], "Stat field " + key + "is the same");
        }
      }
    );

    
    return promise;
});




let test_read_write = maketest("read_write",
  function read_write(test) {
    let promise;
    let array;
    let fileSource, fileDest;
    let pathSource;
    let pathDest = OS.Path.join(OS.Constants.Path.tmpDir,
       "osfile async test.tmp");

    

    promise = OS.File.getCurrentDirectory();
    promise = promise.then(
      function obtained_current_directory(path) {
        test.ok(path, "Obtained current directory");
        pathSource = OS.Path.join(path, EXISTING_FILE);
        return OS.File.open(pathSource);
      }
    );

    promise = promise.then(
      function input_file_opened(file) {
        test.info("Input file opened");
        fileSource = file;
        test.info("OS.Constants.Path is " + OS.Constants.Path.toSource());
        return OS.File.open(pathDest,
          { truncate: true, read: true, write: true} );
      });

    promise = promise.then (
      function output_file_opened(file) {
        test.info("Output file opened");
        fileDest = file;
        return fileSource.stat();
      }
    );

    let size;
    promise = promise.then(
      function input_stat_worked(stat) {
        test.info("Input stat worked");
        size = stat.size;
        array = new Uint8Array(size);
        test.info("Now calling readTo");
        return fileSource.readTo(array);
      }
    );

    promise = promise.then(
      function read_worked(length) {
        test.info("ReadTo worked");
        test.is(length, size, "ReadTo got all bytes");
        return fileDest.write(array);
      }
    );

    promise = promise.then(
      function write_worked(length) {
        test.info("Write worked");
        test.is(length, size, "Write wrote all bytes");
        return;
      }
    );

    
    promise = promise.then(
      function prepare_readall() {
        return fileSource.setPosition(0);
      }
    );
    promise = promise.then(
      function setposition_worked() {
        return fileSource.read();
      }
    );
    promise = promise.then(
      function readall_worked(result) {
        test.info("ReadAll worked");
        test.is(result.length, size, "ReadAll read all bytes");
        test.is(Array.prototype.join.call(result),
                Array.prototype.join.call(array),
                "ReadAll result is correct");
      }
    );


    

    promise = always(promise,
      function close_all() {
        return fileSource.close().then(fileDest.close);
      }
    );

    promise = promise.then(
      function files_closed() {
        test.info("Files are closed");
        return OS.File.stat(pathDest);
      }
    );

    promise = promise.then(
      function comparing_sizes(stat) {
        test.is(stat.size, size, "Both files have the same size");
      }
    );

    promise = promise.then(
      function compare_contents() {
        return reference_compare_files(pathSource, pathDest, test);
      }
    );
    return promise;
});

let test_read_write_all = maketest(
  "read_write_all",
  function read_write_all(test) {
    let pathSource;
    let pathDest = OS.Path.join(OS.Constants.Path.tmpDir,
       "osfile async test read writeAtomic.tmp");
    let tmpPath = pathDest + ".tmp";

    let options, optionsBackup;



    let promise = OS.File.getCurrentDirectory();
    promise = promise.then(
      function obtained_current_directory(path) {
        test.ok(path, "Obtained current directory");
        pathSource = OS.Path.join(path, EXISTING_FILE);
        return OS.File.read(pathSource);
      }
    );
    promise = ensureSuccess(promise, test);

    let contents;
    promise = promise.then(
      function read_complete(result) {
        test.ok(result, "Obtained contents");
        contents = result;
        options = {tmpPath: tmpPath};
        optionsBackup = {tmpPath: tmpPath};
        return OS.File.writeAtomic(pathDest, contents, options);
      }
    );
    promise = ensureSuccess(promise, test);



    promise = promise.then(
      function atomicWrite_complete(bytesWritten) {
        test.is(contents.byteLength, bytesWritten, "Wrote the correct number of bytes");
        test.is(Object.keys(options).length, Object.keys(optionsBackup).length,
                "The number of options was not changed");
        for (let k in options) {
          test.is(options[k], optionsBackup[k], "Option was not changed");
        }
        return reference_compare_files(pathSource, pathDest, test);
      }
    );
    promise = ensureSuccess(promise, test);



    promise = promise.then(
      function compare_complete() {
        test.info("Compare complete");
        test.ok(!(new FileUtils.File(tmpPath).exists()), "Temporary file was removed");
      }
    );
    promise = ensureSuccess(promise, test);



    promise = promise.then(
      function check_with_noOverwrite() {
        let view = new Uint8Array(contents.buffer, 10, 200);
        options = {tmpPath: tmpPath, noOverwrite: true};
        return OS.File.writeAtomic(pathDest, view, options);
      }
    );

    promise = promise.then(
      function onSuccess() {
        test.fail("With noOverwrite, writeAtomic should have refused to overwrite file");
      },
      function onFailure(err) {
        test.info("With noOverwrite, writeAtomic correctly failed");
        test.ok(err instanceof OS.File.Error, "writeAtomic correctly failed with a file error");
        test.ok(err.becauseExists, "writeAtomic file error confirmed that the file already exists");
        return reference_compare_files(pathSource, pathDest, test);
      }
    );

    promise = promise.then(
      function compare_complete() {
        test.info("With noOverwrite, writeAtomic correctly did not overwrite destination file");
        test.ok(!(new FileUtils.File(tmpPath).exists()), "Temporary file was removed");
      }
    );
    promise = ensureSuccess(promise, test);



    let START = 10;
    let LENGTH = 100;
    promise = promise.then(
      function() {
        let view = new Uint8Array(contents.buffer, START, LENGTH);
        return OS.File.writeAtomic(pathDest, view, {tmpPath: tmpPath});
      }
    );

    promise = promise.then(
      function partial_write_complete(bytesWritten) {
        test.is(bytesWritten, LENGTH, "Partial write wrote the correct number of bytes");
        return OS.File.read(pathDest);
      }
    );

    promise = promise.then(
      function read_partial_write_complete(array2) {
        let view1 = new Uint8Array(contents.buffer, START, LENGTH);
        test.is(view1.length, array2.length, "Re-read partial write with the correct number of bytes");
        for (let i = 0; i < LENGTH; ++i) {
          if (view1[i] != array2[i]) {
            test.is(view1[i], array2[i], "Offset " + i + " is correct");
          }
          test.ok(true, "Compared re-read of partial write");
        }
      }
    );
    promise = ensureSuccess(promise, test);




    promise = promise.then(
      function check_without_tmpPath() {
        return OS.File.writeAtomic(pathDest, contents, {});
      },
      function onFailure() {
        test.info("Resetting failure");
      }
    );

    promise = promise.then(
      function onSuccess() {
        test.fail("Without a tmpPath, writeAtomic should have failed");
      },
      function onFailure() {
        test.ok("Without a tmpPath, writeAtomic has failed as expected");
      }
    );

    return promise;
  }
);

let test_position = maketest(
  "position",
  function position(test){

    let promise = OS.File.open(EXISTING_FILE);

    let file;

    promise = promise.then(
      function input_file_opened(aFile) {
        file = aFile;
        return file.stat();
      }
    );

    let view;
    promise = promise.then(
      function obtained_stat(stat) {
        test.info("Obtained file length");
        view = new Uint8Array(stat.size);
        return file.readTo(view);
      });

    promise = promise.then(
      function input_file_read() {
        test.info("First batch of content read");
        return file.getPosition();
      }
    );

    let pos;
    let CHUNK_SIZE = 178;

    promise = promise.then(
      function obtained_position(aPos) {
        test.info("Obtained position");
        test.is(aPos, view.byteLength, "getPosition returned the end of the file");
        return file.setPosition(-CHUNK_SIZE, OS.File.POS_END);
      }
    );

    let view2;
    promise = promise.then(
      function changed_position(aPos) {
        test.info("Changed position");
        test.is(aPos, view.byteLength - CHUNK_SIZE, "setPosition returned the correct position");
        view2 = new Uint8Array(CHUNK_SIZE);
        return file.readTo(view2);
      }
    );

    promise = promise.then(
      function input_file_reread() {
        test.info("Read the end of the file");
        for (let i = 0; i < CHUNK_SIZE; ++i) {
          if (view2[i] != view[i + view.byteLength - CHUNK_SIZE]) {
            test.is(view2[i], view[i], "setPosition put us in the right position");
          }
        }
      }
    );

    promise = always(promise,
      function () {
        if (file) {
          file.close();
        }
      });

    return promise;
  });

let test_copy = maketest("copy",
  function copy(test) {
    let promise;

    let pathSource;
    let pathDest = OS.Path.join(OS.Constants.Path.tmpDir,
       "osfile async test 2.tmp");

    promise = OS.File.getCurrentDirectory();
    promise = promise.then(
      function obtained_current_directory(path) {
        test.ok(path, "Obtained current directory");
        pathSource = OS.Path.join(path, EXISTING_FILE);
        return OS.File.copy(pathSource, pathDest);
      }
    );

    promise = promise.then(
      function copy_complete() {
        test.info("Copy complete");
        return reference_compare_files(pathSource, pathDest, test);
      }
    );

    let pathDest2 = OS.Path.join(OS.Constants.Path.tmpDir,
       "osfile async test 3.tmp");

    promise = promise.then(
      function compare_complete_1() {
        test.info("First compare complete");
        return OS.File.move(pathDest, pathDest2);
      }
    );

    promise = promise.then(
      function move_complete() {
        test.info("Move complete");
        return reference_compare_files(pathSource, pathDest2, test);
      }
    );

    promise = promise.then(
      function compare_complete_2() {
        test.info("Second compare complete");
        return OS.File.open(pathDest);
      }
    );

    promise = promise.then(
      function open_should_not_have_succeeded(file) {
        test.fail("I should not have been able to open " + pathDest);
        file.close();
      },
      function open_did_not_succeed(reason) {
        test.ok(reason, "Could not open a file after it has been moved away " + reason);
        test.ok(reason instanceof OS.File.Error, "Error is an OS.File.Error");
        test.ok(reason.becauseNoSuchFile, "Error mentions that the file does not exist");
      }
    );

    return promise;
  });

let test_mkdir = maketest("mkdir",
  function mkdir(test) {
    const DIRNAME = "test_dir.tmp";

    
    let promise = OS.File.removeEmptyDir(DIRNAME, {ignoreAbsent: true});


    
    promise = promise.then(
      function() {
        return OS.File.removeEmptyDir(DIRNAME, {ignoreAbent: true});
    });

    promise = test.okpromise(promise, "Check that removing an absent directory with ignoreAbsent succeeds");

    
    promise = promise.then(
      function() {
        return OS.File.removeEmptyDir(DIRNAME);
      }
    );
    promise = promise.then(
      function shouldNotHaveSucceeded() {
        test.fail("Check that removing an absent directory without ignoreAbsent fails");
      },
      function(result) {
        test.ok(result.rejected instanceof OS.File.Error && result.rejected.becauseNoSuchFile, "Check that removing an absent directory without ignoreAbsent throws the right error");
      }
    );

    
    promise = promise.then(
      function() {
        return OS.File.makeDir(DIRNAME);
      }
    );
    test.okpromise(promise, "Creating a directory");
    promise = promise.then(
      function() {
        return OS.File.stat(DIRNAME);
      }
    );
    promise = promise.then(
      function(stat) {
        test.ok(stat.isDir, "I have effectively created a directory");
      }
    );

    
    promise = promise.then(
      function() {
        return OS.File.makeDir(DIRNAME);
      }
    );
    promise = promise.then(
      function shouldNotHaveSucceeded() {
        test.fail("Check that creating over an existing directory fails");
      },
      function(result) {
        test.ok(result.rejected instanceof OS.File.Error && result.rejected.becauseExists, "Check that creating over an existing directory throws the right error");
      }
    );

    
    promise = promise.then(
      function() {
        return OS.File.removeEmptyDir(DIRNAME);
      }
    );
    promise = okpromise(promise, "Removing empty directory suceeded");

    promise = promise.then(
      function() {
        return OS.File.stat(DIRNAME);
      }
    );
    promise = promise.then(
      function shouldNotHaveSucceeded() {
        test.fail("Check that directory was effectively removed");
      },
      function(error) {
        ok(error instanceof OS.File.Error && error.becauseNoSuchFile,
           "Directory was effectively removed");
      }
    );

    return promise;
  });

let test_iter = maketest("iter",
  function iter(test) {
    let path;
    let promise = OS.File.getCurrentDirectory();
    let temporary_file_name;
    let iterator;

    
    promise = promise.then(
      function obtained_current_directory(aPath) {
        test.info("Preparing iteration");
        path = aPath;
        iterator = new OS.File.DirectoryIterator(aPath);
        temporary_file_name = OS.Path.join(path, "empty-temporary-file.tmp");
        return OS.File.remove(temporary_file_name);
      }
    );

    
    promise = promise.then(null, function() {});

    promise = promise.then(
      function removed_temporary_file() {
        return iterator.nextBatch();
      }
    );

    let allfiles1;
    promise = promise.then(
      function obtained_allfiles1(aAllFiles) {
        test.info("Obtained all files through nextBatch");
        allfiles1 = aAllFiles;
        test.isnot(allfiles1.length, 0, "There is at least one file");
        test.isnot(allfiles1[0].path, null, "Files have a path");
        return iterator.close();
      });

    let allfiles2 = [];
    let i = 0;
    promise = promise.then(
      function closed_iterator() {
        test.info("Closed iterator");
        iterator = new OS.File.DirectoryIterator(path);
        return iterator.forEach(function(entry, index) {
          is(i++, index, "Getting the correct index");
          allfiles2.push(entry);
        });
      }
    );

    promise = promise.then(
      function obtained_allfiles2() {
        test.info("Obtained all files through forEach");
        is(allfiles1.length, allfiles2.length, "Both runs returned the same number of files");
        for (let i = 0; i < allfiles1.length; ++i) {
          if (allfiles1[i].path != allfiles2[i].path) {
            test.is(allfiles1[i].path, allfiles2[i].path, "Both runs return the same files");
            break;
          }
        }
      }
    );

    
    let BATCH_LENGTH = 10;
    promise = promise.then(
      function compared_allfiles() {
        test.info("Getting some files through nextBatch");
        iterator.close();
        iterator = new OS.File.DirectoryIterator(path);
        return iterator.nextBatch(BATCH_LENGTH);
      }
    );
    let somefiles1;
    promise = promise.then(
      function obtained_somefiles1(aFiles) {
        somefiles1 = aFiles;
        return iterator.nextBatch(BATCH_LENGTH);
      }
    );
    let somefiles2;
    promise = promise.then(
      function obtained_somefiles2(aFiles) {
        somefiles2 = aFiles;
        iterator.close();
        iterator = new OS.File.DirectoryIterator(path);
        return iterator.forEach(
          function cb(entry, index, iterator) {
            if (index < BATCH_LENGTH) {
              test.is(entry.path, somefiles1[index].path, "Both runs return the same files (part 1)");
            } else if (index < 2*BATCH_LENGTH) {
              test.is(entry.path, somefiles2[index - BATCH_LENGTH].path, "Both runs return the same files (part 2)");
            } else if (index == 2 * BATCH_LENGTH) {
              test.info("Attempting to stop asynchronous forEach");
              return iterator.close();
            } else {
              test.fail("Can we stop an asynchronous forEach? " + index);
            }
            return null;
          });
      }
    );

    
    promise = promise.then(
      function create_temporary_file() {
        return OS.File.open(temporary_file_name, { write: true } );
      }
    );
    promise = promise.then(
      function with_temporary_file(file) {
        file.close();
        iterator = new OS.File.DirectoryIterator(path);
        return iterator.nextBatch();
      }
    );
    promise = promise.then(
      function with_new_list(aFiles) {
        is(aFiles.length, allfiles1.length + 1, "The directory iterator has noticed the new file");
      }
    );

    promise = always(promise,
      function cleanup() {
        if (iterator) {
          iterator.close();
        }
      }
    );

    return promise;
});




let test_exists = maketest("exists",
  function exists(test) {
    let promise;

    promise = OS.File.exists(EXISTING_FILE);
    promise = promise.then(function exists_worked(aExists) {
      test.ok(aExists, "file exists");
      return OS.File.exists(EXISTING_FILE + ".tmp");
    });

    promise = promise.then(function exists_on_absent_file_worked(aExists) {
      test.ok(!aExists, "file does not exists");
    });

    
    return promise;
});
