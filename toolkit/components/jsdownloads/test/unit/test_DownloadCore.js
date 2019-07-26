








"use strict";




let gUseLegacySaver = false;

let scriptFile = do_get_file("common_test_Download.js");
Services.scriptloader.loadSubScript(NetUtil.newURI(scriptFile).spec);




let tailFile = do_get_file("tail.js");
Services.scriptloader.loadSubScript(NetUtil.newURI(tailFile).spec);
