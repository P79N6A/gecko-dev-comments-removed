
var numSamples = 500;
var delays = new Array(numSamples);
var sampleIndex = 0;
var sampleTime = 16; 
var gHistogram = new Map(); 


var stopped = 0;
var start;
var prev;
var ctx;


var garbagePerFrame = undefined;
var garbageTotal = undefined;
var activeTest = undefined;
var testDuration = undefined; 
var testState = 'idle';  
var testStart = undefined; 
var testQueue = [];

function xpos(index)
{
    return index * 2;
}

function ypos(delay)
{
    var r = 525 - Math.log(delay) * 64;
    if (r < 5) return 5;
    return r;
}

function drawHBar(delay, color, label)
{
    ctx.fillStyle = color;
    ctx.strokeStyle = color;
    ctx.fillText(label, xpos(numSamples) + 4, ypos(delay) + 3);

    ctx.beginPath();
    ctx.moveTo(xpos(0), ypos(delay));
    ctx.lineTo(xpos(numSamples), ypos(delay));
    ctx.stroke();
    ctx.strokeStyle = 'rgb(0,0,0)';
    ctx.fillStyle = 'rgb(0,0,0)';
}

function drawScale(delay)
{
    drawHBar(delay, 'rgb(150,150,150)', `${delay}ms`);
}

function draw60fps() {
    drawHBar(1000/60, '#00cf61', '60fps');
}

function draw30fps() {
    drawHBar(1000/30, '#cf0061', '30fps');
}

function drawGraph()
{
    ctx.clearRect(0, 0, 1100, 550);

    drawScale(10);
    draw60fps();
    drawScale(20);
    drawScale(30);
    draw30fps();
    drawScale(50);
    drawScale(100);
    drawScale(200);
    drawScale(400);
    drawScale(800);

    var worst = 0, worstpos = 0;
    ctx.beginPath();
    for (var i = 0; i < numSamples; i++) {
        ctx.lineTo(xpos(i), ypos(delays[i]));
        if (delays[i] >= worst) {
            worst = delays[i];
            worstpos = i;
        }
    }
    ctx.stroke();

    ctx.fillStyle = 'rgb(255,0,0)';
    if (worst)
        ctx.fillText(''+worst+'ms', xpos(worstpos) - 10, ypos(worst) - 14);

    ctx.beginPath();
    var where = sampleIndex % numSamples;
    ctx.arc(xpos(where), ypos(delays[where]), 5, 0, Math.PI*2, true);
    ctx.fill();
    ctx.fillStyle = 'rgb(0,0,0)';

    ctx.fillText('Time', 550, 420);
    ctx.save();
    ctx.rotate(Math.PI/2);
    ctx.fillText('Pause between frames (log scale)', 150, -1060);
    ctx.restore();
}

function stopstart()
{
    if (stopped) {
        window.requestAnimationFrame(handler);
        prev = performance.now();
        start += prev - stopped;
        document.getElementById('stop').value = 'Pause';
        stopped = 0;
    } else {
        document.getElementById('stop').value = 'Resume';
        stopped = performance.now();
    }
}

function handler(timestamp)
{
    if (stopped)
        return;

    if (testState === 'running' && (timestamp - testStart) > testDuration)
        end_test(timestamp);

    activeTest.makeGarbage(garbagePerFrame);

    var elt = document.getElementById('data');
    var delay = timestamp - prev;
    prev = timestamp;

    
    
    update_histogram(gHistogram, Math.round(delay * 100));

    var t = timestamp - start;
    var newIndex = Math.round(t / sampleTime);
    while (sampleIndex < newIndex) {
        sampleIndex++;
        delays[sampleIndex % numSamples] = delay;
    }

    drawGraph();
    window.requestAnimationFrame(handler);
}

function update_histogram(histogram, delay)
{
    var current = histogram.has(delay) ? histogram.get(delay) : 0;
    histogram.set(delay, ++current);
}

function reset_draw_state()
{
    for (var i = 0; i < numSamples; i++)
        delays[i] = 0;
    start = prev = performance.now();
    sampleIndex = 0;
}

function onunload()
{
    if (activeTest)
        activeTest.unload();
    activeTest = undefined;
}

function onload()
{
    
    duration_changed();

    
    garbage_total_changed();
    garbage_per_frame_changed();

    
    var select = document.getElementById("test-selection");
    for (var [name, test] of tests) {
        test.name = name;
        var option = document.createElement("option");
        option.id = name;
        option.text = name;
        option.title = test.description;
        select.add(option);
    }

    
    activeTest = tests.get('noAllocation');
    activeTest.load(garbageTotal);

    
    var requestAnimationFrame =
        window.requestAnimationFrame || window.mozRequestAnimationFrame ||
        window.webkitRequestAnimationFrame || window.msRequestAnimationFrame;
    window.requestAnimationFrame = requestAnimationFrame;

    
    var canvas = document.getElementById('graph');
    ctx = canvas.getContext('2d');

    
    reset_draw_state();
    window.requestAnimationFrame(handler);
}

