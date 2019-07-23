
package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;














public class DataTypes_006 extends LiveConnectTest {
    public DataTypes_006() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_006 test = new DataTypes_006();
        test.start();
    }

    public void executeTest() {
        doMethodTests(
            "dt.getBooleanObject()",
            "java.lang.Boolean",
            (Object) new Boolean(DataTypeClass.PUB_STATIC_FINAL_BOOLEAN) );
            
        doMethodTests(
            "dt.getBoolean()",
            "java.lang.Boolean",
            (Object) new Boolean(DataTypeClass.PUB_STATIC_FINAL_BOOLEAN) );

        doMethodTests(
            "dt.getByte()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_BYTE) );

        doMethodTests(
            "dt.getByteObject()",
            "java.lang.Byte",
            (Object) new Byte(DataTypeClass.PUB_STATIC_FINAL_BYTE) );

        doMethodTests(
            "dt.getShort()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_SHORT) );

        doMethodTests(
            "dt.getShortObject()",
            "java.lang.Short",
            (Object) new Short(DataTypeClass.PUB_STATIC_FINAL_SHORT) );

        doMethodTests(
            "dt.getInteger()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_INT) );

        doMethodTests(
            "dt.getIntegerObject()",
            "java.lang.Integer",
            (Object) new Integer(DataTypeClass.PUB_STATIC_FINAL_INT) );

        doMethodTests(
            "dt.getLong()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_LONG) );

        doMethodTests(
            "dt.getLongObject()",
            "java.lang.Long",
            (Object) new Long(DataTypeClass.PUB_STATIC_FINAL_LONG) );

        doMethodTests(
            "dt.getFloat()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_FLOAT) );

        doMethodTests(
            "dt.getFloatObject()",
            "java.lang.Float",
            (Object) new Float(DataTypeClass.PUB_STATIC_FINAL_FLOAT) );

        doMethodTests(
            "dt.getDouble()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_DOUBLE) );

        doMethodTests(
            "dt.getDoubleObject()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_DOUBLE) );

        doMethodTests(
            "dt.getChar()",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_CHAR) );

        doMethodTests(
            "dt.getCharacter()",
            "java.lang.Character",
            (Object) new Character(DataTypeClass.PUB_STATIC_FINAL_CHAR) );

        doMethodTests(
            "dt.getStringObject()",
            "java.lang.String",
            (Object) new String(DataTypeClass.PUB_STATIC_FINAL_STRING) );
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass"  );
        global.eval( "var dt = new DT();" );
    }

    public void doMethodTests( String method, String className, Object value ) {
        getPublicMethod( method, className, value );
    }

    




    public void getPublicMethod( String method, String className, Object value ) {
        String description = method;
        String exception = null;
        Object actual = null;
        String expect = null;

        
        try {
            actual = global.eval( method );
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

        addTestCase( "\"" +actual.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            actual.toString().equals(value.toString()) +"",
            exception );
    }

    

























































    public void setPublicField ( String field, String className, Object value ) {
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

        
        

        addTestCase( "\"" +after.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            after.toString().equals(value.toString()) +"",
            exception );

        
        

        addTestCase( "( " + before +".equals(" + after +") ) ",
                    "true",
                    ( before.equals(after) ) +"",
                    exception );

    }









































 }