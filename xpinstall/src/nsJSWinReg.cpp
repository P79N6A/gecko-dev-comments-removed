




































#include "jsapi.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"
#include "nsWinReg.h"
#include "nsJSWinReg.h"

static void PR_CALLBACK WinRegCleanup(JSContext *cx, JSObject *obj);

extern void ConvertJSValToStr(nsString&  aString,
                             JSContext* aContext,
                             jsval      aValue);

extern void ConvertStrToJSVal(const nsString& aProp,
                             JSContext* aContext,
                             jsval* aReturn);

extern PRBool ConvertJSValToBool(PRBool* aProp,
                                JSContext* aContext,
                                jsval aValue);


static void PR_CALLBACK WinRegCleanup(JSContext *cx, JSObject *obj)
{
    nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
    delete nativeThis;
}





JSClass WinRegClass = {
  "WinReg",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  WinRegCleanup
};







PR_STATIC_CALLBACK(JSBool)
WinRegSetRootKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32  b0;

  *rval = JSVAL_NULL;

  if(argc >= 1)
  {
    
    if(JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      nativeThis->SetRootKey(b0);
    }
    else
    {
      JS_ReportWarning(cx, "Parameter must be a number");
    }
  }
  else
  {
    JS_ReportWarning(cx, "Function SetRootKey requires 1 parameters");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinRegKeyExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRBool nativeRet;
  nsAutoString b0;

  *rval = JSVAL_FALSE;

  if(argc >= 1)
  {
    

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK == nativeThis->KeyExists(b0, &nativeRet))
    {
      *rval = BOOLEAN_TO_JSVAL(nativeRet);
    }
    else
    {
      NS_WARNING("WinReg.KeyExists() internal error");
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.KeyExists() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegValueExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRBool nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_FALSE;

  if(argc >= 2)
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->ValueExists(b0, b1, &nativeRet))
    {
      *rval = BOOLEAN_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.ValueExists() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegIsKeyWritable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_FALSE;

  if(argc >= 1)
  {
    

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK == nativeThis->IsKeyWritable(b0, &nativeRet))
    {
      *rval = BOOLEAN_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.IsKeyWritable() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegCreateKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->CreateKey(b0, b1, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.CreateKey() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegDeleteKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 1)                             
  {
    

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK == nativeThis->DeleteKey(b0, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.DeleteKey() parameters error");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinRegDeleteValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsString b0;
  nsString b1;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->DeleteValue(b0, b1, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.DeleteValue() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegSetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 3)
  {
    
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);

    if(NS_OK == nativeThis->SetValueString(b0, b1, b2, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.SetValueString() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegGetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsString     nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->GetValueString(b0, b1, &nativeRet))
    {
      ConvertStrToJSVal(nativeRet, cx, rval);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.GetValueString() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegEnumValueNames(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString   nativeRet;
  nsAutoString   b0;
  int32          b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);

    if(JS_ValueToInt32(cx, argv[1], &b1))
    {
      if ( NS_OK == nativeThis->EnumValueNames(b0, b1, nativeRet) )
      {
          ConvertStrToJSVal(nativeRet, cx, rval);
      }
    }
    else  
    {
      JS_ReportWarning(cx, "WinReg.enumValueNames - Parameter 2 must be a number");
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.enumValueNames() - Too few parameters");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinRegEnumKeys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString   nativeRet;
  nsAutoString   b0;
  int32          b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);

    if(JS_ValueToInt32(cx, argv[1], &b1))
    {
      if ( NS_OK ==  nativeThis->EnumKeys(b0, b1, nativeRet) )
      {
          ConvertStrToJSVal(nativeRet, cx, rval);
      }
    }
    else
    {
      JS_ReportWarning(cx, "WinReg.enumKeys() - Parameter 2 must be a number");
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.enumKeys() - Too few parameters");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinRegSetValueNumber(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  int32        ib2;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 3)
  {
    
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(!JS_ValueToInt32(cx, argv[2], &ib2))
    {
      JS_ReportWarning(cx, "Parameter 3 must be a number");
    }
    else if(NS_OK == nativeThis->SetValueNumber(b0, b1, ib2, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.SetValueNumber() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegGetValueNumber(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32      nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->GetValueNumber(b0, b1, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.GetValueNumber() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegSetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString b0;
  nsAutoString b1;

  *rval = INT_TO_JSVAL(nsInstall::UNEXPECTED_ERROR);

  if(argc >= 3)
  {
    
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    
    








  }
  else
  {
    JS_ReportWarning(cx, "WinReg.SetValue() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinRegGetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis =
    (nsWinReg*)JS_GetInstancePrivate(cx, obj, &WinRegClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsWinRegValue *nativeRet;
  nsAutoString  b0;
  nsAutoString  b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK == nativeThis->GetValue(b0, b1, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinReg.GetValue() parameters error");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinReg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


static JSConstDoubleSpec winreg_constants[] = 
{
    { nsWinReg::NS_HKEY_CLASSES_ROOT,        "HKEY_CLASSES_ROOT"            },
    { nsWinReg::NS_HKEY_CURRENT_USER,        "HKEY_CURRENT_USER"            },
    { nsWinReg::NS_HKEY_LOCAL_MACHINE,       "HKEY_LOCAL_MACHINE"           },
    { nsWinReg::NS_HKEY_USERS,               "HKEY_USERS"                   },
    {0}
};




static JSFunctionSpec WinRegMethods[] = 
{
  {"setRootKey",                WinRegSetRootKey,               1,0,0},
  {"keyExists",                 WinRegKeyExists,                1,0,0},
  {"valueExists",               WinRegValueExists,              2,0,0},
  {"isKeyWritable",             WinRegIsKeyWritable,            1,0,0},
  {"createKey",                 WinRegCreateKey,                2,0,0},
  {"deleteKey",                 WinRegDeleteKey,                1,0,0},
  {"deleteValue",               WinRegDeleteValue,              2,0,0},
  {"setValueString",            WinRegSetValueString,           3,0,0},
  {"getValueString",            WinRegGetValueString,           2,0,0},
  {"setValueNumber",            WinRegSetValueNumber,           3,0,0},
  {"getValueNumber",            WinRegGetValueNumber,           2,0,0},
  {"setValue",                  WinRegSetValue,                 3,0,0},
  {"getValue",                  WinRegGetValue,                 2,0,0},
  {"enumKeys",                  WinRegEnumKeys,                 2,0,0},
  {"enumValueNames",            WinRegEnumValueNames,           2,0,0},
  {nsnull,nsnull,0,0,0}
};

PRInt32
InitWinRegPrototype(JSContext *jscontext, JSObject *global, JSObject **winRegPrototype)
{
  *winRegPrototype = JS_InitClass( jscontext,         
                                  global,            
                                  nsnull,            
                                  &WinRegClass,      
                                  nsnull,            
                                  0,                 
                                  nsnull,            
                                  nsnull,            
                                  nsnull,            
                                  WinRegMethods);    

  if(nsnull == *winRegPrototype) 
  {
    return NS_ERROR_FAILURE;
  }

  if(PR_FALSE == JS_DefineConstDoubles(jscontext, *winRegPrototype, winreg_constants))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

