













template <typename T>
int
variantToSQLiteT(T aObj,
                 nsIVariant *aValue)
{
  
  if (!aValue)
    return sqlite3_T_null(aObj);

  uint16_t type;
  (void)aValue->GetDataType(&type);
  switch (type) {
    case nsIDataType::VTYPE_INT8:
    case nsIDataType::VTYPE_INT16:
    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_UINT8:
    case nsIDataType::VTYPE_UINT16:
    {
      int32_t value;
      nsresult rv = aValue->GetAsInt32(&value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_int(aObj, value);
    }
    case nsIDataType::VTYPE_UINT32: 
    case nsIDataType::VTYPE_INT64:
    
    case nsIDataType::VTYPE_UINT64:
    {
      int64_t value;
      nsresult rv = aValue->GetAsInt64(&value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_int64(aObj, value);
    }
    case nsIDataType::VTYPE_FLOAT:
    case nsIDataType::VTYPE_DOUBLE:
    {
      double value;
      nsresult rv = aValue->GetAsDouble(&value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_double(aObj, value);
    }
    case nsIDataType::VTYPE_BOOL:
    {
      bool value;
      nsresult rv = aValue->GetAsBool(&value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_int(aObj, value ? 1 : 0);
    }
    case nsIDataType::VTYPE_CHAR:
    case nsIDataType::VTYPE_CHAR_STR:
    case nsIDataType::VTYPE_STRING_SIZE_IS:
    case nsIDataType::VTYPE_UTF8STRING:
    case nsIDataType::VTYPE_CSTRING:
    {
      nsAutoCString value;
      
      
      
      nsresult rv = aValue->GetAsAUTF8String(value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_text(aObj, value);
    }
    case nsIDataType::VTYPE_WCHAR:
    case nsIDataType::VTYPE_DOMSTRING:
    case nsIDataType::VTYPE_WCHAR_STR:
    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
    case nsIDataType::VTYPE_ASTRING:
    {
      nsAutoString value;
      
      
      
      nsresult rv = aValue->GetAsAString(value);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);
      return sqlite3_T_text16(aObj, value);
    }
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
    case nsIDataType::VTYPE_EMPTY_ARRAY:
      return sqlite3_T_null(aObj);
    case nsIDataType::VTYPE_ARRAY:
    {
      uint16_t type;
      nsIID iid;
      uint32_t count;
      void *data;
      nsresult rv = aValue->GetAsArray(&type, &iid, &count, &data);
      NS_ENSURE_SUCCESS(rv, SQLITE_MISMATCH);

      
      NS_ASSERTION(type == nsIDataType::VTYPE_UINT8,
                   "Invalid type passed!  You may leak!");
      if (type != nsIDataType::VTYPE_UINT8) {
        
        
        free(data);
        return SQLITE_MISMATCH;
      }

      
      int rc = sqlite3_T_blob(aObj, data, count);
      return rc;
    }
    
    
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
    default:
      return SQLITE_MISMATCH;
  }
  return SQLITE_OK;
}
