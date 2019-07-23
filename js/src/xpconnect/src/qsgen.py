


































































































































import xpidl
import header
import os, re
import sys
import sets




make_dependencies = []
make_targets = []

def warn(msg):
    sys.stderr.write(msg + '\n')

def unaliasType(t):
    while t.kind == 'typedef':
        t = t.realtype
    assert t is not None
    return t

def isVoidType(type):
    """ Return True if the given xpidl type is void. """
    return type.kind == 'builtin' and type.name == 'void'

def isInterfaceType(t):
    t = unaliasType(t)
    assert t.kind in ('builtin', 'native', 'interface', 'forward')
    return t.kind in ('interface', 'forward')

def isSpecificInterfaceType(t, name):
    """ True if `t` is an interface type with the given name, or a forward
    declaration or typedef aliasing it.

    `name` must not be the name of a typedef but the actual name of the
    interface.
    """
    t = unaliasType(t)
    return t.kind in ('interface', 'forward') and t.name == name

def getBuiltinOrNativeTypeName(t):
    t = unaliasType(t)
    if t.kind == 'builtin':
        return t.name
    elif t.kind == 'native':
        assert t.specialtype is not None
        return '[%s]' % t.specialtype
    else:
        return None




class UserError(Exception):
    pass

def findIDL(includePath, irregularFilenames, interfaceName):
    filename = irregularFilenames.get(interfaceName, interfaceName) + '.idl'
    for d in includePath:
        path = os.path.join(d, filename)
        if os.path.exists(path):
            return path
    raise UserError("No IDL file found for interface %s "
                    "in include path %r"
                    % (interfaceName, includePath))

def loadIDL(parser, includePath, filename):
    make_dependencies.append(filename)
    text = open(filename, 'r').read()
    idl = parser.parse(text, filename=filename)
    idl.resolve(includePath, parser)
    return idl

def addStubMember(memberId, member):
    
    if member.kind not in ('method', 'attribute'):
        raise UserError("Member %s is %r, not a method or attribute."
                        % (memberId, member.kind))
    if member.noscript:
        raise UserError("%s %s is noscript."
                        % (member.kind.capitalize(), memberId))
    if member.notxpcom:
        raise UserError(
            "%s %s: notxpcom methods are not supported."
            % (member.kind.capitalize(), memberId))

    if (member.kind == 'attribute'
          and not member.readonly
          and isSpecificInterfaceType(member.realtype, 'nsIVariant')):
        raise UserError(
            "Attribute %s: Non-readonly attributes of type nsIVariant "
            "are not supported."
            % memberId)

    
    for attrname, value in vars(member).items():
        if value is True and attrname not in ('readonly',):
            raise UserError("%s %s: unrecognized property %r"
                            % (member.kind.capitalize(), memberId,
                               attrname))
    if member.kind == 'method':
        for param in member.params:
            for attrname, value in vars(param).items():
                if value is True and attrname not in ('optional',):
                    raise UserError("Method %s, parameter %s: "
                                    "unrecognized property %r"
                                    % (memberId, param.name, attrname))

    
    member.iface.stubMembers.append(member)

def parseMemberId(memberId):
    """ Split the geven member id into its parts. """
    pieces = memberId.split('.')
    if len(pieces) < 2:
        raise UserError("Member %r: Missing dot." % memberId)
    if len(pieces) > 2:
        raise UserError("Member %r: Dots out of control." % memberId)
    return tuple(pieces)

class Configuration:
    def __init__(self, filename, includePath):
        self.includePath = includePath
        config = {}
        execfile(filename, config)
        
        for name in ('name', 'members'):
            if name not in config:
                raise UserError(filename + ": `%s` was not defined." % name)
            setattr(self, name, config[name])
        
        self.irregularFilenames = config.get('irregularFilenames', {})

