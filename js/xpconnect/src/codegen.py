





































import xpidl
import header
import sys
import os
import string



def warn(msg):
    sys.stderr.write(msg + '\n')

def makeQuote(filename):
    return filename.replace(' ', '\\ ')  



def isVoidType(type):
    """ Return True if the given xpidl type is void. """
    return type.kind == 'builtin' and type.name == 'void'

def isStringType(t):
    t = xpidl.unaliasType(t)
    return t.kind == 'native' and (t.specialtype == 'astring' or t.specialtype == 'domstring');

def isInterfaceType(t):
    t = xpidl.unaliasType(t)
    assert t.kind in ('builtin', 'native', 'interface', 'forward')
    return t.kind in ('interface', 'forward')

def isSpecificInterfaceType(t, name):
    """ True if `t` is an interface type with the given name, or a forward
    declaration or typedef aliasing it.

    `name` must not be the name of a typedef but the actual name of the
    interface.
    """
    t = xpidl.unaliasType(t)
    return t.kind in ('interface', 'forward') and t.name == name

def isVariantType(t):
    return isSpecificInterfaceType(t, 'nsIVariant')



class UserError(Exception):
    pass

def findIDL(includePath, irregularFilenames, interfaceName):
    filename = irregularFilenames.get(interfaceName, interfaceName) + '.idl'
    for d in includePath:
        
        
        path = d + '/' + filename
        if os.path.exists(path):
            return path
    raise UserError("No IDL file found for interface %s "
                    "in include path %r"
                    % (interfaceName, includePath))



argumentUnboxingTemplates = {
    'octet':
        "    uint32 ${name}_u32;\n"
        "    if (!JS_ValueToECMAUint32(cx, ${argVal}, &${name}_u32))\n"
        "        return JS_FALSE;\n"
        "    uint8 ${name} = (uint8) ${name}_u32;\n",

    'short':
        "    int32 ${name}_i32;\n"
        "    if (!JS_ValueToECMAInt32(cx, ${argVal}, &${name}_i32))\n"
        "        return JS_FALSE;\n"
        "    int16 ${name} = (int16) ${name}_i32;\n",

    'unsigned short':
        "    uint32 ${name}_u32;\n"
        "    if (!JS_ValueToECMAUint32(cx, ${argVal}, &${name}_u32))\n"
        "        return JS_FALSE;\n"
        "    uint16 ${name} = (uint16) ${name}_u32;\n",

    'long':
        "    int32 ${name};\n"
        "    if (!JS_ValueToECMAInt32(cx, ${argVal}, &${name}))\n"
        "        return JS_FALSE;\n",

    'unsigned long':
        "    uint32 ${name};\n"
        "    if (!JS_ValueToECMAUint32(cx, ${argVal}, &${name}))\n"
        "        return JS_FALSE;\n",

    'long long':
        "    PRInt64 ${name};\n"
        "    if (!xpc_qsValueToInt64(cx, ${argVal}, &${name}))\n"
        "        return JS_FALSE;\n",

    'unsigned long long':
        "    PRUint64 ${name};\n"
        "    if (!xpc_qsValueToUint64(cx, ${argVal}, &${name}))\n"
        "        return JS_FALSE;\n",

    'float':
        "    jsdouble ${name}_dbl;\n"
        "    if (!JS_ValueToNumber(cx, ${argVal}, &${name}_dbl))\n"
        "        return JS_FALSE;\n"
        "    float ${name} = (float) ${name}_dbl;\n",

    'double':
        "    jsdouble ${name};\n"
        "    if (!JS_ValueToNumber(cx, ${argVal}, &${name}))\n"
        "        return JS_FALSE;\n",

    'boolean':
        "    bool ${name};\n"
        "    JS_ValueToBoolean(cx, ${argVal}, &${name});\n",

    '[astring]':
        "    xpc_qsAString ${name}(cx, ${argVal}, ${argPtr});\n"
        "    if (!${name}.IsValid())\n"
        "        return JS_FALSE;\n",

    '[domstring]':
        "    xpc_qsDOMString ${name}(cx, ${argVal}, ${argPtr}, "
        "xpc_qsDOMString::e${nullBehavior}, "
        "xpc_qsDOMString::e${undefinedBehavior});\n"
        "    if (!${name}.IsValid())\n"
        "        return JS_FALSE;\n",

    'string':
        "    JSAutoByteString ${name}_bytes;\n"
        "    if (!xpc_qsJsvalToCharStr(cx, ${argVal}, &${name}_bytes))\n"
        "        return JS_FALSE;\n"
        "    char *${name} = ${name}_bytes.ptr();\n",

    'wstring':
        "    const PRUnichar *${name};\n"
        "    if (!xpc_qsJsvalToWcharStr(cx, ${argVal}, ${argPtr}, &${name}))\n"
        "        return JS_FALSE;\n",

    '[cstring]':
        "    xpc_qsACString ${name}(cx, ${argVal}, ${argPtr});\n"
        "    if (!${name}.IsValid())\n"
        "        return JS_FALSE;\n",

    '[utf8string]':
        "    xpc_qsAUTF8String ${name}(cx, ${argVal}, ${argPtr});\n"
        "    if (!${name}.IsValid())\n"
        "        return JS_FALSE;\n",

    '[jsval]':
        "    jsval ${name} = ${argVal};\n"
    }







