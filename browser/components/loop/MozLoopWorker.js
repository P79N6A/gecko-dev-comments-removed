







"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let File = OS.File;
let Encoder = new TextEncoder();
let Counter = 0;

const MAX_LOOP_LOGS = 5;











onmessage = function(e) {
  if (++Counter > MAX_LOOP_LOGS) {
    postMessage({
      fail: "Maximum " + MAX_LOOP_LOGS + "loop reports reached for this session"
    });
    return;
  }

  let directory = e.data.directory;
  let filename = e.data.filename;
  let ping = e.data.ping;

  let pingStr = JSON.stringify(ping);

  
  let array = Encoder.encode(pingStr);
  try {
    File.makeDir(directory,
                 { unixMode: OS.Constants.S_IRWXU, ignoreExisting: true });
    File.writeAtomic(OS.Path.join(directory, filename), array);
    postMessage({ ok: true });
  } catch (ex if ex instanceof File.Error) {
    
    postMessage({fail: File.Error.toMsg(ex)});
  }
};