def readConfigFile(filename, includePath, cachedir):
    
    conf = Configuration(filename, includePath)

    
    
    interfaces = []
    interfacesByName = {}
    parser = xpidl.IDLParser(cachedir)

    def getInterface(interfaceName, errorLoc):
        iface = interfacesByName.get(interfaceName)
        if iface is None:
            idlFile = findIDL(conf.includePath, conf.irregularFilenames,
                              interfaceName)
            idl = loadIDL(parser, conf.includePath, idlFile)
            if not idl.hasName(interfaceName):
                raise UserError("The interface %s was not found "
                                "in the idl file %r."
                                % (interfaceName, idlFile))
            iface = idl.getName(interfaceName, errorLoc)
            iface.stubMembers = []
            interfaces.append(iface)
            interfacesByName[interfaceName] = iface
        return iface

    for memberId in conf.members:
        interfaceName, memberName = parseMemberId(memberId)
        iface = getInterface(interfaceName, errorLoc='looking for %r' % memberId)

        if not iface.attributes.scriptable:
            raise UserError("Interface %s is not scriptable. "
                            "IDL file: %r." % (interfaceName, idlFile))

        if memberName == '*':
            
            for member in iface.members:
                if member.kind in ('method', 'attribute') and not member.noscript:
                    addStubMember(iface.name + '.' + member.name, member)
        else:
            
            if memberName not in iface.namemap:
                idlFile = iface.idl.parser.lexer.filename
                raise UserError("Interface %s has no member %r. "
                                "(See IDL file %r.)"
                                % (interfaceName, memberName, idlFile))
            member = iface.namemap.get(memberName, None)
            if member in iface.stubMembers:
                raise UserError("Member %s is specified more than once."
                                % memberId)
            addStubMember(memberId, member)

    return conf, interfaces




def writeHeaderFile(filename, name):
    print "Creating header file", filename
    make_targets.append(filename)

    headerMacro = '__gen_%s__' % filename.replace('.', '_')
    f = open(filename, 'w')
    try:
        f.write("/* THIS FILE IS AUTOGENERATED - DO NOT EDIT */\n"
                "#ifndef " + headerMacro + "\n"
                "#define " + headerMacro + "\n"
                "JSBool " + name + "_DefineQuickStubs("
                "JSContext *cx, JSObject *proto, uintN flags, "
                "PRUint32 count, const nsID **iids);\n"
                "#endif\n")
    finally:
        f.close()



def substitute(template, vals):
    """ Simple replacement for string.Template, which isn't in Python 2.3. """
    def replacement(match):
        return vals[match.group(1)]
    return re.sub(r'\${(\w+)}', replacement, template)


argumentUnboxingTemplates = {
    'short':
        "    int32 ${name}_i32;\n"
        "    if (!JS_ValueToECMAInt32(cx, ${argVal}, &${name}_i32)) ${failBlock}\n"
        "    int16 ${name} = (int16) ${name}_i32;\n",

    'unsigned short':
        "    uint32 ${name}_u32;\n"
        "    if (!JS_ValueToECMAUint32(cx, ${argVal}, &${name}_u32)) ${failBlock}\n"
        "    uint16 ${name} = (uint16) ${name}_u32;\n",

    'long':
        "    int32 ${name};\n"
        "    if (!JS_ValueToECMAInt32(cx, ${argVal}, &${name})) ${failBlock}\n",

    'unsigned long':
        "    uint32 ${name};\n"
        "    if (!JS_ValueToECMAUint32(cx, ${argVal}, &${name})) ${failBlock}\n",

    'float':
        "    jsdouble ${name}_dbl;\n"
        "    if (!JS_ValueToNumber(cx, ${argVal}, &${name}_dbl)) ${failBlock}\n"
        "    float ${name} = (float) ${name}_dbl;\n",

    'double':
        "    jsdouble ${name};\n"
        "    if (!JS_ValueToNumber(cx, ${argVal}, &${name})) ${failBlock}\n",

    'boolean':
        "    PRBool ${name};\n"
        "    if (!JS_ValueToBoolean(cx, ${argVal}, &${name})) ${failBlock}\n",

    '[astring]':
        "    xpc_qsAString ${name}(cx, ${argPtr});\n"
        "    if (!${name}.IsValid()) ${failBlock}\n",

    '[domstring]':
        "    xpc_qsDOMString ${name}(cx, ${argPtr});\n"
        "    if (!${name}.IsValid()) ${failBlock}\n",

    'string':
        "    char *${name};\n"
        "    if (!xpc_qsJsvalToCharStr(cx, ${argPtr}, &${name})) ${failBlock}\n",

    'wstring':
        "    PRUnichar *${name};\n"
        "    if (!xpc_qsJsvalToWcharStr(cx, ${argPtr}, &${name})) ${failBlock}\n",

    '[cstring]':
        "    xpc_qsACString ${name}(cx, ${argPtr});\n"
        "    if (!${name}.IsValid()) ${failBlock}\n"
    }






