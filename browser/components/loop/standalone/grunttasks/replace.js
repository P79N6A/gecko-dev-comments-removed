



module.exports = function (grunt) {
  'use strict';

  grunt.config('replace', {
    tos: {
      src: [
        '<%= project_vars.tos_md_src %>/*.md'
      ],
      overwrite: true,
      replacements: [{
        
        from: /{:\s.*?\s}/g,
        to: ''
      }, {
        
        from: /^#\s.*?\n$/m,
        to: ''
      }, {
        
        from: /(“|”)/g,
        to: "&quot;"
      }, {
        
        from: /’/g,
        to: "&#39;"
      }, {
        from: /–/g,
        to: "&mdash;"
      }
      ]
    }
  });
};
