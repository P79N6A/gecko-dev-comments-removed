



#ifndef dom_src_geolocation_IPC_serialiser
#define dom_src_geolocation_IPC_serialiser

#include "ipc/IPCMessageUtils.h"
#include "nsGeoPosition.h"
#include "nsIDOMGeoPosition.h"

typedef nsIDOMGeoPosition* GeoPosition;

namespace IPC {

template <>
struct ParamTraits<nsIDOMGeoPositionCoords*>
{
  typedef nsIDOMGeoPositionCoords* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) return;

    double coordData;

    aParam->GetLatitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetLongitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAltitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAccuracy(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAltitudeAccuracy(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetHeading(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetSpeed(&coordData);
    WriteParam(aMsg, coordData);
  }

  
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) return false;

    if (isNull) {
      *aResult = 0;
      return true;
    }

    double latitude;
    double longitude;
    double altitude;
    double accuracy;
    double altitudeAccuracy;
    double heading;
    double speed;

    
    if (!(   ReadParam(aMsg, aIter, &latitude         )
          && ReadParam(aMsg, aIter, &longitude        )
          && ReadParam(aMsg, aIter, &altitude         )
          && ReadParam(aMsg, aIter, &accuracy         )
          && ReadParam(aMsg, aIter, &altitudeAccuracy )
          && ReadParam(aMsg, aIter, &heading          )
          && ReadParam(aMsg, aIter, &speed            ))) return false;

    
    *aResult = new nsGeoPositionCoords(latitude,         
                                       longitude,        
                                       altitude,         
                                       accuracy,         
                                       altitudeAccuracy, 
                                       heading,          
                                       speed             
                                      );
    return true;

  }

};

template <>
struct ParamTraits<nsIDOMGeoPosition*>
{
  typedef nsIDOMGeoPosition* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) return;

    DOMTimeStamp timeStamp;
    aParam->GetTimestamp(&timeStamp);
    WriteParam(aMsg, timeStamp);

    nsCOMPtr<nsIDOMGeoPositionCoords> coords;
    aParam->GetCoords(getter_AddRefs(coords));
    WriteParam(aMsg, coords.get());
  }

  
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) return false;

    if (isNull) {
      *aResult = 0;
      return true;
    }

    DOMTimeStamp timeStamp;
    nsIDOMGeoPositionCoords* coords = nullptr;

    
    if (!ReadParam(aMsg, aIter, &timeStamp) ||
        !ReadParam(aMsg, aIter, &coords)) {
      nsCOMPtr<nsIDOMGeoPositionCoords> tmpcoords = coords;
      return false;
    }

    *aResult = new nsGeoPosition(coords, timeStamp);

    return true;
  };

};

}

#endif
