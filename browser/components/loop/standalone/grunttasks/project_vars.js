



module.exports = function (grunt) {
  'use strict';

  var TEMPLATE_ROOT = 'content/legal';
  var TOS_PP_REPO_ROOT = 'bower_components/tos-pp';

  grunt.config('project_vars', {
    app: "content",
    
    tos_pp_repo_dest: TOS_PP_REPO_ROOT,
    tos_md_src: TOS_PP_REPO_ROOT + '/WebRTC_ToS/',
    tos_html_dest: TEMPLATE_ROOT + '/terms'
  });
};
