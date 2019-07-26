


function log(text) {
  dump("WORKER "+text+"\n");
}

function send(message) {
  self.postMessage(message);
}

function should_throw(f) {
  try {
    f();
  } catch (x) {
    return x;
  }
  return null;
}

self.onmessage = function onmessage_start(msg) {
  self.onmessage = function onmessage_ignored(msg) {
    log("ignored message " + JSON.stringify(msg.data));
  };
  try {
    test_init();
    test_offsetby();
    test_open_existing_file();
    test_open_non_existing_file();
    test_flush_open_file();
    test_copy_existing_file();
    test_readall_writeall_file();
    test_position();
    test_move_file();
    test_iter_dir();
    test_mkdir();
    test_info();
    test_path();
    test_exists_file();
  } catch (x) {
    log("Catching error: " + x);
    log("Stack: " + x.stack);
    log("Source: " + x.toSource());
    ok(false, x.toString() + "\n" + x.stack);
  }
  finish();
};

function finish() {
  send({kind: "finish"});
}

function ok(condition, description) {
  send({kind: "ok", condition: condition, description:description});
}
function is(a, b, description) {
  send({kind: "is", a: a, b:b, description:description});
}
function isnot(a, b, description) {
  send({kind: "isnot", a: a, b:b, description:description});
}

function test_init() {
  ok(true, "Starting test_init");
  importScripts("resource://gre/modules/osfile.jsm");
}

function test_offsetby() {
  ok(true, "Starting test_offsetby");

  
  let LENGTH = 1024;
  let buf = new ArrayBuffer(LENGTH);
  let view = new Uint8Array(buf);
  let i;
  for (i = 0; i < LENGTH; ++i) {
    view[i] = i;
  }

  
  let uint8 = OS.Shared.Type.uint8_t.in_ptr.implementation(buf);
  for (i = 0; i < LENGTH; ++i) {
    let value = OS.Shared.offsetBy(uint8, i).contents;
    if (value != i%256) {
      is(value, i % 256, "test_offsetby: Walking through array with offsetBy (8 bits)");
      break;
    }
  }

  
  let uint16 = OS.Shared.Type.uint16_t.in_ptr.implementation(buf);
  let view2 = new Uint16Array(buf);
  for (i = 0; i < LENGTH/2; ++i) {
    let value = OS.Shared.offsetBy(uint16, i).contents;
    if (value != view2[i]) {
      is(value, view2[i], "test_offsetby: Walking through array with offsetBy (16 bits)");
      break;
    }
  }

  
  let startptr = OS.Shared.offsetBy(uint8, 0);
  let startptr2 = OS.Shared.offsetBy(startptr, 0);
  is(startptr.toString(), startptr2.toString(), "test_offsetby: offsetBy(..., 0) is idmpotent");

  
  let ptr = ctypes.voidptr_t(0);
  let exn;
  try {
    OS.Shared.Utils.offsetBy(ptr, 1);
  } catch (x) {
    exn = x;
  }
  ok(!!exn, "test_offsetby: rejected offsetBy with void*");

  ok(true, "test_offsetby: complete");
}





function test_open_existing_file()
{
  ok(true, "Starting test_open_existing");
  let file = OS.File.open("chrome/toolkit/components/osfile/tests/mochi/worker_test_osfile_unix.js");
  file.close();
}




function test_open_non_existing_file()
{
  ok(true, "Starting test_open_non_existing");
  let exn;
  try {
    let file = OS.File.open("/I do not exist");
  } catch (x) {
    exn = x;
    ok(true, "test_open_non_existing_file: Exception detail " + exn);
  }
  ok(!!exn, "test_open_non_existing_file: Exception was raised ");
  ok(exn instanceof OS.File.Error, "test_open_non_existing_file: Exception was a OS.File.Error");
  ok(exn.becauseNoSuchFile, "test_open_non_existing_file: Exception confirms that the file does not exist");
}





function test_flush_open_file()
{
  ok(true, "Starting test_flush_open_file");
  let tmp = "test_flush.tmp";
  let file = OS.File.open(tmp, {create: true, write: true});
  file.flush();
  file.close();
  OS.File.remove(tmp);
}













