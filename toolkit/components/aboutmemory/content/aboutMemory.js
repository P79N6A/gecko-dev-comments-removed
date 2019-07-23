





































var gMemReporters = { };

function $(n) {
    return document.getElementById(n);
}

function makeTableCell(s, c) {
    var td = document.createElement("td");
    var text = document.createTextNode(s);
    td.appendChild(text);
    if (c)
        td.setAttribute("class", c);

    return td;
}

function makeAbbrNode(str, title) {
    var abbr = document.createElement("abbr");
    var text = document.createTextNode(str);
    abbr.appendChild(text);
    abbr.setAttribute("title", title);

    return abbr;
}

function makeTableRow() {
    var row = document.createElement("tr");

    for (var i = 0; i < arguments.length; ++i) {
        var arg = arguments[i];
        if (typeof(arg) == "string") {
            row.appendChild(makeTableCell(arg));
        } else if (arg.__proto__ == Array.prototype) {
            row.appendChild(makeAbbrNode(arg[0], arg[1]));
        } else {
            row.appendChild(arg);
        }
    }

    return row;
}

function setTextContent(node, s) {
    while (node.lastChild)
        node.removeChild(node.lastChild);

    node.appendChild(document.createTextNode(s));
}

function formatNumber(n) {
    var s = "";
    var neg = false;
    if (n < 0) {
        neg = true;
        n = -n;
    }

    do {
        var k = n % 1000;
        if (n > 999) {
            if (k > 99)
                s = k + s;
            else if (k > 9)
                s = "0" + k + s;
            else
                s = "00" + k + s;
        } else {
            s = k + s;
        }

        n = Math.floor(n / 1000);
        if (n > 0)
            s = "," + s;
    } while (n > 0);

    return s;
}

function updateMemoryStatus()
{
    
    
    if ("malloc/mapped" in gMemReporters &&
        "malloc/allocated" in gMemReporters)
    {
        
        
        setTextContent($("memMappedValue"),
                       formatNumber(gMemReporters["malloc/mapped"].memoryUsed));

        
        setTextContent($("memInUseValue"),
                       formatNumber(gMemReporters["malloc/allocated"].memoryUsed));
    } else {
        $("memOverview").style.display = "none";
    }

    var mo = $("memOtherRows");
    while (mo.lastChild)
        mo.removeChild(mo.lastChild);

    var otherCount = 0;
    for each (var rep in gMemReporters) {
        var row = makeTableRow([rep.path, rep.description],
                               makeTableCell(formatNumber(rep.memoryUsed), "memValue"));

        mo.appendChild(row);

        otherCount++;
    }

    if (otherCount == 0) {
        var row = makeTableRow("No other information available.");
        mo.appendChild(row);
    }
}

function doLoad()
{
    var mgr = Components
        .classes["@mozilla.org/memory-reporter-manager;1"]
        .getService(Components.interfaces.nsIMemoryReporterManager);

    var e = mgr.enumerateReporters();
    while (e.hasMoreElements()) {
        var mr = e.getNext().QueryInterface(Components.interfaces.nsIMemoryReporter);
        gMemReporters[mr.path] = mr;
    }

    updateMemoryStatus();
}

