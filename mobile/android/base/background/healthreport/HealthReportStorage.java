



package org.mozilla.gecko.background.healthreport;

import android.database.Cursor;
import android.util.SparseArray;




public interface HealthReportStorage {
  
  public interface MeasurementFields {
    public class FieldSpec {
      public final String name;
      public final int type;
      public FieldSpec(String name, int type) {
        this.name = name;
        this.type = type;
      }
    }
    Iterable<FieldSpec> getFields();
  }

  public abstract class Field {
    protected static final int UNKNOWN_TYPE_OR_FIELD_ID = -1;

    protected static final int FLAG_INTEGER  = 1 << 0;
    protected static final int FLAG_STRING   = 1 << 1;

    protected static final int FLAG_DISCRETE = 1 << 8;
    protected static final int FLAG_LAST     = 1 << 9;
    protected static final int FLAG_COUNTER  = 1 << 10;

    public static final int TYPE_INTEGER_DISCRETE = FLAG_INTEGER | FLAG_DISCRETE;
    public static final int TYPE_INTEGER_LAST     = FLAG_INTEGER | FLAG_LAST;
    public static final int TYPE_INTEGER_COUNTER  = FLAG_INTEGER | FLAG_COUNTER;

    public static final int TYPE_STRING_DISCRETE  = FLAG_STRING | FLAG_DISCRETE;
    public static final int TYPE_STRING_LAST      = FLAG_STRING | FLAG_LAST;

    protected int fieldID = UNKNOWN_TYPE_OR_FIELD_ID;
    protected int flags;

    protected final String measurementName;
    protected final String measurementVersion;
    protected final String fieldName;

    public Field(String mName, int mVersion, String fieldName, int type) {
      this.measurementName = mName;
      this.measurementVersion = Integer.toString(mVersion, 10);
      this.fieldName = fieldName;
      this.flags = type;
    }

    



    public abstract int getID() throws IllegalStateException;

    public boolean isIntegerField() {
      return (this.flags & FLAG_INTEGER) > 0;
    }

    public boolean isStringField() {
      return (this.flags & FLAG_STRING) > 0;
    }

    public boolean isDiscreteField() {
      return (this.flags & FLAG_DISCRETE) > 0;
    }
  }

  


  public void close();

  






  public int getDay(long time);

  




  public int getDay();

  





  public Environment getEnvironment();

  



  public SparseArray<String> getEnvironmentHashesByID();

  



  public SparseArray<Environment> getEnvironmentRecordsByID();

  




  public Cursor getEnvironmentRecordForID(int id);

  









  public Field getField(String measurement, int measurementVersion,
                        String fieldName);

  



  public SparseArray<Field> getFieldsByID();

  public void recordDailyLast(int env, int day, int field, String value);
  public void recordDailyLast(int env, int day, int field, int value);
  public void recordDailyDiscrete(int env, int day, int field, String value);
  public void recordDailyDiscrete(int env, int day, int field, int value);
  public void incrementDailyCount(int env, int day, int field, int by);
  public void incrementDailyCount(int env, int day, int field);

  



  public Cursor getRawEventsSince(long time);

  





  public Cursor getEventsSince(long time);

  












  public void ensureMeasurementInitialized(String measurement,
                                           int version,
                                           MeasurementFields fields);
  public Cursor getMeasurementVersions();
  public Cursor getFieldVersions();
  public Cursor getFieldVersions(String measurement, int measurementVersion);

  public void deleteEverything();
  public void deleteEnvironments();
  public void deleteMeasurements();

  public void enqueueOperation(Runnable runnable);
}
