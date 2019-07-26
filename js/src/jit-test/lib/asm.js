



const ASM_OK_STRING = "Successfully compiled asm.js code";
const ASM_TYPE_FAIL_STRING = "asm.js type error:";

const USE_ASM = "'use asm';";
const HEAP_IMPORTS = "var i8=new glob.Int8Array(b);var u8=new glob.Uint8Array(b);"+
                     "var i16=new glob.Int16Array(b);var u16=new glob.Uint16Array(b);"+
                     "var i32=new glob.Int32Array(b);var u32=new glob.Uint32Array(b);"+
                     "var f32=new glob.Float32Array(b);var f64=new glob.Float64Array(b);";
const BUF_64KB = new ArrayBuffer(64 * 1024);

function asmCompile()
{
    if (!isAsmJSCompilationAvailable())
        return Function.apply(null, arguments);

    

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    
    var caught = false;
    try {
        Function.apply(null, arguments);
    } catch (e) {
        if ((''+e).indexOf(ASM_OK_STRING) == -1)
            throw new Error("Didn't catch the expected success error; instead caught: " + e);
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the success error");

    
    options("werror");

    
    return Function.apply(null, arguments);
}

function assertAsmTypeFail()
{
    if (!isAsmJSCompilationAvailable())
        return;

    
    Function.apply(null, arguments);

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    
    var caught = false;
    try {
        Function.apply(null, arguments);
    } catch (e) {
        if ((''+e).indexOf(ASM_TYPE_FAIL_STRING) == -1)
            throw new Error("Didn't catch the expected type failure error; instead caught: " + e);
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the type failure error");

    
    options("werror");
}

function assertAsmLinkFail(f)
{
    if (!isAsmJSCompilationAvailable())
        return;

    
    f.apply(null, Array.slice(arguments, 1));

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    
    var caught = false;
    try {
        f.apply(null, Array.slice(arguments, 1));
    } catch (e) {
        
        
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the link failure error");

    
    options("werror");
}


function assertAsmLinkAlwaysFail(f)
{
    var caught = false;
    try {
        f.apply(null, Array.slice(arguments, 1));
    } catch (e) {
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the link failure error");

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    
    var caught = false;
    try {
        f.apply(null, Array.slice(arguments, 1));
    } catch (e) {
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the link failure error");

    
    options("werror");
}


function asmLink(f)
{
    if (!isAsmJSCompilationAvailable())
        return f.apply(null, Array.slice(arguments, 1));

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    var ret = f.apply(null, Array.slice(arguments, 1));

    
    options("werror");

    return ret;
}