def writeArgumentUnboxing(f, i, name, type, haveCcx, optional, rvdeclared,
                          nullBehavior, undefinedBehavior):
    
    
    
    
    
    
    
    
    

    typeName = xpidl.getBuiltinOrNativeTypeName(type)

    isSetter = (i is None)

    if isSetter:
        argPtr = "vp"
        argVal = "*vp"
    elif optional:
        if typeName == "[jsval]":
            val = "JSVAL_VOID"
        else:
            val = "JSVAL_NULL"
        argVal = "(%d < argc ? argv[%d] : %s)" % (i, i, val)
        argPtr = "(%d < argc ? &argv[%d] : NULL)" % (i, i)
    else:
        argVal = "argv[%d]" % i
        argPtr = "&" + argVal

    params = {
        'name': name,
        'argVal': argVal,
        'argPtr': argPtr,
        'nullBehavior': nullBehavior or 'DefaultNullBehavior',
        'undefinedBehavior': undefinedBehavior or 'DefaultUndefinedBehavior'
        }

    if typeName is not None:
        template = argumentUnboxingTemplates.get(typeName)
        if template is not None:
            f.write(string.Template(template).substitute(params))
            return rvdeclared
        
    elif isInterfaceType(type):
        if type.name == 'nsIVariant':
            
            assert haveCcx
            template = (
                "    nsCOMPtr<nsIVariant> ${name}(already_AddRefed<nsIVariant>("
                "XPCVariant::newVariant(ccx, ${argVal})));\n"
                "    if (!${name}) {\n"
                "        xpc_qsThrowBadArgWithCcx(ccx, NS_ERROR_XPC_BAD_CONVERT_JS, %d);\n"
                "        return JS_FALSE;\n"
                "    }\n") % i
            f.write(string.Template(template).substitute(params))
            return rvdeclared
        elif type.name == 'nsIAtom':
            
            pass
        else:
            if not rvdeclared:
                f.write("    nsresult rv;\n");
            f.write("    %s *%s;\n" % (type.name, name))
            f.write("    xpc_qsSelfRef %sref;\n" % name)
            f.write("    rv = xpc_qsUnwrapArg<%s>("
                    "cx, %s, &%s, &%sref.ptr, %s);\n"
                    % (type.name, argVal, name, name, argPtr))
            f.write("    if (NS_FAILED(rv)) {\n")
            if isSetter:
                f.write("        xpc_qsThrowBadSetterValue("
                        "cx, rv, JSVAL_TO_OBJECT(*tvr.jsval_addr()), id);\n")
            elif haveCcx:
                f.write("        xpc_qsThrowBadArgWithCcx(ccx, rv, %d);\n" % i)
            else:
                f.write("        xpc_qsThrowBadArg(cx, rv, vp, %d);\n" % i)
            f.write("        return JS_FALSE;\n"
                    "    }\n")
            return True

    warn("Unable to unbox argument of type %s (native type %s)" % (type.name, typeName))
    if i is None:
        src = '*vp'
    else:
        src = 'argv[%d]' % i
    f.write("    !; // TODO - Unbox argument %s = %s\n" % (name, src))
    return rvdeclared