function compare_files(test, sourcePath, destPath, prefix)
{
  ok(true, test + ": Comparing " + sourcePath + " and " + destPath);
  let source = OS.File.open(sourcePath);
  let dest = OS.File.open(destPath);
  ok(true, "Files are open");
  let sourceResult, destResult;
  try {
    if (prefix != undefined) {
      sourceResult = source.read(prefix);
      destResult = dest.read(prefix);
    } else {
      sourceResult = source.read();
      destResult = dest.read();
    }
    is(sourceResult.length, destResult.length, test + ": Both files have the same size");
    for (let i = 0; i < sourceResult.length; ++i) {
      if (sourceResult[i] != destResult[i]) {
        is(sourceResult[i] != destResult[i], test + ": Comparing char " + i);
        break;
      }
    }
  } finally {
    source.close();
    dest.close();
  }
  ok(true, test + ": Comparison complete");
}

function test_readall_writeall_file()
{
  let src_file_name = "chrome/toolkit/components/osfile/tests/mochi/worker_test_osfile_unix.js";
  let tmp_file_name = "test_osfile_front.tmp";
  ok(true, "Starting test_readall_writeall_file");

  

  let source = OS.File.open(src_file_name);
  let dest = OS.File.open(tmp_file_name, {write: true, trunc:true});
  let size = source.stat().size;

  let buf = new Uint8Array(size);
  let readResult = source.readTo(buf);
  is(readResult, size, "test_readall_writeall_file: read the right number of bytes");

  dest.write(buf);

  ok(true, "test_readall_writeall_file: copy complete (manual allocation)");
  source.close();
  dest.close();

  compare_files("test_readall_writeall_file (manual allocation)", src_file_name, tmp_file_name);
  OS.File.remove(tmp_file_name);

  

  source = OS.File.open(src_file_name);
  dest = OS.File.open(tmp_file_name, {write: true, trunc:true});
  buf = new ArrayBuffer(size);
  let ptr = OS.Shared.Type.voidptr_t.implementation(buf);
  readResult = source.readTo(ptr, {bytes: size});
  is(readResult, size, "test_readall_writeall_file: read the right number of bytes (C buffer)");

  dest.write(ptr, {bytes: size});

  ok(true, "test_readall_writeall_file: copy complete (C buffer)");
  source.close();
  dest.close();

  compare_files("test_readall_writeall_file (C buffer)", src_file_name, tmp_file_name);
  OS.File.remove(tmp_file_name);

  
  source = OS.File.open(src_file_name);
  dest = OS.File.open(tmp_file_name, {write: true, trunc:true});
  let exn = should_throw(function() { source.readTo(ptr); });
  ok(exn != null && exn instanceof TypeError, "test_readall_writeall_file: read with C pointer and without bytes fails with the correct error");
  exn = should_throw(function() { dest.write(ptr); });
  ok(exn != null && exn instanceof TypeError, "test_readall_writeall_file: write with C pointer and without bytes fails with the correct error");

  source.close();
  dest.close();

  
  let OFFSET = 12;
  let LEFT = size - OFFSET;
  buf = new ArrayBuffer(size);
  let offset_view = new Uint8Array(buf, OFFSET);
  source = OS.File.open(src_file_name);
  dest = OS.File.open(tmp_file_name, {write: true, trunc:true});

  readResult = source.readTo(offset_view);
  is(readResult, LEFT, "test_readall_writeall_file: read the right number of bytes (with offset)");

  dest.write(offset_view);
  is(dest.stat().size, LEFT, "test_readall_writeall_file: wrote the right number of bytes (with offset)");

  ok(true, "test_readall_writeall_file: copy complete (with offset)");
  source.close();
  dest.close();

  compare_files("test_readall_writeall_file (with offset)", src_file_name, tmp_file_name, LEFT);
  OS.File.remove(tmp_file_name);

  
  buf = new Uint8Array(size);
  source = OS.File.open(src_file_name);
  dest = OS.File.open(tmp_file_name, {write: true, trunc:true});

  readResult = source.read();
  is(readResult.length, size, "test_readall_writeall_file: read the right number of bytes (auto allocation)");

  dest.write(readResult);

  ok(true, "test_readall_writeall_file: copy complete (auto allocation)");
  source.close();
  dest.close();

  compare_files("test_readall_writeall_file (auto allocation)", src_file_name, tmp_file_name);
  OS.File.remove(tmp_file_name);

  
  readResult = OS.File.read(src_file_name);
  is(readResult.length, size, "test_readall_writeall_file: read the right number of bytes (OS.File.readAll)");
 
  
  OS.File.writeAtomic(tmp_file_name, readResult,
    {tmpPath: tmp_file_name + ".tmp"});
  try {
    let stat = OS.File.stat(tmp_file_name);
    ok(true, "readAll + writeAtomic created a file");
    is(stat.size, size, "readAll + writeAtomic created a file of the right size");
  } catch (x) {
    ok(false, "readAll + writeAtomic somehow failed");
    if(x.becauseNoSuchFile) {
      ok(false, "readAll + writeAtomic did not create file");
    }
  }
  compare_files("test_readall_writeall_file (OS.File.readAll + writeAtomic)",
                src_file_name, tmp_file_name);
  exn = null;
  try {
    let stat = OS.File.stat(tmp_file_name + ".tmp");
  } catch (x) {
    exn = x;
  }
  ok(!!exn, "readAll + writeAtomic cleaned up after itself");


  
  
  dest = OS.File.open(tmp_file_name, {write: true, trunc:true});
  dest.setPosition(1234);
  dest.close();

  OS.File.writeAtomic(tmp_file_name, readResult,
    {tmpPath: tmp_file_name + ".tmp"});
  compare_files("test_readall_writeall_file (OS.File.readAll + writeAtomic 2)",
                src_file_name, tmp_file_name);

  
  exn = null;
  try {
    let view = new Uint8Array(readResult.buffer, 10, 200);
    OS.File.writeAtomic(tmp_file_name, view,
      { tmpPath: tmp_file_name + ".tmp", noOverwrite: true});
  } catch (x) {
    exn = x;
  }
  ok(exn && exn instanceof OS.File.Error && exn.becauseExists, "writeAtomic fails if file already exists with noOverwrite option");
  
  compare_files("test_readall_writeall_file (OS.File.readAll + writeAtomic check file was not overwritten)",
                src_file_name, tmp_file_name);

  
  

  exn = null;
  try {
    OS.File.writeAtomic(tmp_file_name, readResult.buffer,
      {bytes: readResult.length});
  } catch (x) {
    exn = x;
  }
  ok(!!exn && exn instanceof TypeError, "writeAtomic fails if tmpPath is not provided");
}