def writeArgumentUnboxing(f, i, name, type, haveCcx, optional):
    
    
    
    
    
    
    
    

    isSetter = (i is None)

    
    
    fail = ("        NS_RELEASE(self);\n"
            "        return JS_FALSE;\n")

    if isSetter:
        argPtr = "vp"
        argVal = "*vp"
    elif optional:
        argPtr = '!  /* TODO - optional parameter of this type not supported */'
        argVal = "(%d < argc ? argv[%d] : JSVAL_NULL)" % (i, i)
    else:
        argVal = "argv[%d]" % i
        argPtr = "&" + argVal

    params = {
        'name': name,
        'argVal': argVal,
        'argPtr': argPtr,
        'failBlock': '{\n' + fail + '    }'
        }

    typeName = getBuiltinOrNativeTypeName(type)
    if typeName is not None:
        template = argumentUnboxingTemplates.get(typeName)
        if template is not None:
            if optional and ("${argPtr}" in template):
                warn("Optional parameters of type %s are not supported."
                     % type.name)
            f.write(substitute(template, params))
            return
        
    elif isInterfaceType(type):
        if type.name == 'nsIVariant':
            
            assert haveCcx
            template = (
                "    nsCOMPtr<nsIVariant> ${name}(already_AddRefed<nsIVariant>("
                "XPCVariant::newVariant(ccx, ${argVal})));\n"
                "    if (!${name}) ${failBlock}\n")
            f.write(substitute(template, params))
            return
        elif type.name == 'nsIAtom':
            
            pass
        else:
            f.write("    nsCOMPtr<%s> %s;\n" % (type.name, name))
            f.write("    rv = xpc_qsUnwrapArg<%s>("
                    "cx, %s, getter_AddRefs(%s));\n"
                    % (type.name, argVal, name))
            f.write("    if (NS_FAILED(rv)) {\n")
            if isSetter:
                f.write("        xpc_qsThrowBadSetterValue("
                        "cx, rv, wrapper, id);\n")
            elif haveCcx:
                f.write("        xpc_qsThrowBadArgWithCcx(ccx, rv, %d);\n" % i)
            else:
                f.write("        xpc_qsThrowBadArg(cx, rv, wrapper, vp, %d);\n"
                        % i)
            f.write(fail);
            f.write("    }\n")
            return

    warn("Unable to unbox argument of type %s" % type.name)
    if i is None:
        src = '*vp'
    else:
        src = 'argv[%d]' % i
    f.write("    !; // TODO - Unbox argument %s = %s\n" % (name, src))

def writeResultDecl(f, type):
    if isVoidType(type):
        return  
    
    t = unaliasType(type)
    if t.kind == 'builtin':
        if not t.nativename.endswith('*'):
            if type.kind == 'typedef':
                typeName = type.name  
            else:
                typeName = t.nativename
            f.write("    %s result;\n" % typeName)
            return
    elif t.kind == 'native':
        name = getBuiltinOrNativeTypeName(t)
        if name in ('[domstring]', '[astring]'):
            f.write("    nsString result;\n")
            return
    elif t.kind in ('interface', 'forward'):
        f.write("    nsCOMPtr<%s> result;\n" % type.name)
        return

    warn("Unable to declare result of type %s" % type.name)
    f.write("    !; // TODO - Declare out parameter `result`.\n")