def writeResultDecl(f, member, varname):
    type = member.realtype

    if isVoidType(type):
        return  

    t = xpidl.unaliasType(type)
    if t.kind == 'builtin':
        if not t.nativename.endswith('*'):
            if type.kind == 'typedef':
                typeName = type.name  
            else:
                typeName = t.nativename
            f.write("    %s %s;\n" % (typeName, varname))
            return
    elif t.kind == 'native':
        name = xpidl.getBuiltinOrNativeTypeName(t)
        if name in ('[domstring]', '[astring]'):
            f.write("    nsString %s;\n" % varname)
            return
        elif name == '[jsval]':
            return  
    elif t.kind in ('interface', 'forward'):
        if member.kind == 'method' and member.notxpcom:
            f.write("    %s *%s;\n" % (type.name, varname))
        else:
            f.write("    nsCOMPtr<%s> %s;\n" % (type.name, varname))
        return

    warn("Unable to declare result of type %s" % type.name)
    f.write("    !; // TODO - Declare out parameter `%s`.\n" % varname)

def outParamForm(name, type):
    type = xpidl.unaliasType(type)
    
    
    
    assert type.kind != 'native' or type.specialtype != 'jsval'
    if type.kind == 'builtin':
        return '&' + name
    elif type.kind == 'native':
        if type.specialtype == 'jsval':
            return 'vp'
        elif type.modifier == 'ref':
            return name
        else:
            return '&' + name
    else:
        return 'getter_AddRefs(%s)' % name


resultConvTemplates = {
    'void':
            "    ${jsvalRef} = JSVAL_VOID;\n"
            "    return JS_TRUE;\n",

    'octet':
        "    ${jsvalRef} = INT_TO_JSVAL((int32) result);\n"
        "    return JS_TRUE;\n",

    'short':
        "    ${jsvalRef} = INT_TO_JSVAL((int32) result);\n"
        "    return JS_TRUE;\n",

    'long':
        "    return xpc_qsInt32ToJsval(cx, result, ${jsvalPtr});\n",

    'long long':
        "    return xpc_qsInt64ToJsval(cx, result, ${jsvalPtr};\n",

    'unsigned short':
        "    ${jsvalRef} = INT_TO_JSVAL((int32) result);\n"
        "    return JS_TRUE;\n",

    'unsigned long':
        "    return xpc_qsUint32ToJsval(cx, result, ${jsvalPtr});\n",

    'unsigned long long':
        "    return xpc_qsUint64ToJsval(cx, result, ${jsvalPtr});\n",

    'float':
        "    return JS_NewNumberValue(cx, result, ${jsvalPtr});\n",

    'double':
        "    return JS_NewNumberValue(cx, result, ${jsvalPtr});\n",

    'boolean':
        "    ${jsvalRef} = (result ? JSVAL_TRUE : JSVAL_FALSE);\n"
        "    return JS_TRUE;\n",

    '[astring]':
        "    return xpc_qsStringToJsval(cx, result, ${jsvalPtr});\n",

    '[domstring]':
        "    return xpc_qsStringToJsval(cx, result, ${jsvalPtr});\n",

    '[jsval]':
        
        
        "    return JS_TRUE;\n"
    }

def writeResultConv(f, type, interfaceResultTemplate, jsvalPtr, jsvalRef):
    """ Emit code to convert the C++ variable `result` to a jsval.

    The emitted code contains a return statement; it returns JS_TRUE on
    success, JS_FALSE on error.
    """
    
    typeName = xpidl.getBuiltinOrNativeTypeName(type)
    if typeName is not None:
        template = resultConvTemplates.get(typeName)
    elif isInterfaceType(type):
        if isVariantType(type):
            template = "    return xpc_qsVariantToJsval(lccx, result, ${jsvalPtr});\n"
        else:
            template = ("    if (!result) {\n"
                        "      *${jsvalPtr} = JSVAL_NULL;\n"
                        "      return JS_TRUE;\n"
                        "    }\n")
            template += interfaceResultTemplate

    if template is not None:
        values = {'jsvalRef': jsvalRef,
                  'jsvalPtr': jsvalPtr,
                  'typeName': type.name}
        f.write(string.Template(template).substitute(values))
        return
    

    warn("Unable to convert result of type %s" % type.name)
    f.write("    !; // TODO - Convert `result` to jsval, store in `%s`.\n"
            % jsvalRef)
    f.write("    return xpc_qsThrow(cx, NS_ERROR_UNEXPECTED); // FIXME\n")

def anyParamRequiresCcx(member):
    for p in member.params:
        if isVariantType(p.realtype):
            return True
    return False

