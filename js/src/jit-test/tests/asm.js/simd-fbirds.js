





load(libdir + "asm.js");
if (!isSimdAvailable() || typeof SIMD === 'undefined') {
    print("won't run tests as simd extensions aren't activated yet");
    quit(0);
}

const NUM_BIRDS = 30;
const NUM_UPDATES = 20;
const ACCEL_DATA_STEPS = 30;

var buffer = new ArrayBuffer(0x200000);
var bufferF32 = new Float32Array(buffer);

var actualBirds = 0;

function init() {
    actualBirds = 0;
    
    var accelDataValues = [10.0, 9.5, 9.0, 8.0, 7.0, 6.0, 5.5, 5.0, 5.0, 5.0, 5.5, 6.0, 7.0, 8.0, 9.0, 10.0];
    accelDataValues = accelDataValues.map(function(v) { return 50*v; });
    var accelDataValuesLength = accelDataValues.length;
    assertEq(accelDataValuesLength, 16); 
    for (i = 0; i < accelDataValuesLength; i++)
        bufferF32[i + NUM_BIRDS * 2] = accelDataValues[i];
}

function addBird(pos, vel) {
    bufferF32[actualBirds] = pos;
    bufferF32[actualBirds + NUM_BIRDS] = vel;
    actualBirds++;
    return actualBirds - 1;
}

function getActualBirds() {
    return actualBirds;
}

var code = `
    "use asm";
    var toF = global.Math.fround;
    var f32 = new global.Float32Array(buffer);
    const maxBirds = 100000;
    const maxBirdsx4 = 400000;
    const maxBirdsx4Plus4 = 400004;
    const maxBirdsx4Plus8 = 400008;
    const maxBirdsx4Plus12 = 400012;
    const maxBirdsx8 = 800000;
    const accelMask = 0x3c;
    const mk2 = 0x000ffffc;

    const getMaxPos = 1000.0;
    const getAccelDataSteps = imp.accelDataSteps | 0;
    var getActualBirds = imp.getActualBirds;

    var i4 = global.SIMD.int32x4;
    var f4 = global.SIMD.float32x4;
    var i4add = i4.add;
    var i4and = i4.and;
    var f4select = f4.select;
    var f4add = f4.add;
    var f4sub = f4.sub;
    var f4mul = f4.mul;
    var f4greaterThan = f4.greaterThan;
    var f4splat = f4.splat;

    const zerox4 = f4(0.0,0.0,0.0,0.0);

    function declareHeapSize() {
        f32[0x0007ffff] = toF(0.0);
    }

    function update(timeDelta) {
        timeDelta = toF(timeDelta);
        //      var steps               = Math.ceil(timeDelta/accelData.interval);
        var steps = 0;
        var subTimeDelta = toF(0.0);
        var actualBirds = 0;
        var maxPos = toF(0.0);
        var maxPosx4 = f4(0.0,0.0,0.0,0.0);
        var subTimeDeltax4  = f4(0.0,0.0,0.0,0.0);
        var subTimeDeltaSquaredx4 = f4(0.0,0.0,0.0,0.0);
        var point5x4 = f4(0.5, 0.5, 0.5, 0.5);
        var i = 0;
        var len = 0;
        var accelIndex = 0;
        var newPosx4 = f4(0.0,0.0,0.0,0.0);
        var newVelx4 = f4(0.0,0.0,0.0,0.0);
        var accel = toF(0.0);
        var accelx4 = f4(0.0,0.0,0.0,0.0);
        var a = 0;
        var posDeltax4 = f4(0.0,0.0,0.0,0.0);
        var cmpx4 = i4(0,0,0,0);
        var newVelTruex4 = f4(0.0,0.0,0.0,0.0);

        steps = getAccelDataSteps | 0;
        subTimeDelta = toF(toF(timeDelta / toF(steps | 0)) / toF(1000.0));
        actualBirds = getActualBirds() | 0;
        maxPos = toF(+getMaxPos);
        maxPosx4 = f4splat(maxPos);
        subTimeDeltax4 = f4splat(subTimeDelta);
        subTimeDeltaSquaredx4 = f4mul(subTimeDeltax4, subTimeDeltax4);

        len = ((actualBirds + 3) >> 2) << 4;

        for (i = 0; (i | 0) < (len | 0); i = (i + 16) | 0) {
            accelIndex = 0;
            // Work around unimplemented Float32x4Array
            newPosx4 = f4(toF(f32[(i & mk2) >> 2]),
                    toF(f32[(i & mk2) + 4 >> 2]),
                    toF(f32[(i & mk2) + 8 >> 2]),
                    toF(f32[(i & mk2) + 12 >> 2]));
            newVelx4 = f4(toF(f32[(i & mk2) + maxBirdsx4 >> 2]),
                    toF(f32[(i & mk2) + maxBirdsx4Plus4 >> 2]),
                    toF(f32[(i & mk2) + maxBirdsx4Plus8 >> 2]),
                    toF(f32[(i & mk2) + maxBirdsx4Plus12 >> 2]));
            for (a = 0; (a | 0) < (steps | 0); a = (a + 1) | 0) {
                accel = toF(f32[(accelIndex & accelMask) + maxBirdsx8 >> 2]);
                accelx4 = f4splat(accel);
                accelIndex = (accelIndex + 4) | 0;
                posDeltax4 = f4mul(point5x4, f4mul(accelx4, subTimeDeltaSquaredx4));
                posDeltax4 = f4add(posDeltax4, f4mul(newVelx4, subTimeDeltax4));
                newPosx4 = f4add(newPosx4, posDeltax4);
                newVelx4 = f4add(newVelx4, f4mul(accelx4, subTimeDeltax4));
                cmpx4 = f4greaterThan(newPosx4, maxPosx4);

                if (cmpx4.signMask) {
                    // Work around unimplemented 'neg' operation, using 0 - x.
                    newVelTruex4 = f4sub(zerox4, newVelx4);
                    newVelx4 = f4select(cmpx4, newVelTruex4, newVelx4);
                }
            }
            // Work around unimplemented Float32x4Array
            f32[(i & mk2) >> 2] = newPosx4.x;
            f32[(i & mk2) + 4 >> 2] = newPosx4.y;
            f32[(i & mk2) + 8 >> 2] = newPosx4.z;
            f32[(i & mk2) + 12 >> 2] = newPosx4.w;
            f32[(i & mk2) + maxBirdsx4 >> 2] = newVelx4.x;
            f32[(i & mk2) + maxBirdsx4Plus4 >> 2] = newVelx4.y;
            f32[(i & mk2) + maxBirdsx4Plus8 >> 2] = newVelx4.z;
            f32[(i & mk2) + maxBirdsx4Plus12 >> 2] = newVelx4.w;
        }
    }

    return update;
`