def outParamForm(name, type):
    type = unaliasType(type)
    if type.kind == 'builtin':
        return '&' + name
    elif type.kind == 'native':
        if type.modifier == 'ref':
            return name
        else:
            return '&' + name
    else:
        return 'getter_AddRefs(%s)' % name


resultConvTemplates = {
    'void':
            "    ${jsvalRef} = JSVAL_VOID;\n"
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
        "    return xpc_qsStringToJsval(cx, result, ${jsvalPtr});\n"
    }    

def isVariantType(t):
    return isSpecificInterfaceType(t, 'nsIVariant')

def writeResultConv(f, type, paramNum, jsvalPtr, jsvalRef):
    """ Emit code to convert the C++ variable `result` to a jsval.

    The emitted code contains a return statement; it returns JS_TRUE on
    success, JS_FALSE on error.
    """
    
    typeName = getBuiltinOrNativeTypeName(type)
    if typeName is not None:
        template = resultConvTemplates.get(typeName)
        if template is not None:
            values = {'jsvalRef': jsvalRef,
                      'jsvalPtr': jsvalPtr}
            f.write(substitute(template, values))
            return
        
    elif isInterfaceType(type):
        if isVariantType(type):
            f.write("    return xpc_qsVariantToJsval(ccx, result, %d, %s);\n"
                    % (paramNum, jsvalPtr))
            return
        else:
            f.write("    return xpc_qsXPCOMObjectToJsval(ccx, result, "
                    "NS_GET_IID(%s), %s);\n" % (type.name, jsvalPtr))
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