function test_copy_existing_file()
{
  let src_file_name = "chrome/toolkit/components/osfile/tests/mochi/worker_test_osfile_unix.js";
  let tmp_file_name = "test_osfile_front.tmp";
  ok(true, "Starting test_copy_existing");
  OS.File.copy(src_file_name, tmp_file_name);

  ok(true, "test_copy_existing: Copy complete");
  compare_files("test_copy_existing", src_file_name, tmp_file_name);

  
  
  let dest = OS.File.open(tmp_file_name, {trunc: true});
  let buf = new Uint8Array(50);
  dest.write(buf);
  dest.close();

  OS.File.copy(src_file_name, tmp_file_name);

  compare_files("test_copy_existing 2", src_file_name, tmp_file_name);

  
  let exn;
  try {
    OS.File.copy(src_file_name, tmp_file_name, {noOverwrite: true});
  } catch(x) {
    exn = x;
  }
  ok(!!exn, "test_copy_existing: noOverwrite prevents overwriting existing files");


  ok(true, "test_copy_existing: Cleaning up");
  OS.File.remove(tmp_file_name);
}




function test_move_file()
{
  ok(true, "test_move_file: Starting");
  
  let src_file_name = "chrome/toolkit/components/osfile/tests/mochi/worker_test_osfile_unix.js";
  let tmp_file_name = "test_osfile_front.tmp";
  let tmp2_file_name = "test_osfile_front.tmp2";
  OS.File.copy(src_file_name, tmp_file_name);

  ok(true, "test_move_file: Copy complete");

  
  OS.File.move(tmp_file_name, tmp2_file_name);

  ok(true, "test_move_file: Move complete");

  
  compare_files("test_move_file", src_file_name, tmp2_file_name);

  
  let exn;
  try {
    OS.File.open(tmp_file_name);
  } catch (x) {
    exn = x;
  }
  ok(!!exn, "test_move_file: Original file has been removed");

  ok(true, "test_move_file: Cleaning up");
  OS.File.remove(tmp2_file_name);
}


