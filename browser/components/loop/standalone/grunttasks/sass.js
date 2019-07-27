



module.exports = function (grunt) {
  'use strict';

  grunt.config('sass', {
    styles: {
      files: {
        '<%= project_vars.app %>/legal/styles/main.css':
          '<%= project_vars.app %>/legal/styles/main.scss'
      }
    }
  });
};
