


"use strict";




const TESTCASE_URI = TEST_BASE_HTTP + "minified.html";

const PRETTIFIED_SOURCE = "" +
"body\{\r?\n" +                   
  "\tbackground\:white;\r?\n" +   
"\}\r?\n" +                       
"\r?\n" +                         
"div\{\r?\n" +                    
  "\tfont\-size\:4em;\r?\n" +     
  "\tcolor\:red\r?\n" +           
"\}\r?\n" +                       
"\r?\n" +                         
"span\{\r?\n" +                   
  "\tcolor\:green;\r?\n"          
"\}\r?\n";                        

const ORIGINAL_SOURCE = "" +
"body \{ background\: red; \}\r?\n" + 
"div \{\r?\n" +                       
  "font\-size\: 5em;\r?\n" +          
  "color\: red\r?\n" +                
"\}";                                 

add_task(function* () {
  let { ui } = yield openStyleEditorForURL(TESTCASE_URI);
  is(ui.editors.length, 2, "Two sheets present.");

  info("Testing minified style sheet.");
  let editor = yield ui.editors[0].getSourceEditor();

  let prettifiedSourceRE = new RegExp(PRETTIFIED_SOURCE);
  ok(prettifiedSourceRE.test(editor.sourceEditor.getText()),
     "minified source has been prettified automatically");

  info("Selecting second, non-minified style sheet.");
  yield ui.selectStyleSheet(ui.editors[1].styleSheet);

  editor = ui.editors[1];

  let originalSourceRE = new RegExp(ORIGINAL_SOURCE);
  ok(originalSourceRE.test(editor.sourceEditor.getText()),
     "non-minified source has been left untouched");
});
