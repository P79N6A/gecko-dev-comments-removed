#!/usr/bin/env node





module.exports = function (grunt) {
  'use strict';

  
  require('time-grunt')(grunt);

  
  require('load-grunt-tasks')(grunt, {scope: 'devDependencies'});

  grunt.initConfig({
  });

  grunt.loadTasks('grunttasks');
}
;
