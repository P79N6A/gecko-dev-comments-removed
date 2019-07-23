
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;














public class DataTypes_005 extends LiveConnectTest {
    public DataTypes_005() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_005 test = new DataTypes_005();
        test.start();
    }

    public void executeTest() {
        doFieldTests( 
            "dt.PUB_BOOLEAN",
            "java.lang.Boolean",
            (Object) new Boolean(DataTypeClass.PUB_STATIC_FINAL_BOOLEAN) );

        doFieldTests( 
            "dt.PUB_BYTE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_BYTE) );
            
        doFieldTests( 
            "dt.PUB_SHORT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_SHORT) );

        doFieldTests( 
            "dt.PUB_INT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_INT) );
            
        doFieldTests( 
            "dt.PUB_LONG",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_LONG) );
            
        doFieldTests( 
            "dt.PUB_FLOAT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_FLOAT) );
            
        doFieldTests( 
            "dt.PUB_DOUBLE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_DOUBLE) );
            
        doFieldTests(
            "dt.PUB_CHAR",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_CHAR) );

        doFieldTests( 
            "dt.PUB_STRING",
            "java.lang.String",
            (Object) new String(DataTypeClass.PUB_STATIC_FINAL_STRING) );

    }
    
    






    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = "+
            "Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass");
        global.eval( "var dt = new DT();" );
    }
    
    









    public void doFieldTests( String field, String className, Object value ) {
        getPublicField( field, className, value );
        
        if ( field.startsWith( "dt.PUB_FINAL" )) {
            setPublicFinalField( field, className, value );                
        } else {
            setPublicField( field, className, value );
        }
        
        setJavaScriptVariable( field, className, value );
    }        
    
    











    public void getPublicField( String field, String className, 
        Object value ) 
    {
        String description = field;
        String exception = null;
        Object actual = null;
        String expect = null;
        
        
        try {
            global.eval( "var myobject = " +description );            
            actual = global.getMember( "myobject" );
            expect  = Class.forName(className).getName();
        } catch ( ClassNotFoundException e ) {
        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        
        
        addTestCase( "( " + description +" ).getClass()",
            expect,
            actual.getClass().getName(),
            exception );
            
        addTestCase( "( " +description + " == " + value.toString() +" )",
            "true",
            actual.equals( value ) + "",
            exception );

        addTestCase( 
            "\"" +actual.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            actual.toString().equals(value.toString()) +"",
            exception );
    }

    












    public void setPublicField( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object before = null;
        Object after = null;
        String expect = null;
       
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("New Value!") 
                            ;        
        try {
            before = global.eval( description );

            
            if ( className.equals("java.lang.String") ){
                global.eval( description +" = \"" + newValue.toString() +"\"" );
            } else {
                global.eval( description +" = " + newValue );                
            }        
    
            after = global.eval( description );
            expect = Class.forName( className ).getName();
        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        addTestCase( "global.eval(\""+description+" = "+newValue.toString()+
            "\"); after = global.eval( \"" + description +"\" );" +
            "after.getClass().getName()",
            expect,
            after.getClass().getName(),
            exception );
            
        addTestCase( 
            "( "+after.toString() +" ).equals(" + newValue.toString() +")",
            "true",
            after.equals( newValue ) + "",
            exception );

        addTestCase( 
            "\"" +after.toString() + "\".equals(\""+newValue.toString()+"\")",
            "true",
            after.toString().equals(newValue.toString()) +"",
            exception );        
    }       
    
    











    public void setPublicFinalField ( String field, String className, 
        Object value ) 
    {
        String description = field;
        String exception = null;
        Object before = null;
        Object after = null;
        String expect = null;
        
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("New Value!") 
                            ;        
                            
        try {
            expect = Class.forName( className ).getName();
        } catch ( Exception e ) {
            exception = e.toString();
        }            
            before = global.eval( description );
            global.eval( description +" = " + newValue.toString() );
            after = global.eval( description );
        
        
        
        addTestCase( "( " + description +" ).getClass()",
            expect,
            after.getClass().getName(),
            exception );
            
        
            
        addTestCase( "( " +description + " == " + value.toString() +" )",
            "true",
            after.equals( value ) + "",
            exception );
            
        
        

        addTestCase( 
            "\"" +after.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            after.toString().equals(value.toString()) +"",
            exception );        
            
        
        
        
        addTestCase( "( " + before +".equals(" + after +") ) ",
                    "true",
                    ( before.equals(after) ) +"",
                    exception );
    }           
    
    












    public void setJavaScriptVariable( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object actual = null;
        String expect = null;
        
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("New Value!") 
                            ;        
        try {
            global.eval( "var myobject = " + description );
            global.setMember( "myobject", newValue );
            actual = global.getMember( "myobject" );
            expect = Class.forName( className ).getName();
        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        addTestCase( "global.eval(\"var myobject = " + description +"\" ); " +
            "global.setMember( \"myobject\", " + newValue.toString() +");" +
            "actual = global.getMember( \"myobject\" ); "+
            "actual.getClass().getName()",
            expect,
            actual.getClass().getName(),
            exception );
            
        addTestCase( "( " + description + " == " + newValue.toString() +" )",
            "true",
            actual.equals( newValue ) + "",
            exception );

        addTestCase( "\"" +actual.toString() + "\".equals(\"" + newValue.toString() +"\")",
            "true",
            actual.toString().equals(newValue.toString()) +"",
            exception );        
    }           
 }