def writeQuickStub(f, member, stubName, isSetter=False):
    """ Write a single quick stub (a custom SpiderMonkey getter/setter/method)
    for the specified XPCOM interface-member. 
    """
    isAttr = (member.kind == 'attribute')
    isMethod = (member.kind == 'method')
    assert isAttr or isMethod
    isGetter = isAttr and not isSetter

    
    f.write("static JS_DLL_CALLBACK JSBool\n")
    if isAttr:
        
        f.write(stubName + "(JSContext *cx, JSObject *obj, jsval id, "
                "jsval *vp)\n")
    else:
        
        f.write(stubName + "(JSContext *cx, uintN argc, jsval *vp)\n")
    f.write("{\n")
    f.write("    XPC_QS_ASSERT_CONTEXT_OK(cx);\n")

    
    if isMethod:
        f.write("    JSObject *obj = JS_THIS_OBJECT(cx, vp);\n"
                "    if (!obj)\n"
                "        return JS_FALSE;\n")

    
    haveCcx = isMethod and (isInterfaceType(member.realtype)
                            or anyParamRequiresCcx(member))
    if haveCcx:
            f.write("    XPCCallContext ccx(JS_CALLER, cx, obj, "
                    "JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));\n")
    else:
        
        
        if isAttr and isInterfaceType(member.realtype):
            f.write("    XPCCallContext ccx(JS_CALLER, cx, obj);\n")

    
    thisType = member.iface.name
    f.write("    %s *self;\n" % thisType)
    
    
    if isMethod and haveCcx:
        f.write("    if (!xpc_qsUnwrapThisFromCcx(ccx, &self))\n"
                "        return JS_FALSE;\n")
    else:
        
        f.write("    XPCWrappedNative *wrapper;\n"
                "    if (!xpc_qsUnwrapThis(cx, obj, &self, &wrapper))\n"
                "        return JS_FALSE;\n")

    if isMethod:
        
        requiredArgs = len(member.params)
        while requiredArgs and member.params[requiredArgs-1].optional:
            requiredArgs -= 1
        if requiredArgs:
            f.write("    if (argc < %d) {\n" % requiredArgs)
            f.write("        NS_RELEASE(self);\n"
                    "        return xpc_qsThrow(cx, "
                    "NS_ERROR_XPC_NOT_ENOUGH_ARGS);\n"
                    "    }\n")

    def pfail(msg):
        raise UserError(
            member.iface.name + '.' + member.name + ": "
            "parameter " + param.name + ": " + msg)

    
    f.write("    nsresult rv;\n")
    if isMethod:
        if len(member.params) > 0:
            f.write("    jsval *argv = JS_ARGV(cx, vp);\n")
        for i, param in enumerate(member.params):
            if param.iid_is is not None:
                pfail("iid_is parameters are not supported.")
            if param.size_is is not None:
                pfail("size_is parameters are not supported.")
            if param.retval:
                pfail("Unexpected retval parameter!")
            if param.paramtype in ('out', 'inout'):
                pfail("Out parameters are not supported.")
            if param.const or param.array or param.shared:
                pfail("I am a simple caveman.")
            
            writeArgumentUnboxing(
                f, i, 'arg%d' % i, param.realtype,
                haveCcx=haveCcx,
                optional=param.optional)
    elif isSetter:
        writeArgumentUnboxing(f, None, 'arg0', member.realtype,
                              haveCcx=False, optional=False)

    
    if isMethod or isGetter:
        writeResultDecl(f, member.realtype)

    
    if isMethod:
        comName = header.methodNativeName(member)
        argv = ['arg' + str(i) for i, p in enumerate(member.params)]
        if not isVoidType(member.realtype):
            argv.append(outParamForm('result', member.realtype))
        args = ', '.join(argv)
    else:
        comName = header.attributeNativeName(member, isGetter)
        if isGetter:
            args = outParamForm("result", member.realtype)
        else:
            args = "arg0"
    f.write("    rv = self->%s(%s);\n" % (comName, args))
    f.write("    NS_RELEASE(self);\n")

    
    f.write("    if (NS_FAILED(rv))\n")
    if isMethod:
        if haveCcx:
            f.write("        return xpc_qsThrowMethodFailedWithCcx(ccx, rv);\n")
        else:
            f.write("        return xpc_qsThrowMethodFailed("
                    "cx, rv, wrapper, vp);\n")
    else:
        f.write("        return xpc_qsThrowGetterSetterFailed("
                "cx, rv, wrapper, id);\n")

    
    if isMethod:
        writeResultConv(f, member.realtype, len(member.params) + 1, 'vp', '*vp')
    elif isGetter:
        writeResultConv(f, member.realtype, None, 'vp', '*vp')
    else:
        f.write("    return JS_TRUE;\n");

    
    f.write("}\n\n")

def writeAttrStubs(f, attr):
    getterName = (attr.iface.name + '_'
                  + header.attributeNativeName(attr, True))
    writeQuickStub(f, attr, getterName)
    if attr.readonly:
        setterName = 'xpc_qsReadOnlySetter'
    else:
        setterName = (attr.iface.name + '_'
                      + header.attributeNativeName(attr, False))
        writeQuickStub(f, attr, setterName, isSetter=True)

    ps = ('{"%s", %s, %s}'
          % (attr.name, getterName, setterName))
    return ps

def writeMethodStub(f, method):
    """ Write a method stub to `f`. Return an xpc_qsFunctionSpec initializer. """
    stubName = method.iface.name + '_' + header.methodNativeName(method)
    writeQuickStub(f, method, stubName)
    fs = '{"%s", %s, %d}' % (method.name, stubName, len(method.params))
    return fs

def writeStubsForInterface(f, iface):
    f.write("// === interface %s\n\n" % iface.name)
    propspecs = []
    funcspecs = []
    for member in iface.stubMembers:
        if member.kind == 'attribute':
            ps = writeAttrStubs(f, member)
            propspecs.append(ps)
        elif member.kind == 'method':
            fs = writeMethodStub(f, member)
            funcspecs.append(fs)
        else:
            raise TypeError('expected attribute or method, not %r'
                            % member.__class__.__name__)

    if propspecs:
        f.write("static const xpc_qsPropertySpec %s_properties[] = {\n"
                % iface.name)
        for ps in propspecs:
            f.write("    %s,\n" % ps)
        f.write("    {nsnull}};\n")
    if funcspecs:
        f.write("static const xpc_qsFunctionSpec %s_functions[] = {\n" % iface.name)
        for fs in funcspecs:
            f.write("    %s,\n" % fs)
        f.write("    {nsnull}};\n")
    f.write('\n\n')