var ffi = {
    getActualBirds,
    accelDataSteps: ACCEL_DATA_STEPS
};

var fbirds = asmLink(asmCompile('global', 'imp', 'buffer', code), this, ffi, buffer);

init();
for (var i = 0; i < NUM_BIRDS; i++) {
    addBird(i / 10, Math.exp(2, NUM_BIRDS - i));
}

var b = dateNow();
for (var j = 0; j < NUM_UPDATES; j++) {
    fbirds(16);
}
print(dateNow() - b);

assertEq(bufferF32[0], 0);
assertEq(bufferF32[1], 0.10000000149011612);
assertEq(bufferF32[2], 0.20000000298023224);
assertEq(bufferF32[3], 0.30000001192092896);
assertEq(bufferF32[4], 0.4000000059604645);
assertEq(bufferF32[5], 0.5);
assertEq(bufferF32[6], 0.6000000238418579);
assertEq(bufferF32[7], 0.699999988079071);
assertEq(bufferF32[8], 0.800000011920929);
assertEq(bufferF32[9], 0.8999999761581421);
assertEq(bufferF32[10], 1);
assertEq(bufferF32[11], 1.100000023841858);
assertEq(bufferF32[12], 1.2000000476837158);
assertEq(bufferF32[13], 1.2999999523162842);
assertEq(bufferF32[14], 1.399999976158142);
assertEq(bufferF32[15], 1.5);
assertEq(bufferF32[16], 1.600000023841858);
assertEq(bufferF32[17], 1.7000000476837158);
assertEq(bufferF32[18], 1.7999999523162842);
assertEq(bufferF32[19], 1.899999976158142);
assertEq(bufferF32[20], 2);
assertEq(bufferF32[21], 2.0999999046325684);
assertEq(bufferF32[22], 2.200000047683716);
assertEq(bufferF32[23], 2.299999952316284);
assertEq(bufferF32[24], 2.4000000953674316);
assertEq(bufferF32[25], 2.5);
assertEq(bufferF32[26], 2.5999999046325684);
assertEq(bufferF32[27], 2.700000047683716);
assertEq(bufferF32[28], 2.799999952316284);
assertEq(bufferF32[29], 2.9000000953674316);



function generateAssertList() {
    var buf = '';
    for (var k = 0; k < NUM_BIRDS; k++) {
        buf += 'assertEq(bufferF32['+ k + '], ' + bufferF32[k] + ');\n';
    }
    print(buf);
}

