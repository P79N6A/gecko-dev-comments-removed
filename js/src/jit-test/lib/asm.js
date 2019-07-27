



const ASM_OK_STRING = "successfully compiled asm.js code";
const ASM_TYPE_FAIL_STRING = "asm.js type error:";
const ASM_DIRECTIVE_FAIL_STRING = "\"use asm\" is only meaningful in the Directive Prologue of a function body";

const USE_ASM = '"use asm";';
const HEAP_IMPORTS = "const i8=new glob.Int8Array(b);var u8=new glob.Uint8Array(b);"+
                     "const i16=new glob.Int16Array(b);var u16=new glob.Uint16Array(b);"+
                     "const i32=new glob.Int32Array(b);var u32=new glob.Uint32Array(b);"+
                     "const f32=new glob.Float32Array(b);var f64=new glob.Float64Array(b);";
const BUF_MIN = 64 * 1024;
const BUF_64KB = new ArrayBuffer(BUF_MIN);

function asmCompile()
{
    var f = Function.apply(null, arguments);
    assertEq(!isAsmJSCompilationAvailable() || isAsmJSModule(f), true);
    return f;
}

function asmCompileCached()
{
    if (!isAsmJSCompilationAvailable())
        return Function.apply(null, arguments);

    if (!isCachingEnabled()) {
        var f = Function.apply(null, arguments);
        assertEq(isAsmJSModule(f), true);
        return f;
    }

    var quotedArgs = [];
    for (var i = 0; i < arguments.length; i++)
        quotedArgs.push("'" + arguments[i] + "'");
    var code = "setCachingEnabled(true); var f = new Function(" + quotedArgs.join(',') + ");assertEq(isAsmJSModule(f), true);";
    nestedShell("--js-cache", "--no-js-cache-per-process", "--execute=" + code);

    var f = Function.apply(null, arguments);
    assertEq(isAsmJSModuleLoadedFromCache(f), true);
    return f;
}

function assertAsmDirectiveFail(str)
{
    if (!isAsmJSCompilationAvailable())
        return;

    
    var oldOpts = options("werror");
    assertEq(oldOpts.indexOf("werror"), -1);

    
    var caught = false;
    try {
        eval(str);
    } catch (e) {
        if ((''+e).indexOf(ASM_DIRECTIVE_FAIL_STRING) == -1)
            throw new Error("Didn't catch the expected directive failure error; instead caught: " + e + "\nStack: " + new Error().stack);
        caught = true;
    }
    if (!caught)
        throw new Error("Didn't catch the directive failure error");

    
    options("werror");
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
            throw new Error("Didn't catch the expected type failure error; instead caught: " + e + "\nStack: " + new Error().stack);
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

    assertEq(isAsmJSModule(f), true);

    
    var ret = f.apply(null, Array.slice(arguments, 1));

    assertEq(isAsmJSFunction(ret), false);
    if (typeof ret === 'object')
        for (var i in ret)
            assertEq(isAsmJSFunction(ret[i]), false);

    
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

function assertAsmLinkDeprecated(f)
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
