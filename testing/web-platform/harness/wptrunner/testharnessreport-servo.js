



var props = {output:%(output)d};

setup(props);

add_completion_callback(function (tests, harness_status) {
    var id = location.pathname + location.search + location.hash;
    alert("RESULT: " + JSON.stringify([
        id,
        harness_status.status,
        harness_status.message,
        harness_status.stack,
        tests.map(function(t) {
            return [t.name, t.status, t.message, t.stack]
        }),
    }));
});
