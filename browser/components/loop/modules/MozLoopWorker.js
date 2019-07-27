







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

  
  resetIpMask();
  ping.payload.localSdp = redactSdp(ping.payload.localSdp);
  ping.payload.remoteSdp = redactSdp(ping.payload.remoteSdp);
  ping.payload.log = sanitizeLogs(ping.payload.log);

  let pingStr = anonymizeIPv4(sanitizeUrls(JSON.stringify(ping)));

  
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





let IpMap = {};
let IpCount = 0;

function resetIpMask() {
  IpMap = {};
  IpCount = Math.floor(Math.random() * 16777215) + 1;
}






function maskIp(ip) {
  let isInvalidOrRfc1918or3927 = function(p1, p2, p3, p4) {
    let invalid = octet => octet < 0 || octet > 255;
    return invalid(p1) || invalid(p2) || invalid(p3) || invalid(p4) ||
    (p1 == 10) ||
    (p1 == 172 && p2 >= 16 && p2 <= 31) ||
    (p1 == 192 && p2 == 168) ||
    (p1 == 169 && p2 == 254);
  };

  let [p1, p2, p3, p4] = ip.split(".");

  if (isInvalidOrRfc1918or3927(p1, p2, p3, p4)) {
    return ip;
  }
  let key = [p1, p2, p3].join();
  if (!IpMap[key]) {
    do {
      IpCount = (IpCount + 1049039) % 16777216; 
      p1 = (IpCount >> 16) % 256;
      p2 = (IpCount >> 8) % 256;
      p3 = IpCount % 256;
    } while (isInvalidOrRfc1918or3927(p1, p2, p3, p4));
    IpMap[key] = p1 + "." + p2 + "." + p3;
  }
  return IpMap[key] + "." + p4;
}






function anonymizeIPv4(text) {
  return text.replace(/\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b/g,
                      maskIp.bind(this));
}












function sanitizeUrls(text) {
  let trimUrl = url => url.replace(/(#call|#incoming).*/g,
                                   (match, type) => type + "/xxxx");
  return text.replace(/\(id=(\d+) url=([^\)]+)\)/g,
                      (match, id, url) =>
                      "(id=" + id + " url=" + trimUrl(url) + ")");
}











let redactSdp = sdp => sdp.replace(/\r\na=(fingerprint|identity):.*?\r\n/g,
                                   "\r\n");









function sanitizeLogs(log) {
  let rex = /(srflx|relay)\(IP4:\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d{1,5}\/(UDP|TCP)\|[^\)]+\)/g;

  return log.replace(rex, match => match.replace(/\|[^\)]+\)/, "|xxxx.xxx)"));
}