def hashIID(iid):
    
    return int(iid[:8], 16)

uuid_re = re.compile(r'^([0-9a-f]{8})-([0-9a-f]{4})-([0-9a-f]{4})-([0-9a-f]{4})-([0-9a-f]{12})$')

def writeDefiner(f, conf, interfaces):
    f.write("// === Definer\n\n")

    
    loadFactor = 0.6
    size = int(len(interfaces) / loadFactor)
    buckets = [[] for i in range(size)]
    for iface in interfaces:
        
        
        if iface.stubMembers:
            h = hashIID(iface.attributes.uuid)
            buckets[h % size].append(iface)

    
    
    
    entryIndexes = {}
    arraySize = size
    for i, bucket in enumerate(buckets):
        if bucket:
            entryIndexes[bucket[0].attributes.uuid] = i
            for iface in bucket[1:]:
                entryIndexes[iface.attributes.uuid] = arraySize
                arraySize += 1

    entries = ["    {{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}}, "
               "nsnull, nsnull, XPC_QS_NULL_INDEX, XPC_QS_NULL_INDEX}"
               for i in range(arraySize)]
    for i, bucket in enumerate(buckets):
        for j, iface in enumerate(bucket):
            
            uuid = iface.attributes.uuid.lower()
            m = uuid_re.match(uuid)
            assert m is not None
            m0, m1, m2, m3, m4 = m.groups()
            m3arr = ('{0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s}'
                     % (m3[0:2], m3[2:4], m4[0:2], m4[2:4],
                        m4[4:6], m4[6:8], m4[8:10], m4[10:12]))
            iid = ('{0x%s, 0x%s, 0x%s, %s}' % (m0, m1, m2, m3arr))

            
            properties = "nsnull"
            for member in iface.stubMembers:
                if member.kind == 'attribute':
                    properties = iface.name + "_properties"
                    break
            functions = "nsnull"

            
            for member in iface.stubMembers:
                if member.kind == 'method':
                    functions = iface.name + "_functions"
                    break

            
            baseName = iface.base
            while baseName is not None:
                piface = iface.idl.getName(baseName, None)
                k = entryIndexes.get(piface.attributes.uuid)
                if k is not None:
                    parentInterface = str(k)
                    break
                baseName = piface.base
            else:
                parentInterface = "XPC_QS_NULL_INDEX"

            
            if j == len(bucket) - 1:
                chain = "XPC_QS_NULL_INDEX"
            else:
                k = entryIndexes[bucket[j+1].attributes.uuid]
                chain = str(k)

            
            entry = "    {%s, %s, %s, %s, %s}" % (
                iid, properties, functions, parentInterface, chain)
            entries[entryIndexes[iface.attributes.uuid]] = entry

    f.write("static const xpc_qsHashEntry tableData[] = {\n")
    f.write(",\n".join(entries))
    f.write("\n    };\n\n")

    
    f.write("JSBool %s_DefineQuickStubs(" % conf.name)
    f.write("JSContext *cx, JSObject *proto, uintN flags, PRUint32 count, "
            "const nsID **iids)\n"
            "{\n")
    f.write("    return xpc_qsDefineQuickStubs("
            "cx, proto, flags, count, iids, %d, tableData);\n" % size)
    f.write("}\n\n\n")


stubTopTemplate = '''\
/* THIS FILE IS AUTOGENERATED - DO NOT EDIT */
#include "jsapi.h"
#include "prtypes.h"
#include "nsID.h"
#include "%s"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsDependentString.h"
#include "xpcprivate.h"  // for XPCCallContext
#include "xpcquickstubs.h"

'''