def memberNeedsCcx(member):
    return member.kind == 'method' and anyParamRequiresCcx(member)

def validateParam(member, param):
    def pfail(msg):
        raise UserError(
            member.iface.name + '.' + member.name + ": "
            "parameter " + param.name + ": " + msg)

    if param.iid_is is not None:
        pfail("iid_is parameters are not supported.")
    if param.size_is is not None:
        pfail("size_is parameters are not supported.")
    if param.retval:
        pfail("Unexpected retval parameter!")
    if param.paramtype in ('out', 'inout'):
        pfail("inout parameters are not supported.")
    if param.const or param.array or param.shared:
        pfail("I am a simple caveman.")


def argumentsLength(member):
    assert member.kind == 'method'

    inArgs = len(member.params)
    if inArgs and member.notxpcom and member.params[inArgs - 1].paramtype == 'out':
        if member.params[inArgs - 1].realtype.kind != 'native' or member.params[inArgs - 1].realtype.nativename != 'nsWrapperCache':
            pfail("We only support a wrapper cache as out argument")
        inArgs -= 1
    return inArgs

def writeStub(f, customMethodCalls, member, stubName, writeThisUnwrapping, writeCheckForFailure, writeResultWrapping, isSetter=False):
    """ Write a single quick stub (a custom SpiderMonkey getter/setter/method)
    for the specified XPCOM interface-member. 
    """
    if member.kind == 'method' and member.forward:
        member = member.iface.namemap[member.forward]

    isAttr = (member.kind == 'attribute')
    isMethod = (member.kind == 'method')
    assert isAttr or isMethod
    isNotxpcom = isMethod and member.notxpcom
    isGetter = isAttr and not isSetter

    signature = "static JSBool\n"
    if isAttr:
        
        if isSetter:
            signature += "%s(JSContext *cx, JSObject *obj, jsid id, JSBool strict,%s jsval *vp)\n"
        else:
            signature += "%s(JSContext *cx, JSObject *obj, jsid id,%s jsval *vp)\n"
    else:
        
        signature += "%s(JSContext *cx, uintN argc,%s jsval *vp)\n"

    customMethodCall = customMethodCalls.get(stubName, None)

    if customMethodCall is None:
        customMethodCall = customMethodCalls.get(member.iface.name + '_', None)
        if customMethodCall is not None:
            if isMethod:
                code = customMethodCall.get('code', None)
            elif isGetter:
                code = customMethodCall.get('getter_code', None)
            else:
                code = customMethodCall.get('setter_code', None)
        else:
            code = None

        if code is not None:
            templateName = member.iface.name
            if isGetter:
                templateName += '_Get'
            elif isSetter:
                templateName += '_Set'

            
            
            
            callTemplate = signature % (stubName, '')
            callTemplate += "{\n"

            argumentValues = (customMethodCall['additionalArgumentValues']
                              % header.methodNativeName(member))
            if isAttr:
                callTemplate += ("    return %s(cx, obj, id%s, %s, vp);\n"
                                 % (templateName, ", strict" if isSetter else "", argumentValues))
            else:
                callTemplate += ("    return %s(cx, argc, %s, vp);\n"
                                 % (templateName, argumentValues))
            callTemplate += "}\n\n"

            
            
            
            templateGenerated = templateName + '_generated'
            if templateGenerated in customMethodCall:
                f.write(callTemplate)
                return
            customMethodCall[templateGenerated] = True

            stubName = templateName
        else:
            callTemplate = ""
    else:
        callTemplate = ""
        code = customMethodCall.get('code', None)

    

    
    if customMethodCall is None or not 'additionalArguments' in customMethodCall:
        additionalArguments = ''
    else:
        additionalArguments = " %s," % customMethodCall['additionalArguments']
    f.write(signature % (stubName, additionalArguments))
    f.write("{\n")
    f.write("    XPC_QS_ASSERT_CONTEXT_OK(cx);\n")

    
    if isMethod:
        f.write("    JSObject *obj = JS_THIS_OBJECT(cx, vp);\n"
                "    if (!obj)\n"
                "        return JS_FALSE;\n")

    
    haveCcx = memberNeedsCcx(member)
    if haveCcx:
        f.write("    XPCCallContext ccx(JS_CALLER, cx, obj, "
                "JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));\n")
        if isInterfaceType(member.realtype):
            f.write("    XPCLazyCallContext lccx(ccx);\n")

    selfname = writeThisUnwrapping(f, member, isMethod, isGetter, customMethodCall, haveCcx)

    rvdeclared = False
    if isMethod:
        inArgs = argumentsLength(member)
        
        requiredArgs = inArgs
        while requiredArgs and member.params[requiredArgs-1].optional:
            requiredArgs -= 1
        if requiredArgs:
            f.write("    if (argc < %d)\n" % requiredArgs)
            f.write("        return xpc_qsThrow(cx, "
                    "NS_ERROR_XPC_NOT_ENOUGH_ARGS);\n")

        
        if inArgs > 0:
            f.write("    jsval *argv = JS_ARGV(cx, vp);\n")
        for i in range(inArgs):
            param = member.params[i]
            argName = 'arg%d' % i
            argTypeKey = argName + 'Type'
            if customMethodCall is None or not argTypeKey in customMethodCall:
                validateParam(member, param)
                realtype = param.realtype
            else:
                realtype = xpidl.Forward(name=customMethodCall[argTypeKey],
                                         location='', doccomments='')
            
            rvdeclared = writeArgumentUnboxing(
                f, i, argName, realtype,
                haveCcx=haveCcx,
                optional=param.optional,
                rvdeclared=rvdeclared,
                nullBehavior=param.null,
                undefinedBehavior=param.undefined)
        if inArgs < len(member.params):
            f.write("    nsWrapperCache *cache;\n")
    elif isSetter:
        rvdeclared = writeArgumentUnboxing(f, None, 'arg0', member.realtype,
                                           haveCcx=False, optional=False,
                                           rvdeclared=rvdeclared,
                                           nullBehavior=member.null,
                                           undefinedBehavior=member.undefined)

    canFail = not isNotxpcom and (customMethodCall is None or customMethodCall.get('canFail', True))
    if canFail and not rvdeclared:
        f.write("    nsresult rv;\n")
        rvdeclared = True

    if code is not None:
        f.write("%s\n" % code)

    if code is None or (isGetter and callTemplate is ""):
        debugGetter = code is not None
        if debugGetter:
            f.write("#ifdef DEBUG\n")
            f.write("    nsresult debug_rv;\n")
            f.write("    nsCOMPtr<%s> debug_self;\n"
                    "    CallQueryInterface(self, getter_AddRefs(debug_self));\n"
                    % member.iface.name);
            prefix = 'debug_'
        else:
            prefix = ''

        resultname = prefix + 'result'
        selfname = prefix + selfname
        nsresultname = prefix + 'rv'

        
        if isMethod or isGetter:
            writeResultDecl(f, member, resultname)

        
        if isMethod:
            comName = header.methodNativeName(member)
            argv = ['arg' + str(i) for i in range(inArgs)]
            if inArgs < len(member.params):
                argv.append(outParamForm('cache', member.params[inArgs].realtype))
            if member.implicit_jscontext:
                argv.append('cx')
            if member.optional_argc:
                argv.append('argc - %d' % requiredArgs)
            if not isNotxpcom and not isVoidType(member.realtype):
                argv.append(outParamForm(resultname, member.realtype))
            args = ', '.join(argv)
        else:
            comName = header.attributeNativeName(member, isGetter)
            if isGetter:
                args = outParamForm(resultname, member.realtype)
            else:
                args = "arg0"
            if member.implicit_jscontext:
                args = "cx, " + args

        f.write("    ")
        if canFail or debugGetter:
            f.write("%s = " % nsresultname)
        elif isNotxpcom:
            f.write("%s = " % resultname)
        f.write("%s->%s(%s);\n" % (selfname, comName, args))

        if debugGetter:
            checkSuccess = "NS_SUCCEEDED(debug_rv)"
            if canFail:
                checkSuccess += " == NS_SUCCEEDED(rv)"
            f.write("    NS_ASSERTION(%s && "
                    "xpc_qsSameResult(debug_result, result),\n"
                    "                 \"Got the wrong answer from the custom "
                    "method call!\");\n" % checkSuccess)
            f.write("#endif\n")

    if canFail:
        
        writeCheckForFailure(f, isMethod, isGetter, haveCcx)

    
    if isMethod or isGetter:
        writeResultWrapping(f, member, 'vp', '*vp')
    else:
        f.write("    return JS_TRUE;\n")

    
    f.write("}\n\n")

    
    if customMethodCall is not None:
        f.write(callTemplate)
