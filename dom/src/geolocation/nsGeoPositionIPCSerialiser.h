



































#ifndef dom_src_geolocation_IPC_serialiser
#define dom_src_geolocation_IPC_serialiser


#include "IPC/IPCMessageUtils.h"
#include "nsGeoPosition.h"
#include "nsIDOMGeoPosition.h"

typedef nsIDOMGeoPositionAddress  *GeoPositionAddress;
typedef nsIDOMGeoPositionCoords   *GeoPositionCoords;
typedef nsIDOMGeoPosition         *GeoPosition;

namespace IPC {

template <>
struct ParamTraits<GeoPositionAddress>
{
  typedef GeoPositionAddress paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) return;

    nsString addressLine;

    aParam->GetStreetNumber(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetStreet(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetPremises(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetCity(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetCounty(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetRegion(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetCountry(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetCountryCode(addressLine);
    WriteParam(aMsg, addressLine);

    aParam->GetPostalCode(addressLine);
    WriteParam(aMsg, addressLine);
  }

  
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) return false;

    if (isNull) {
      *aResult = 0;
      return true;
    }

    
    nsString streetNumber;
    nsString street;
    nsString premises;
    nsString city;
    nsString county;
    nsString region;
    nsString country;
    nsString countryCode;
    nsString postalCode;

    
    if (!(ReadParam(aMsg, aIter, &streetNumber) &&
          ReadParam(aMsg, aIter, &street      ) &&
          ReadParam(aMsg, aIter, &premises    ) &&
          ReadParam(aMsg, aIter, &city        ) &&
          ReadParam(aMsg, aIter, &county      ) &&
          ReadParam(aMsg, aIter, &region      ) &&
          ReadParam(aMsg, aIter, &country     ) &&
          ReadParam(aMsg, aIter, &countryCode ) &&
          ReadParam(aMsg, aIter, &postalCode  ))) return false;

    
    *aResult = new nsGeoPositionAddress(streetNumber, 
                                        street,       
                                        premises,     
                                        city,         
                                        county,       
                                        region,       
                                        country,      
                                        countryCode,  
                                        postalCode    
                                       );
    return true;
  }
} ;

template <>
struct ParamTraits<GeoPositionCoords>
{
  typedef GeoPositionCoords paramType;

  
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
struct ParamTraits<GeoPosition>
{
  typedef GeoPosition paramType;

  
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
    GeoPositionCoords simpleCoords = coords.get();
    WriteParam(aMsg, simpleCoords);

    nsCOMPtr<nsIDOMGeoPositionAddress> address;
    aParam->GetAddress(getter_AddRefs(address));
    GeoPositionAddress simpleAddress = address.get();
    WriteParam(aMsg, simpleAddress);
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
    GeoPositionCoords coords = nsnull;
    GeoPositionAddress address;

    
    if (!(   ReadParam(aMsg, aIter, &timeStamp)
          && ReadParam(aMsg, aIter, &coords   )
          && ReadParam(aMsg, aIter, &address  ))) {
          
          
          
          delete coords;
          return false;
      }

    *aResult = new nsGeoPosition(coords, address, timeStamp);

    return true;
  };

};

}

#endif