function test_iter_dir()
{
  ok(true, "test_iter_dir: Starting");

  
  let tmp_file_name = "test_osfile_front.tmp";
  let tmp_file = OS.File.open(tmp_file_name, {write: true, trunc:true});
  tmp_file.close();

  let parent = OS.File.getCurrentDirectory();
  ok(true, "test_iter_dir: directory " + parent);
  let iterator = new OS.File.DirectoryIterator(parent);
  ok(true, "test_iter_dir: iterator created");
  let encountered_tmp_file = false;
  for (let entry in iterator) {
    
    ok(true, "test_iter_dir: encountering entry " + entry.name);

    if (entry.name == tmp_file_name) {
      encountered_tmp_file = true;
      isnot(entry.isDir, "test_iter_dir: The temporary file is not a directory");
      isnot(entry.isSymLink, "test_iter_dir: The temporary file is not a link");
    }

    let file;
    let success = true;
    try {
      file = OS.File.open(entry.path);
    } catch (x) {
      if (x.becauseNoSuchFile) {
        success = false;
      }
    }
    if (file) {
      file.close();
    }
    ok(success, "test_iter_dir: Entry " + entry.path + " exists");

    if (OS.Win) {
      let year = new Date().getFullYear();
      let creation = entry.winCreationDate;
      ok(creation, "test_iter_dir: Windows creation date exists: " + creation);
      ok(creation.getFullYear() >= year -  1 && creation.getFullYear() <= year, "test_iter_dir: consistent creation date");

      let lastWrite = entry.winLastWriteDate;
      ok(lastWrite, "test_iter_dir: Windows lastWrite date exists: " + lastWrite);
      ok(lastWrite.getFullYear() >= year - 1 && lastWrite.getFullYear() <= year, "test_iter_dir: consistent lastWrite date");

      let lastAccess = entry.winLastAccessDate;
      ok(lastAccess, "test_iter_dir: Windows lastAccess date exists: " + lastAccess);
      ok(lastAccess.getFullYear() >= year - 1 && lastAccess.getFullYear() <= year, "test_iter_dir: consistent lastAccess date");
    }

  }
  ok(encountered_tmp_file, "test_iter_dir: We have found the temporary file");

  ok(true, "test_iter_dir: Cleaning up");
  iterator.close();

  
  iterator = new OS.File.DirectoryIterator(parent);
  let allentries = [x for(x in iterator)];
  iterator.close();

  ok(allentries.length >= 14, "test_iter_dir: Meta-check: the test directory should contain at least 14 items");

  iterator = new OS.File.DirectoryIterator(parent);
  let firstten = iterator.nextBatch(10);
  is(firstten.length, 10, "test_iter_dir: nextBatch(10) returns 10 items");
  for (let i = 0; i < firstten.length; ++i) {
    is(allentries[i].path, firstten[i].path, "test_iter_dir: Checking that batch returns the correct entries");
  }
  let nextthree = iterator.nextBatch(3);
  is(nextthree.length, 3, "test_iter_dir: nextBatch(3) returns 3 items");
  for (let i = 0; i < nextthree.length; ++i) {
    is(allentries[i + firstten.length].path, nextthree[i].path, "test_iter_dir: Checking that batch 2 returns the correct entries");
  }
  let everythingelse = iterator.nextBatch();
  ok(everythingelse.length >= 1, "test_iter_dir: nextBatch() returns at least one item");
  for (let i = 0; i < everythingelse.length; ++i) {
    is(allentries[i + firstten.length + nextthree.length].path, everythingelse[i].path, "test_iter_dir: Checking that batch 3 returns the correct entries");
  }
  is(iterator.nextBatch().length, 0, "test_iter_dir: Once there is nothing left, nextBatch returns an empty array");
  iterator.close();

  iterator = new OS.File.DirectoryIterator(parent);
  iterator.close();
  is(iterator.nextBatch().length, 0, "test_iter_dir: nextBatch on closed iterator returns an empty array");

  iterator = new OS.File.DirectoryIterator(parent);
  let allentries2 = iterator.nextBatch();
  is(allentries.length, allentries2.length, "test_iter_dir: Checking that getBatch(null) returns the right number of entries");
  for (let i = 0; i < allentries.length; ++i) {
    is(allentries[i].path, allentries2[i].path, "test_iter_dir: Checking that getBatch(null) returns everything in the right order");
  }
  iterator.close();

  
  iterator = new OS.File.DirectoryIterator(parent);
  let index = 0;
  iterator.forEach(
    function cb(entry, aIndex, aIterator) {
      is(index, aIndex, "test_iter_dir: Checking that forEach index is correct");
      ok(iterator == aIterator, "test_iter_dir: Checking that right iterator is passed");
      if (index < 10) {
        is(allentries[index].path, entry.path, "test_iter_dir: Checking that forEach entry is correct");
      } else if (index == 10) {
        iterator.close();
      } else {
        ok(false, "test_iter_dir: Checking that forEach can be stopped early");
      }
      ++index;
  });
  iterator.close();

  ok(true, "test_iter_dir: Complete");
}

