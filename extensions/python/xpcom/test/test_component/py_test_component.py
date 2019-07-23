









































from xpcom import components, verbose

class PythonTestComponent:
    
    
    _com_interfaces_ = components.interfaces.nsIPythonTestInterfaceDOMStrings
    _reg_clsid_ = "{7EE4BDC6-CB53-42c1-A9E4-616B8E012ABA}"
    _reg_contractid_ = "Python.TestComponent"
    def __init__(self):
        self.boolean_value = 1
        self.octet_value = 2
        self.short_value = 3
        self.ushort_value = 4
        self.long_value = 5
        self.ulong_value = 6
        self.long_long_value = 7
        self.ulong_long_value = 8
        self.float_value = 9.0
        self.double_value = 10.0
        self.char_value = "a"
        self.wchar_value = "b"
        self.string_value = "cee"
        self.wstring_value = "dee"
        self.astring_value = "astring"
        self.acstring_value = "acstring"
        self.utf8string_value = "utf8string"
        self.iid_value = self._reg_clsid_
        self.interface_value = None
        self.isupports_value = None
        self.domstring_value = "dom"

    def __del__(self):
        if verbose:
            print "Python.TestComponent: __del__ method called - object is destructing"

    def do_boolean(self, p1, p2):
        
        ret = p1 ^ p2
        return ret, not ret, ret

    def do_octet(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2

    def do_short(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2

    def do_unsigned_short(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_long(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2

    def do_unsigned_long(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_long_long(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_unsigned_long_long(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_float(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_double(self, p1, p2):
        
        return p1+p2, p1-p2, p1*p2
    def do_char(self, p1, p2):
        
        return chr(ord(p1)+ord(p2)), p2, p1
    def do_wchar(self, p1, p2):
        
        return chr(ord(p1)+ord(p2)), p2, p1
    def do_string(self, p1, p2):
        
        ret = ""
        if p1 is not None: ret = ret + p1
        if p2 is not None: ret = ret + p2
        return ret, p1, p2
    def do_wstring(self, p1, p2):
        
        ret = u""
        if p1 is not None: ret = ret + p1
        if p2 is not None: ret = ret + p2
        return ret, p1, p2
    def do_nsIIDRef(self, p1, p2):
        
        return p1, self._reg_clsid_, p2
    def do_nsIPythonTestInterface(self, p1, p2):
        
        return p2, p1, self
    def do_nsISupports(self, p1, p2):
        
        return self, p1, p2
    def do_nsISupportsIs(self, iid):
        
        
        
        
        
        return self











    
    
    def MultiplyEachItemInIntegerArray(self, val, valueArray):
        
        
        
        
        
        results = []
        for item in valueArray:
            results.append(item * val)
        return results
    def MultiplyEachItemInIntegerArrayAndAppend(self, val, valueArray):
        
        
        
        
        results = valueArray[:]
        for item in valueArray:
            results.append(item * val)
        return results
    def DoubleStringArray(self, valueArray):
        
        
        results = []
        for item in valueArray:
            results.append(item * 2)
        return results

    def ReverseStringArray(self, valueArray):
        
        
        valueArray.reverse()
        return valueArray

    
    def CompareStringArrays(self, ar1, ar2):
        
        
        
        
        return cmp(ar1, ar2)

    def DoubleString(self, val):
        
        
        return val * 2
    def DoubleString2(self, val):
        
        
        return val * 2
    def DoubleString3(self, val):
        
        
        return val * 2
    def DoubleString4(self, val):
        
        return val * 2
    def UpString(self, val):
        
        
        
        return val.upper()
    UpString2 = UpString
        
        
        
        

    def GetFixedString(self, count):
        
        return "A" * count

    
    def DoubleWideString(self, val):
        return val * 2
    def DoubleWideString2(self, val):
        return val * 2
    def DoubleWideString3(self, val):
        return val * 2
    def DoubleWideString4(self, val):
        return val * 2
    def UpWideString(self, val):
        return val.upper()
    UpWideString2 = UpWideString
    def CopyUTF8String(self, v):
        return v
    def CopyUTF8String2(self, v):
        return v.encode("utf8")
    
    def GetFixedWideString(self, count):
        
        return u"A" * count

    def GetStrings(self):
        
        
        return "Hello from the Python test component".split()
    
    def UpOctetArray( self, data ):
        
        
        return data.upper()

    def UpOctetArray2( self, data ):
        
        
        data = data.upper()
        
        return map( ord, data )

    
    def CheckInterfaceArray(self, interfaces):
        
        
        
        ret = 1
        for i in interfaces:
            if i is None:
                ret = 0
                break
        return ret
    def CopyInterfaceArray(self, a):
        return a
    def GetInterfaceArray(self):
        
        
        return self, self, self, None
    def ExtendInterfaceArray(self, data):
        
        
        return data * 2

    
    def CheckIIDArray(self, data):
        
        
        
        ret = 1
        for i in data:
            if i!= self._com_interfaces_ and i != self._reg_clsid_:
                ret = 0
                break
        return ret
    def GetIIDArray(self):
        
        
        return self._com_interfaces_, self._reg_clsid_
    def ExtendIIDArray(self, data):
        
        
        return data * 2

    
    def SumArrays(self, array1, array2):
        
        if len(array1)!=len(array2):
            print "SumArrays - not expecting different lengths!"
        result = 0
        for i in array1:
            result = result + i
        for i in array2:
            result = result+i
        return result

    
    def GetArrays(self):
        
        return (1,2,3), (4,5,6)
    
    def GetFixedArray(self, size):
        
        return 0 * size
    
    
    def CopyArray(self, array1):
        
        return array1
    
    def CopyAndDoubleArray(self, array):
        
        return array + array
    
    def AppendArray(self, array1, array2):
        
        rc = array1
        if array2 is not None:
            rc.extend(array2)
        return rc
    
    def AppendVariant(self, invar, inresult):
        if type(invar)==type([]):
            invar_use = invar[0]
            for v in invar[1:]:
                invar_use += v
        else:
            invar_use = invar
        if type(inresult)==type([]):
            inresult_use = inresult[0]
            for v in inresult[1:]:
                inresult_use += v
        else:
            inresult_use = inresult
        if inresult_use is None and invar_use is None:
            return None
        return inresult_use + invar_use

    def CopyVariant(self, invar):
        return invar

    def SumVariants(self, variants):
        if len(variants) == 0:
            return None
        result = variants[0]
        for v in variants[1:]:
            result += v
        return result

    
    def GetDOMStringResult( self, length ):
        
        if length == -1:
            return None
        return "P" * length
    def GetDOMStringOut( self, length ):
        
        if length == -1:
            return None
        return "y" * length
    def GetDOMStringLength( self, param0 ):
        
        
        if param0 is None: return -1
        return len(param0)

    def GetDOMStringRefLength( self, param0 ):
        
        
        if param0 is None: return -1
        return len(param0)

    def GetDOMStringPtrLength( self, param0 ):
        
        
        if param0 is None: return -1
        return len(param0)

    def ConcatDOMStrings( self, param0, param1 ):
        
        
        
        
        return param0 + param1
    def get_domstring_value( self ):
        
        return self.domstring_value
    def set_domstring_value( self, param0 ):
        
        
        self.domstring_value = param0

    def get_domstring_value_ro( self ):
        
        return self.domstring_value
