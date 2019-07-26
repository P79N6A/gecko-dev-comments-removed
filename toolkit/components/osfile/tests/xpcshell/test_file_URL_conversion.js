



function run_test() {
  Components.utils.import("resource://gre/modules/Services.jsm");
  Components.utils.import("resource://gre/modules/osfile.jsm");
  Components.utils.import("resource://gre/modules/FileUtils.jsm");

  let isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);

  
  let paths = isWindows ? [
    'C:\\',
    'C:\\test',
    'C:\\test\\',
    'C:\\test%2f',
    'C:\\test\\test\\test',
    'C:\\test;+%',
    'C:\\test?action=index\\',
    'C:\\test\ test',
    '\\\\C:\\a\\b\\c',
    '\\\\Server\\a\\b\\c',

    
    
    'C:\\char^',
    'C:\\char&',
    'C:\\char\'',
    'C:\\char@',
    'C:\\char{',
    'C:\\char}',
    'C:\\char[',
    'C:\\char]',
    'C:\\char,',
    'C:\\char$',
    'C:\\char=',
    'C:\\char!',
    'C:\\char-',
    'C:\\char#',
    'C:\\char(',
    'C:\\char)',
    'C:\\char%',
    'C:\\char.',
    'C:\\char+',
    'C:\\char~',
    'C:\\char_'
  ] : [
    '/',
    '/test',
    '/test/',
    '/test%2f',
    '/test/test/test',
    '/test;+%',
    '/test?action=index/',
    '/test\ test',
    '/punctuation/;,/?:@&=+$-_.!~*\'()"#',
    '/CasePreserving'
  ];

  
  let uris = isWindows ? [
    'file:///C:/test/',
    'file://localhost/C:/test',
    'file:///c:/test/test.txt',
    
    'file:///C:/%3f%3F',
    'file:///C:/%3b%3B',
    'file:///C:/%3c%3C', 
    'file:///C:/%78', 
    'file:///C:/test#frag', 
    'file:///C:/test?action=index' 
  ] : [
    'file:///test/',
    'file://localhost/test',
    'file:///test/test.txt',
    'file:///foo%2f', 
    'file:///%3f%3F',
    'file:///%3b%3B',
    'file:///%3c%3C', 
    'file:///%78', 
    'file:///test#frag', 
    'file:///test?action=index' 
  ];

  for (let path of paths) {
    
    let file = FileUtils.File(path);
    let uri = Services.io.newFileURI(file).spec;
    do_check_eq(uri, OS.Path.toFileURI(path));

    
    uris.push(uri)
  }

  for (let uri of uris) {
    
    let path = Services.io.newURI(uri, null, null).QueryInterface(Components.interfaces.nsIFileURL).file.path;
    do_check_eq(path, OS.Path.fromFileURI(uri));
  }

  
  let thrown = false;
  try {
    OS.Path.fromFileURI('http://test.com')
  } catch (e) {
    do_check_eq(e.message, "fromFileURI expects a file URI");
    thrown = true;
  }
  do_check_true(thrown);
}
