

Cu.import("resource://gre/modules/NetUtil.jsm");

function test() {
    var file = new File([new Blob(['test'], {type: 'text/plain'})], "test-name");
    var url = URL.createObjectURL(file);
    var channel = NetUtil.newChannel({uri: url, loadUsingSystemPrincipal: true});

    is(channel.contentDispositionFilename, 'test-name', "filename matches");
}
