









#ifndef __MEASUREUNIT_H__
#define __MEASUREUNIT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"





 
U_NAMESPACE_BEGIN

class StringEnumeration;








class U_I18N_API MeasureUnit: public UObject {
 public:

    



    MeasureUnit() : fTypeId(0), fSubTypeId(0) { 
        fCurrency[0] = 0;
    }
    
    



    MeasureUnit(const MeasureUnit &other);
        
    



    MeasureUnit &operator=(const MeasureUnit &other);

    




    virtual UObject* clone() const;

    



    virtual ~MeasureUnit();

    




    virtual UBool operator==(const UObject& other) const;

    




    UBool operator!=(const UObject& other) const {
        return !(*this == other);
    }

    



    const char *getType() const;

    



    const char *getSubtype() const;

    










    static int32_t getAvailable(
            MeasureUnit *destArray,
            int32_t destCapacity,
            UErrorCode &errorCode);

    











    static int32_t getAvailable(
            const char *type,
            MeasureUnit *destArray,
            int32_t destCapacity,
            UErrorCode &errorCode);

    







    static StringEnumeration* getAvailableTypes(UErrorCode &errorCode);

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

#ifndef U_HIDE_INTERNAL_API
    





    int32_t getIndex() const;

    




    static int32_t getIndexCount();

    



    static MeasureUnit *resolveUnitPerUnit(
            const MeasureUnit &unit, const MeasureUnit &perUnit);
#endif 









    





    static MeasureUnit *createGForce(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMeterPerSecondSquared(UErrorCode &status);

#endif 
    





    static MeasureUnit *createArcMinute(UErrorCode &status);

    





    static MeasureUnit *createArcSecond(UErrorCode &status);

    





    static MeasureUnit *createDegree(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createRadian(UErrorCode &status);

#endif 
    





    static MeasureUnit *createAcre(UErrorCode &status);

    





    static MeasureUnit *createHectare(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createSquareCentimeter(UErrorCode &status);

#endif 
    





    static MeasureUnit *createSquareFoot(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createSquareInch(UErrorCode &status);

#endif 
    





    static MeasureUnit *createSquareKilometer(UErrorCode &status);

    





    static MeasureUnit *createSquareMeter(UErrorCode &status);

    





    static MeasureUnit *createSquareMile(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createSquareYard(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createLiterPerKilometer(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMilePerGallon(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createBit(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createByte(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createGigabit(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createGigabyte(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilobit(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilobyte(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMegabit(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMegabyte(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createTerabit(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createTerabyte(UErrorCode &status);

#endif 
    





    static MeasureUnit *createDay(UErrorCode &status);

    





    static MeasureUnit *createHour(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMicrosecond(UErrorCode &status);

#endif 
    





    static MeasureUnit *createMillisecond(UErrorCode &status);

    





    static MeasureUnit *createMinute(UErrorCode &status);

    





    static MeasureUnit *createMonth(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createNanosecond(UErrorCode &status);

#endif 
    





    static MeasureUnit *createSecond(UErrorCode &status);

    





    static MeasureUnit *createWeek(UErrorCode &status);

    





    static MeasureUnit *createYear(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createAmpere(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMilliampere(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createOhm(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createVolt(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCalorie(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createFoodcalorie(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createJoule(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilocalorie(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilojoule(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilowattHour(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createGigahertz(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createHertz(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKilohertz(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMegahertz(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createAstronomicalUnit(UErrorCode &status);

#endif 
    





    static MeasureUnit *createCentimeter(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createDecimeter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createFathom(UErrorCode &status);

#endif 
    





    static MeasureUnit *createFoot(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createFurlong(UErrorCode &status);

#endif 
    





    static MeasureUnit *createInch(UErrorCode &status);

    





    static MeasureUnit *createKilometer(UErrorCode &status);

    





    static MeasureUnit *createLightYear(UErrorCode &status);

    





    static MeasureUnit *createMeter(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMicrometer(UErrorCode &status);

#endif 
    





    static MeasureUnit *createMile(UErrorCode &status);

    





    static MeasureUnit *createMillimeter(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createNanometer(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createNauticalMile(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createParsec(UErrorCode &status);

#endif 
    





    static MeasureUnit *createPicometer(UErrorCode &status);

    





    static MeasureUnit *createYard(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createLux(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCarat(UErrorCode &status);

#endif 
    





    static MeasureUnit *createGram(UErrorCode &status);

    





    static MeasureUnit *createKilogram(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMetricTon(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMicrogram(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMilligram(UErrorCode &status);

#endif 
    





    static MeasureUnit *createOunce(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createOunceTroy(UErrorCode &status);

#endif 
    





    static MeasureUnit *createPound(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createStone(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createTon(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createGigawatt(UErrorCode &status);

#endif 
    





    static MeasureUnit *createHorsepower(UErrorCode &status);

    





    static MeasureUnit *createKilowatt(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMegawatt(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMilliwatt(UErrorCode &status);

#endif 
    





    static MeasureUnit *createWatt(UErrorCode &status);

    





    static MeasureUnit *createHectopascal(UErrorCode &status);

    





    static MeasureUnit *createInchHg(UErrorCode &status);

    





    static MeasureUnit *createMillibar(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMillimeterOfMercury(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createPoundPerSquareInch(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKarat(UErrorCode &status);

#endif 
    





    static MeasureUnit *createKilometerPerHour(UErrorCode &status);

    





    static MeasureUnit *createMeterPerSecond(UErrorCode &status);

    





    static MeasureUnit *createMilePerHour(UErrorCode &status);

    





    static MeasureUnit *createCelsius(UErrorCode &status);

    





    static MeasureUnit *createFahrenheit(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createKelvin(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createAcreFoot(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createBushel(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCentiliter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCubicCentimeter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCubicFoot(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCubicInch(UErrorCode &status);

#endif 
    





    static MeasureUnit *createCubicKilometer(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCubicMeter(UErrorCode &status);

#endif 
    





    static MeasureUnit *createCubicMile(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCubicYard(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createCup(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createDeciliter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createFluidOunce(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createGallon(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createHectoliter(UErrorCode &status);

#endif 
    





    static MeasureUnit *createLiter(UErrorCode &status);

#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMegaliter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createMilliliter(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createPint(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createQuart(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createTablespoon(UErrorCode &status);

#endif 
#ifndef U_HIDE_DRAFT_API
    





    static MeasureUnit *createTeaspoon(UErrorCode &status);

#endif 



 protected:

#ifndef U_HIDE_INTERNAL_API
    



    void initTime(const char *timeId);

    



    void initCurrency(const char *isoCurrency);

#endif  

private:
    int32_t fTypeId;
    int32_t fSubTypeId;
    char fCurrency[4];

    MeasureUnit(int32_t typeId, int32_t subTypeId) : fTypeId(typeId), fSubTypeId(subTypeId) {
        fCurrency[0] = 0;
    }
    void setTo(int32_t typeId, int32_t subTypeId);
    int32_t getOffset() const;
    static MeasureUnit *create(int typeId, int subTypeId, UErrorCode &status);
};

U_NAMESPACE_END

#endif 
#endif 