def writeStubFile(filename, headerFilename, conf, interfaces):
    print "Creating stub file", filename
    make_targets.append(filename)

    f = open(filename, 'w')
    filesIncluded = sets.Set()

    def includeType(type):
        type = unaliasType(type)
        if type.kind in ('builtin', 'native'):
            return
        file = conf.irregularFilenames.get(type.name, type.name) + '.h'
        if file not in filesIncluded:
            f.write('#include "%s"\n' % file)
            filesIncluded.add(file)

    def writeIncludesForMember(member):
        assert member.kind in ('attribute', 'method')
        includeType(member.realtype)
        if member.kind == 'method':
            for p in member.params:
                includeType(p.realtype)

    def writeIncludesForInterface(iface):
        assert iface.kind == 'interface'
        for member in iface.stubMembers:
            writeIncludesForMember(member)
        includeType(iface)

    try:
        f.write(stubTopTemplate % os.path.basename(headerFilename))
        N = 256
        for iface in interfaces:
            writeIncludesForInterface(iface)
        f.write("\n\n")
        for iface in interfaces:
            writeStubsForInterface(f, iface)
        writeDefiner(f, conf, interfaces)
    finally:
        f.close()

def makeQuote(filename):
    return filename.replace(' ', '\\ ')  

def writeMakeDependOutput(filename):
    print "Creating makedepend file", filename
    f = open(filename, 'w')
    try:
        if len(make_targets) > 0:
            f.write("%s: \\\n" % makeQuote(make_targets[0]))
            for filename in make_dependencies:
                f.write('\t\t%s \\\n' % makeQuote(filename))
            f.write('\t\t$(NULL)\n\n')
            for filename in make_targets[1:]:
                f.write('%s: %s\n' % (makeQuote(filename), makeQuote(make_targets[0])))
    finally:
        f.close()

def main():
    from optparse import OptionParser
    o = OptionParser(usage="usage: %prog [options] configfile")
    o.add_option('-o', "--stub-output",
                 type='string', dest='stub_output', default=None,
                 help="Quick stub C++ source output file", metavar="FILE")
    o.add_option('--header-output', type='string', default=None,
                 help="Quick stub header output file", metavar="FILE")
    o.add_option('--makedepend-output', type='string', default=None,
                 help="gnumake dependencies output file", metavar="FILE")
    o.add_option('--idlpath', type='string', default='.',
                 help="colon-separated directories to search for idl files",
                 metavar="PATH")
    o.add_option('--cachedir', dest='cachedir', default='',
                 help="Directory in which to cache lex/parse tables.")
    o.add_option("--verbose-errors", action='store_true', default=False,
                 help="When an error happens, display the Python traceback.")
    (options, filenames) = o.parse_args()

    if len(filenames) != 1:
        o.error("Exactly one config filename is needed.")
    filename = filenames[0]

    if options.stub_output is None:
        if filename.endswith('.qsconf') or filename.endswith('.py'):
            options.stub_output = filename.rsplit('.', 1)[0] + '.cpp'
        else:
            options.stub_output = filename + '.cpp'
    if options.header_output is None:
        options.header_output = re.sub(r'(\.c|\.cpp)?$', '.h',
                                       options.stub_output)

    if options.cachedir != '':
        sys.path.append(options.cachedir)
        if not os.path.isdir(options.cachedir):
            os.mkdir(options.cachedir)

    try:
        includePath = options.idlpath.split(':')
        conf, interfaces = readConfigFile(filename,
                                          includePath=includePath,
                                          cachedir=options.cachedir)
        writeHeaderFile(options.header_output, conf.name)
        writeStubFile(options.stub_output, options.header_output,
                      conf, interfaces)
        if options.makedepend_output is not None:
            writeMakeDependOutput(options.makedepend_output)
    except Exception, exc:
        if options.verbose_errors:
            raise
        elif isinstance(exc, (UserError, xpidl.IDLError)):
            warn(str(exc))
        elif isinstance(exc, OSError):
            warn("%s: %s" % (exc.__class__.__name__, exc))
        else:
            raise
        sys.exit(1)

if __name__ == '__main__':
    main()
