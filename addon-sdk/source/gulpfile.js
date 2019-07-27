


"use strict";

var gulp = require('gulp');

gulp.task('test', function(done) {
  require("./bin/jpm-test").run().then(done);
});

gulp.task('test:addons', function(done) {
  require("./bin/jpm-test").run("addons").then(done);
});

gulp.task('test:examples', function(done) {
  require("./bin/jpm-test").run("examples").then(done);
});

gulp.task('test:modules', function(done) {
  require("./bin/jpm-test").run("modules").then(done);
});