function run_one_test()
{
    start_test_cycle([activeTest.name]);
}

function run_all_tests()
{
    start_test_cycle(tests.keys());
}

function start_test_cycle(tests_to_run)
{
    
    testQueue = [];
    for (var key of tests_to_run)
        testQueue.push(key);
    testState = 'running';
    testStart = performance.now();
    gHistogram.clear();

    change_active_test(testQueue.pop());
    console.log(`Running test: ${activeTest.name}`);
    reset_draw_state();
}

function end_test(timestamp)
{
    report_test_result(activeTest, gHistogram);
    gHistogram.clear();
    console.log(`Ending test ${activeTest.name}`);
    if (testQueue.length) {
        change_active_test(testQueue.pop());
        console.log(`Running test: ${activeTest.name}`);
        testStart = timestamp;
    } else {
        testState = 'idle';
        testStart = 0;
    }
    reset_draw_state();
}

function report_test_result(test, histogram)
{
    var resultList = document.getElementById('results-display');
    var resultElem = document.createElement("div");
    var score = compute_test_score(histogram);
    var sparks = compute_test_spark_histogram(histogram);
    resultElem.innerHTML = `${score} ms/s : ${sparks} : ${test.name} - ${test.description}`;
    resultList.appendChild(resultElem);
}


function compute_test_score(histogram)
{
    var score = 0;
    for (var [delay, count] of histogram) {
        delay = delay / 100;
        score += Math.abs((delay - 16.66) * count);
    }
    score = score / (testDuration / 1000);
    return Math.round(score * 1000) / 1000;
}


function compute_test_spark_histogram(histogram)
{
    var ranges = [
        [-99999999, 16.6],
        [16.6, 16.8],
        [16.8, 25],
        [25, 33.4],
        [33.4, 60],
        [60, 100],
        [100, 300],
        [300, 99999999],
    ];
    var rescaled = new Map();
    for (var [delay, count] of histogram) {
        delay = delay / 100;
        for (var i = 0; i < ranges.length; ++i) {
            var low = ranges[i][0];
            var high = ranges[i][1];
            if (low <= delay && delay < high) {
                update_histogram(rescaled, i);
                break;
            }
        }
    }
    var total = 0;
    for (var [i, count] of rescaled)
        total += count;
    var sparks = "▁▂▃▄▅▆▇█";
    var colors = ['#aaaa00', '#007700', '#dd0000', '#ff0000',
                  '#ff0000', '#ff0000', '#ff0000', '#ff0000'];
    var line = "";
    for (var i = 0; i < ranges.length; ++i) {
        var amt = rescaled.has(i) ? rescaled.get(i) : 0;
        var spark = sparks.charAt(parseInt(amt/total*8));
        line += `<span style="color:${colors[i]}">${spark}</span>`;
    }
    return line;
}

function reload_active_test()
{
    activeTest.unload();
    activeTest.load(garbageTotal);
}

function change_active_test(new_test_name)
{
    activeTest.unload();
    activeTest = tests.get(new_test_name);
    activeTest.load(garbageTotal);
}

function duration_changed()
{
    var durationInput = document.getElementById('test-duration');
    testDuration = parseInt(durationInput.value) * 1000;
    console.log(`Updated test duration to: ${testDuration / 1000} seconds`);
}

function testchanged()
{
    var select = document.getElementById("test-selection");
    console.log(`Switching to test: ${select.value}`);
    change_active_test(select.value);
    gHistogram.clear();
    reset_draw_state();
}

function parse_units(v)
{
    if (v.length == 0)
        return NaN;
    var lastChar = v[v.length - 1].toLowerCase();
    if (!isNaN(parseFloat(lastChar)))
        return parseFloat(v);
    var units = parseFloat(v.substr(0, v.length - 1));
    if (lastChar == "k")
        return units * 1e3;
    if (lastChar == "m")
        return units * 1e6;
    if (lastChar == "g")
        return units * 1e9;
    return NaN;
}

function garbage_total_changed()
{
    var value = parse_units(document.getElementById('garbage-total').value);
    if (isNaN(value))
        return;
    garbageTotal = value;
    console.log(`Updated garbage-total to ${garbageTotal} items`);
    if (activeTest)
        reload_active_test();
    gHistogram.clear();
    reset_draw_state();
}

function garbage_per_frame_changed()
{
    var value = parse_units(document.getElementById('garbage-per-frame').value);
    if (isNaN(value))
        return;
    garbagePerFrame = value;
    console.log(`Updated garbage-per-frame to ${garbagePerFrame} items`);
}
