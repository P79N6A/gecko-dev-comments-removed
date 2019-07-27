



var path = require('path');
var i18n = require('i18n-abide');

module.exports = function (grunt) {
  'use strict';

  

  function rename(destPath, srcFile) {
    
    var lang = srcFile.replace('.md', '');
    return path.join(destPath, i18n.localeFrom(lang) + '.html');
  }

  grunt.config('marked', {
    options: {
      sanitize: false,
      gfm: true
    },
    tos: {
      files: [
        {
          expand: true,
          cwd: '<%= project_vars.tos_md_src %>',
          src: ['**/*.md'],
          dest: '<%= project_vars.tos_html_dest %>',
          rename: rename
        }
      ]
    }
  });
};