function test_position() {
  ok(true, "test_position: Starting");

  ok("POS_START" in OS.File, "test_position: POS_START exists");
  ok("POS_CURRENT" in OS.File, "test_position: POS_CURRENT exists");
  ok("POS_END" in OS.File, "test_position: POS_END exists");

  let ARBITRARY_POSITION = 321;
  let src_file_name = "chrome/toolkit/components/osfile/tests/mochi/worker_test_osfile_unix.js";


  let file = OS.File.open(src_file_name);
  is(file.getPosition(), 0, "test_position: Initial position is 0");

  let size = 0 + file.stat().size; 

  file.setPosition(ARBITRARY_POSITION, OS.File.POS_START);
  is(file.getPosition(), ARBITRARY_POSITION, "test_position: Setting position from start");

  file.setPosition(0, OS.File.POS_START);
  is(file.getPosition(), 0, "test_position: Setting position from start back to 0");

  file.setPosition(ARBITRARY_POSITION);
  is(file.getPosition(), ARBITRARY_POSITION, "test_position: Setting position without argument");

  file.setPosition(-ARBITRARY_POSITION, OS.File.POS_END);
  is(file.getPosition(), size - ARBITRARY_POSITION, "test_position: Setting position from end");

  file.setPosition(ARBITRARY_POSITION, OS.File.POS_CURRENT);
  is(file.getPosition(), size, "test_position: Setting position from current");

  file.close();
  ok(true, "test_position: Complete");
}

function test_info() {
  ok(true, "test_info: Starting");

  let filename = "test_info.tmp";
  let size = 261;
  let start = new Date();

 
  try {
    OS.File.remove(filename);
    ok(true, "test_info: Cleaned up previous garbage");
  } catch (x) {
    if (!x.becauseNoSuchFile) {
      throw x;
    }
    ok(true, "test_info: No previous garbage");
  }

  let file = OS.File.open(filename, {trunc: true});
  let buf = new ArrayBuffer(size);
  file._write(buf, size);
  file.close();

  
  let info = OS.File.stat(filename);
  ok(!!info, "test_info: info acquired");
  ok(!info.isDir, "test_info: file is not a directory");
  is(info.isSymLink, false, "test_info: file is not a link");
  is(info.size.toString(), size, "test_info: correct size");

  let stop = new Date();

  
  let startMs = start.getTime() - 1000;
  let stopMs  = stop.getTime() + 1000;

  let birth = info.creationDate;
  ok(birth.getTime() <= stopMs,
     "test_info: file was created before now - " + stop + ", " + birth);
  
  
  
  
  
  
  

  let change = info.lastModificationDate;
  ok(change.getTime() >= startMs
     && change.getTime() <= stopMs,
     "test_info: file has changed between the start of the test and now - " + start + ", " + stop + ", " + change);

  
  file = OS.File.open(filename);
  try {
    info = file.stat();
  } finally {
    file.close();
  }

  ok(!!info, "test_info: info acquired 2");
  ok(!info.isDir, "test_info: file is not a directory 2");
  ok(!info.isSymLink, "test_info: file is not a link 2");
  is(info.size.toString(), size, "test_info: correct size 2");

  stop = new Date();

  
  startMs = start.getTime() - 1000;
  stopMs  = stop.getTime() + 1000;

  birth = info.creationDate;
  ok(birth.getTime() <= stopMs,
      "test_info: file 2 was created between the start of the test and now - " + start +  ", " + stop + ", " + birth);

  let access = info.lastModificationDate;
  ok(access.getTime() >= startMs
     && access.getTime() <= stopMs,
     "test_info: file 2 was accessed between the start of the test and now - " + start + ", " + stop + ", " + access);

  change = info.lastModificationDate;
  ok(change.getTime() >= startMs
     && change.getTime() <= stopMs,
     "test_info: file 2 has changed between the start of the test and now - " + start + ", " + stop + ", " + change);

  
  info = OS.File.stat(OS.File.getCurrentDirectory());
  ok(!!info, "test_info: info on directory acquired");
  ok(info.isDir, "test_info: directory is a directory");

  ok(true, "test_info: Complete");
}

