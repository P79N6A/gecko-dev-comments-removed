

Cu.import("resource://gre/modules/NetUtil.jsm");

function test() {
    var file = new File(new Blob(['test'], {type: 'text/plain'}), {name: 'test-name'});
    var url = URL.createObjectURL(file);
    var channel = NetUtil.newChannel(url);

    is(channel.contentDispositionFilename, 'test-name', "filename matches");
}
