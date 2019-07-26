




const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test()
{
  
  let base = Cc['@mozilla.org/file/directory_service;1']
             .getService(Ci.nsIProperties)
             .get('TmpD', Ci.nsILocalFile);
  base.append('renameTesting');
  if (base.exists()) {
    base.remove(true);
  }
  base.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt('0777', 8));

  
  let subdir = base.clone();
  subdir.append('subdir');
  subdir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt('0777', 8));

  
  let tempFile = subdir.clone();
  tempFile.append('file0.txt');
  tempFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt('0777', 8));

  
  tempFile.renameTo(null, 'file1.txt');
  do_check_true(exists(subdir, 'file1.txt'));

  
  tempFile = subdir.clone();
  tempFile.append('file1.txt');
  tempFile.renameTo(base, '');
  do_check_true(exists(base, 'file1.txt'));

  
  tempFile = base.clone();
  tempFile.append('file1.txt');
  tempFile.renameTo(subdir, 'file2.txt');
  do_check_true(exists(subdir, 'file2.txt'));

  
  subdir.renameTo(base, 'renamed');
  do_check_true(exists(base, 'renamed'));
  let renamed = base.clone();
  renamed.append('renamed');
  do_check_true(exists(renamed, 'file2.txt'));

  base.remove(true);
}

function exists(parent, filename) {
  let file = parent.clone();
  file.append(filename);
  return file.exists();
}
