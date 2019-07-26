
























exports['test path'] = function(assert) {

var system = require('sdk/system');
var path = require('sdk/fs/path');
var isWindows = require('sdk/system').platform.indexOf('win') === 0;




if (!isWindows) {
  var controlCharFilename = 'Icon' + String.fromCharCode(13);
  assert.equal(path.basename('/a/b/' + controlCharFilename),
               controlCharFilename);
}


assert.equal(path.dirname('/a/b/'), '/a');
assert.equal(path.dirname('/a/b'), '/a');
assert.equal(path.dirname('/a'), '/');
assert.equal(path.dirname('/'), '/');

if (isWindows) {
  assert.equal(path.dirname('c:\\'), 'c:\\');
  assert.equal(path.dirname('c:\\foo'), 'c:\\');
  assert.equal(path.dirname('c:\\foo\\'), 'c:\\');
  assert.equal(path.dirname('c:\\foo\\bar'), 'c:\\foo');
  assert.equal(path.dirname('c:\\foo\\bar\\'), 'c:\\foo');
  assert.equal(path.dirname('c:\\foo\\bar\\baz'), 'c:\\foo\\bar');
  assert.equal(path.dirname('\\'), '\\');
  assert.equal(path.dirname('\\foo'), '\\');
  assert.equal(path.dirname('\\foo\\'), '\\');
  assert.equal(path.dirname('\\foo\\bar'), '\\foo');
  assert.equal(path.dirname('\\foo\\bar\\'), '\\foo');
  assert.equal(path.dirname('\\foo\\bar\\baz'), '\\foo\\bar');
  assert.equal(path.dirname('c:'), 'c:');
  assert.equal(path.dirname('c:foo'), 'c:');
  assert.equal(path.dirname('c:foo\\'), 'c:');
  assert.equal(path.dirname('c:foo\\bar'), 'c:foo');
  assert.equal(path.dirname('c:foo\\bar\\'), 'c:foo');
  assert.equal(path.dirname('c:foo\\bar\\baz'), 'c:foo\\bar');
  assert.equal(path.dirname('\\\\unc\\share'), '\\\\unc\\share');
  assert.equal(path.dirname('\\\\unc\\share\\foo'), '\\\\unc\\share\\');
  assert.equal(path.dirname('\\\\unc\\share\\foo\\'), '\\\\unc\\share\\');
  assert.equal(path.dirname('\\\\unc\\share\\foo\\bar'),
               '\\\\unc\\share\\foo');
  assert.equal(path.dirname('\\\\unc\\share\\foo\\bar\\'),
               '\\\\unc\\share\\foo');
  assert.equal(path.dirname('\\\\unc\\share\\foo\\bar\\baz'),
               '\\\\unc\\share\\foo\\bar');
}


assert.equal(path.extname(''), '');
assert.equal(path.extname('/path/to/file'), '');
assert.equal(path.extname('/path/to/file.ext'), '.ext');
assert.equal(path.extname('/path.to/file.ext'), '.ext');
assert.equal(path.extname('/path.to/file'), '');
assert.equal(path.extname('/path.to/.file'), '');
assert.equal(path.extname('/path.to/.file.ext'), '.ext');
assert.equal(path.extname('/path/to/f.ext'), '.ext');
assert.equal(path.extname('/path/to/..ext'), '.ext');
assert.equal(path.extname('file'), '');
assert.equal(path.extname('file.ext'), '.ext');
assert.equal(path.extname('.file'), '');
assert.equal(path.extname('.file.ext'), '.ext');
assert.equal(path.extname('/file'), '');
assert.equal(path.extname('/file.ext'), '.ext');
assert.equal(path.extname('/.file'), '');
assert.equal(path.extname('/.file.ext'), '.ext');
assert.equal(path.extname('.path/file.ext'), '.ext');
assert.equal(path.extname('file.ext.ext'), '.ext');
assert.equal(path.extname('file.'), '.');
assert.equal(path.extname('.'), '');
assert.equal(path.extname('./'), '');
assert.equal(path.extname('.file.ext'), '.ext');
assert.equal(path.extname('.file'), '');
assert.equal(path.extname('.file.'), '.');
assert.equal(path.extname('.file..'), '.');
assert.equal(path.extname('..'), '');
assert.equal(path.extname('../'), '');
assert.equal(path.extname('..file.ext'), '.ext');
assert.equal(path.extname('..file'), '.file');
assert.equal(path.extname('..file.'), '.');
assert.equal(path.extname('..file..'), '.');
assert.equal(path.extname('...'), '.');
assert.equal(path.extname('...ext'), '.ext');
assert.equal(path.extname('....'), '.');
assert.equal(path.extname('file.ext/'), '');

if (isWindows) {
  
  assert.equal(path.extname('.\\'), '');
  assert.equal(path.extname('..\\'), '');
  assert.equal(path.extname('file.ext\\'), '');
} else {
  
  assert.equal(path.extname('.\\'), '');
  assert.equal(path.extname('..\\'), '.\\');
  assert.equal(path.extname('file.ext\\'), '.ext\\');
}


var failures = [];
var joinTests =
    
    [[['.', 'x/b', '..', '/b/c.js'], 'x/b/c.js'],
     [['/.', 'x/b', '..', '/b/c.js'], '/x/b/c.js'],
     [['/foo', '../../../bar'], '/bar'],
     [['foo', '../../../bar'], '../../bar'],
     [['foo/', '../../../bar'], '../../bar'],
     [['foo/x', '../../../bar'], '../bar'],
     [['foo/x', './bar'], 'foo/x/bar'],
     [['foo/x/', './bar'], 'foo/x/bar'],
     [['foo/x/', '.', 'bar'], 'foo/x/bar'],
     [['./'], './'],
     [['.', './'], './'],
     [['.', '.', '.'], '.'],
     [['.', './', '.'], '.'],
     [['.', '/./', '.'], '.'],
     [['.', '/////./', '.'], '.'],
     [['.'], '.'],
     [['', '.'], '.'],
     [['', 'foo'], 'foo'],
     [['foo', '/bar'], 'foo/bar'],
     [['', '/foo'], '/foo'],
     [['', '', '/foo'], '/foo'],
     [['', '', 'foo'], 'foo'],
     [['foo', ''], 'foo'],
     [['foo/', ''], 'foo/'],
     [['foo', '', '/bar'], 'foo/bar'],
     [['./', '..', '/foo'], '../foo'],
     [['./', '..', '..', '/foo'], '../../foo'],
     [['.', '..', '..', '/foo'], '../../foo'],
     [['', '..', '..', '/foo'], '../../foo'],
     [['/'], '/'],
     [['/', '.'], '/'],
     [['/', '..'], '/'],
     [['/', '..', '..'], '/'],
     [[''], '.'],
     [['', ''], '.'],
     [[' /foo'], ' /foo'],
     [[' ', 'foo'], ' /foo'],
     [[' ', '.'], ' '],
     [[' ', '/'], ' /'],
     [[' ', ''], ' '],
     
     [['x', true, 7, 'y', null, {}], 'x/y']
    ];
joinTests.forEach(function(test) {
  var actual = path.join.apply(path, test[0]);
  var expected = isWindows ? test[1].replace(/\//g, '\\') : test[1];
  var message = 'path.join(' + test[0].map(JSON.stringify).join(',') + ')' +
                '\n  expect=' + JSON.stringify(expected) +
                '\n  actual=' + JSON.stringify(actual);
  if (actual !== expected) failures.push('\n' + message);
  
});
assert.equal(failures.length, 0, failures.join(''));


if (isWindows) {
  assert.equal(path.normalize('./fixtures///b/../b/c.js'),
               'fixtures\\b\\c.js');
  assert.equal(path.normalize('/foo/../../../bar'), '\\bar');
  assert.equal(path.normalize('a//b//../b'), 'a\\b');
  assert.equal(path.normalize('a//b//./c'), 'a\\b\\c');
  assert.equal(path.normalize('a//b//.'), 'a\\b');
  assert.equal(path.normalize('//server/share/dir/file.ext'),
               '\\\\server\\share\\dir\\file.ext');
} else {
  assert.equal(path.normalize('./fixtures///b/../b/c.js'),
               'fixtures/b/c.js');
  assert.equal(path.normalize('/foo/../../../bar'), '/bar');
  assert.equal(path.normalize('a//b//../b'), 'a/b');
  assert.equal(path.normalize('a//b//./c'), 'a/b/c');
  assert.equal(path.normalize('a//b//.'), 'a/b');
}


if (isWindows) {
  
  var resolveTests =
      
      [[['c:/blah\\blah', 'd:/games', 'c:../a'], 'c:\\blah\\a'],
       [['c:/ignore', 'd:\\a/b\\c/d', '\\e.exe'], 'd:\\e.exe'],
       [['c:/ignore', 'c:/some/file'], 'c:\\some\\file'],
       [['d:/ignore', 'd:some/dir//'], 'd:\\ignore\\some\\dir'],
       [['.'], system.pathFor('CurProcD')],
       [['//server/share', '..', 'relative\\'], '\\\\server\\share\\relative']];
} else {
  
  var resolveTests =
      
      [[['/var/lib', '../', 'file/'], '/var/file'],
       [['/var/lib', '/../', 'file/'], '/file'],
       [['a/b/c/', '../../..'], system.pathFor('CurProcD')],
       [['.'], system.pathFor('CurProcD')],
       [['/some/dir', '.', '/absolute/'], '/absolute']];
}
var failures = [];
resolveTests.forEach(function(test) {
  var actual = path.resolve.apply(path, test[0]);
  var expected = test[1];
  var message = 'path.resolve(' + test[0].map(JSON.stringify).join(',') + ')' +
                '\n  expect=' + JSON.stringify(expected) +
                '\n  actual=' + JSON.stringify(actual);
  if (actual !== expected) failures.push('\n' + message);
  
});
assert.equal(failures.length, 0, failures.join(''));


if (isWindows) {
  
  var relativeTests =
      
      [['c:/blah\\blah', 'd:/games', 'd:\\games'],
       ['c:/aaaa/bbbb', 'c:/aaaa', '..'],
       ['c:/aaaa/bbbb', 'c:/cccc', '..\\..\\cccc'],
       ['c:/aaaa/bbbb', 'c:/aaaa/bbbb', ''],
       ['c:/aaaa/bbbb', 'c:/aaaa/cccc', '..\\cccc'],
       ['c:/aaaa/', 'c:/aaaa/cccc', 'cccc'],
       ['c:/', 'c:\\aaaa\\bbbb', 'aaaa\\bbbb'],
       ['c:/aaaa/bbbb', 'd:\\', 'd:\\']];
} else {
  
  var relativeTests =
      
      [['/var/lib', '/var', '..'],
       ['/var/lib', '/bin', '../../bin'],
       ['/var/lib', '/var/lib', ''],
       ['/var/lib', '/var/apache', '../apache'],
       ['/var/', '/var/lib', 'lib'],
       ['/', '/var/lib', 'var/lib']];
}
var failures = [];
relativeTests.forEach(function(test) {
  var actual = path.relative(test[0], test[1]);
  var expected = test[2];
  var message = 'path.relative(' +
                test.slice(0, 2).map(JSON.stringify).join(',') +
                ')' +
                '\n  expect=' + JSON.stringify(expected) +
                '\n  actual=' + JSON.stringify(actual);
  if (actual !== expected) failures.push('\n' + message);
});
assert.equal(failures.length, 0, failures.join(''));


if (isWindows) {
    
    assert.equal(path.sep, '\\');
} else {
    
    assert.equal(path.sep, '/');
}

};

require('test').run(exports);
