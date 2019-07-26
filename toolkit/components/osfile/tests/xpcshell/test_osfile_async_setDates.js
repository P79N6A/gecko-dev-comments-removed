"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");







function run_test() {
  do_test_pending();
  run_next_test();
}


add_task(function test_nonproto() {
  
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                              "test_osfile_async_setDates_nonproto.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  try {
    
    
    
    const accDate = 2000;
    const modDate = 4000;
    {
      yield OS.File.setDates(path, accDate, modDate);
      let stat = yield OS.File.stat(path);
      do_check_eq(accDate, stat.lastAccessDate.getTime());
      do_check_eq(modDate, stat.lastModificationDate.getTime());
    }

    
    
    {
      yield OS.File.setDates(path, accDate);
      let stat = yield OS.File.stat(path);
      do_check_eq(accDate, stat.lastAccessDate.getTime());
      do_check_neq(modDate, stat.lastModificationDate.getTime());
    }

    
    
    {
      yield OS.File.setDates(path);
      let stat = yield OS.File.stat(path);
      do_check_neq(accDate, stat.lastAccessDate.getTime());
      do_check_neq(modDate, stat.lastModificationDate.getTime());
    }

    
    {
      yield OS.File.setDates(path, new Date(accDate), new Date(modDate));
      let stat = yield OS.File.stat(path);
      do_check_eq(accDate, stat.lastAccessDate.getTime());
      do_check_eq(modDate, stat.lastModificationDate.getTime());
    }

    
    {
      for (let p of ["invalid", new Uint8Array(1), NaN]) {
        try {
          yield OS.File.setDates(path, p, modDate);
          do_throw("Invalid access date should have thrown for: " + p);
        } catch (ex) {
          let stat = yield OS.File.stat(path);
          do_check_eq(accDate, stat.lastAccessDate.getTime());
          do_check_eq(modDate, stat.lastModificationDate.getTime());
        }
        try {
          yield OS.File.setDates(path, accDate, p);
          do_throw("Invalid modification date should have thrown for: " + p);
        } catch (ex) {
          let stat = yield OS.File.stat(path);
          do_check_eq(accDate, stat.lastAccessDate.getTime());
          do_check_eq(modDate, stat.lastModificationDate.getTime());
        }
        try {
          yield OS.File.setDates(path, p, p);
          do_throw("Invalid dates should have thrown for: " + p);
        } catch (ex) {
          let stat = yield OS.File.stat(path);
          do_check_eq(accDate, stat.lastAccessDate.getTime());
          do_check_eq(modDate, stat.lastModificationDate.getTime());
        }
      }
    }
  } finally {
    
    yield OS.File.remove(path);
  }
});


add_task(function test_proto() {
  
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                              "test_osfile_async_setDates_proto.tmp");
  yield OS.File.writeAtomic(path, new Uint8Array(1));

  tryÂ {
    let fd = yield OS.File.open(path, {write: true});

    try {
      
      
      
      const accDate = 2000;
      const modDate = 4000;
      {
        yield fd.setDates(accDate, modDate);
        let stat = yield fd.stat();
        do_check_eq(accDate, stat.lastAccessDate.getTime());
        do_check_eq(modDate, stat.lastModificationDate.getTime());
      }

      
      
      {
        yield fd.setDates(accDate);
        let stat = yield fd.stat();
        do_check_eq(accDate, stat.lastAccessDate.getTime());
        do_check_neq(modDate, stat.lastModificationDate.getTime());
      }

      
      
      {
        yield fd.setDates();
        let stat = yield fd.stat();
        do_check_neq(accDate, stat.lastAccessDate.getTime());
        do_check_neq(modDate, stat.lastModificationDate.getTime());
      }

      
      {
        yield fd.setDates(new Date(accDate), new Date(modDate));
        let stat = yield fd.stat();
        do_check_eq(accDate, stat.lastAccessDate.getTime());
        do_check_eq(modDate, stat.lastModificationDate.getTime());
      }

      
      {
        for (let p of ["invalid", new Uint8Array(1), NaN]) {
          try {
            yield fd.setDates(p, modDate);
            do_throw("Invalid access date should have thrown for: " + p);
          } catch (ex) {
            let stat = yield fd.stat();
            do_check_eq(accDate, stat.lastAccessDate.getTime());
            do_check_eq(modDate, stat.lastModificationDate.getTime());
          }
          try {
            yield fd.setDates(accDate, p);
            do_throw("Invalid modification date should have thrown for: " + p);
          } catch (ex) {
            let stat = yield fd.stat();
            do_check_eq(accDate, stat.lastAccessDate.getTime());
            do_check_eq(modDate, stat.lastModificationDate.getTime());
          }
          try {
            yield fd.setDates(p, p);
            do_throw("Invalid dates should have thrown for: " + p);
          } catch (ex) {
            let stat = yield fd.stat();
            do_check_eq(accDate, stat.lastAccessDate.getTime());
            do_check_eq(modDate, stat.lastModificationDate.getTime());
          }
        }
      }
    } finally {
      yield fd.close();
    }
  } finally {
    
    yield OS.File.remove(path);
  }
});


add_task(function test_dirs() {
  let path = OS.Path.join(OS.Constants.Path.tmpDir,
                              "test_osfile_async_setDates_dir");
  yield OS.File.makeDir(path);

  try {
    
    
    
    const accDate = 2000;
    const modDate = 4000;
    {
      yield OS.File.setDates(path, accDate, modDate);
      let stat = yield OS.File.stat(path);
      do_check_eq(accDate, stat.lastAccessDate.getTime());
      do_check_eq(modDate, stat.lastModificationDate.getTime());
    }
  } finally {
    yield OS.File.removeEmptyDir(path);
  }
});

add_task(do_test_finished);
