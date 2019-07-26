

























#pragma once



#include "inc/Main.h"
#include "inc/FeatureVal.h"

namespace graphite2 {


class FeatureMap;
class Face;


class FeatureSetting
{
public:
    FeatureSetting(int16 theValue, uint16 labelId) : m_label(labelId), m_value(theValue) {};
    uint16 label() const { return m_label; }
    int16 value() const { return m_value; }
    
    CLASS_NEW_DELETE;
private:
    FeatureSetting(const FeatureSetting & fs) : m_label(fs.m_label), m_value(fs.m_value) {};

    uint16 m_label;
    int16 m_value;
};

class FeatureRef
{
	typedef uint32		chunk_t;
	static const uint8	SIZEOF_CHUNK = sizeof(chunk_t)*8;

public:
	FeatureRef() : m_nameValues(0) {}
    FeatureRef(const Face & face, unsigned short & bits_offset, uint32 max_val,
               uint32 name, uint16 uiName, uint16 flags,
               FeatureSetting *settings, uint16 num_set) throw();
    ~FeatureRef() throw();

    bool applyValToFeature(uint32 val, Features& pDest) const; 
    void maskFeature(Features & pDest) const {
	if (m_index < pDest.size()) 				
	    pDest[m_index] |= m_mask; 
    }

    uint32 getFeatureVal(const Features& feats) const; 

    uint32 getId() const { return m_id; }
    uint16 getNameId() const { return m_nameid; }
    uint16 getNumSettings() const { return m_numSet; }
    uint16 getSettingName(uint16 index) const { return m_nameValues[index].label(); }
    int16  getSettingValue(uint16 index) const { return m_nameValues[index].value(); }
    uint32 maxVal() const { return m_max; }
    const Face* getFace() const { return m_pFace;}
    const FeatureMap* getFeatureMap() const;

    CLASS_NEW_DELETE;
private:
    FeatureRef(const FeatureRef & rhs);

    const Face 	   * m_pFace;   
    FeatureSetting * m_nameValues; 
    chunk_t m_mask,             
    		m_max;              
    uint32 	m_id;               
    uint16 	m_nameid,            
    		m_flags,             
    		m_numSet;            
    byte 	m_bits,             
    	 	m_index;            

private:        
    FeatureRef& operator=(const FeatureRef&);
};


class NameAndFeatureRef
{
  public:
    NameAndFeatureRef(uint32 name = 0) : m_name(name) , m_pFRef(NULL){}
    NameAndFeatureRef(const FeatureRef* p) : m_name(p->getId()), m_pFRef(p) {}

    bool operator<(const NameAndFeatureRef& rhs) const 
        {   return m_name<rhs.m_name; }

    CLASS_NEW_DELETE
 
    uint32 m_name;
    const FeatureRef* m_pFRef;
};

class FeatureMap
{
public:
    FeatureMap() : m_numFeats(0), m_feats(NULL), m_pNamedFeats(NULL),
        m_defaultFeatures(NULL) {}
    ~FeatureMap() { delete [] m_feats; delete[] m_pNamedFeats; delete m_defaultFeatures; }

    bool readFeats(const Face & face);
    const FeatureRef *findFeatureRef(uint32 name) const;
    FeatureRef *feature(uint16 index) const { return m_feats + index; }
    
    const FeatureRef *featureRef(byte index) const { return index < m_numFeats ? m_feats + index : NULL; }
    FeatureVal* cloneFeatures(uint32 langname) const;      
    uint16 numFeats() const { return m_numFeats; };
    CLASS_NEW_DELETE
private:
friend class SillMap;
    uint16 m_numFeats;

    FeatureRef *m_feats;
    NameAndFeatureRef* m_pNamedFeats;   
    FeatureVal* m_defaultFeatures;        
    
private:		
    FeatureMap(const FeatureMap&);
    FeatureMap& operator=(const FeatureMap&);
};


class SillMap
{
private:
    class LangFeaturePair
    {
        LangFeaturePair(const LangFeaturePair &);
        LangFeaturePair & operator = (const LangFeaturePair &);

    public:
        LangFeaturePair() :  m_lang(0), m_pFeatures(0) {}
        ~LangFeaturePair() { delete m_pFeatures; }
        
        uint32 m_lang;
        Features* m_pFeatures;      
        CLASS_NEW_DELETE
    };
public:
    SillMap() : m_langFeats(NULL), m_numLanguages(0) {}
    ~SillMap() { delete[] m_langFeats; }
    bool readFace(const Face & face);
    bool readSill(const Face & face);
    FeatureVal* cloneFeatures(uint32 langname) const;      
    uint16 numLanguages() const { return m_numLanguages; };
    uint32 getLangName(uint16 index) const { return (index < m_numLanguages)? m_langFeats[index].m_lang : 0; };

    const FeatureMap & theFeatureMap() const { return m_FeatureMap; };
private:
    FeatureMap m_FeatureMap;        
    LangFeaturePair * m_langFeats;
    uint16 m_numLanguages;

private:        
    SillMap(const SillMap&);
    SillMap& operator=(const SillMap&);
};

} 

struct gr_feature_ref : public graphite2::FeatureRef {};
