




TEST_MAIN_JS = '''\
var main = require("./main");

exports["test main"] = function(assert) {
  assert.pass("Unit test running!");
};

exports["test main async"] = function(assert, done) {
  assert.pass("async Unit test running!");
  done();
};

require("sdk/test").run(exports);
'''


PACKAGE_JSON = '''\
{
  "name": "%(name)s",
  "fullName": "%(fullName)s",
  "id": "%(id)s",
  "description": "a basic add-on",
  "author": "",
  "license": "MPL 2.0",
  "version": "0.1"
}
'''