function test_mkdir()
{
  ok(true, "test_mkdir: Starting");

  let dirName = "test_dir.tmp";
  OS.File.removeEmptyDir(dirName, {ignoreAbsent: true});

  
  let exn;
  try {
    OS.File.removeEmptyDir(dirName, {ignoreAbsent: true});
  } catch (x) {
    exn = x;
  }
  ok(!exn, "test_mkdir: ignoreAbsent works");

  exn = null;
  try {
    OS.File.removeEmptyDir(dirName);
  } catch (x) {
    exn = x;
  }
  ok(!!exn, "test_mkdir: removeDir throws if there is no such directory");
  ok(exn instanceof OS.File.Error && exn.becauseNoSuchFile, "test_mkdir: removeDir throws the correct exception if there is no such directory");

  ok(true, "test_mkdir: Creating directory");
  OS.File.makeDir(dirName);
  ok(OS.File.stat(dirName).isDir, "test_mkdir: Created directory is a directory");

  ok(true, "test_mkdir: Creating directory that already exists");
  exn = null;
  try {
    OS.File.makeDir(dirName);
  } catch (x) {
    exn = x;
  }
  ok(exn && exn instanceof OS.File.Error && exn.becauseExists, "test_mkdir: makeDir over an existing directory failed for all the right reasons");

  
  OS.File.removeEmptyDir(dirName);

  try {
    OS.File.stat(dirName);
    ok(false, "test_mkdir: Directory was not removed");
  } catch (x) {
    ok(x instanceof OS.File.Error && x.becauseNoSuchFile, "test_mkdir: Directory was removed");
  }

  ok(true, "test_mkdir: Complete");
}



function test_path()
{
  ok(true, "test_path: starting");
  let abcd = OS.Path.join("a", "b", "c", "d");
  is(OS.Path.basename(abcd), "d", "basename of a/b/c/d");

  let abc = OS.Path.join("a", "b", "c");
  is(OS.Path.dirname(abcd), abc, "dirname of a/b/c/d");

  let abdotsc = OS.Path.join("a", "b", "..", "c");
  is(OS.Path.normalize(abdotsc), OS.Path.join("a", "c"), "normalize a/b/../c");

  let adotsdotsdots = OS.Path.join("a", "..", "..", "..");
  is(OS.Path.normalize(adotsdotsdots), OS.Path.join("..", ".."), "normalize a/../../..");

  ok(true, "test_path: Complete");
}




function test_exists_file()
{
  let file_name = OS.Path.join("chrome", "toolkit", "components" ,"osfile",
                               "tests", "mochi", "test_osfile_front.xul");
  ok(true, "test_exists_file: starting");
  ok(OS.File.exists(file_name), "test_exists_file: file exists (OS.File.exists)");
  ok(!OS.File.exists(file_name + ".tmp"), "test_exists_file: file does not exists (OS.File.exists)");
  ok(true, "test_exists_file: complete");
}
