








#ifndef SkOSMenu_DEFINED
#define SkOSMenu_DEFINED

#include "SkEvent.h"
#include "SkTDArray.h"

class SkOSMenu {
public:
    explicit SkOSMenu(const char title[] = "");
    ~SkOSMenu();
    
    












    enum Type {
        kAction_Type,
        kList_Type,
        kSlider_Type,
        kSwitch_Type,
        kTriState_Type,
        kTextField_Type,
        kCustom_Type
    };
    
    enum TriState {
        kMixedState = -1,
        kOffState = 0,
        kOnState = 1
    };
    
    class Item {
    public:
        



        Item(const char label[], SkOSMenu::Type type, const char slotName[], 
             SkEvent* evt);
        ~Item() { delete fEvent; }
        
        SkEvent*    getEvent() const { return fEvent; }
        int         getID() const { return fID; }
        const char* getLabel() const { return fLabel.c_str(); }
        const char* getSlotName() const { return fSlotName.c_str(); }
        Type        getType() const { return fType; }
        void        setKeyEquivalent(SkUnichar key) { fKey = key; }
        SkUnichar   getKeyEquivalent() const { return fKey; }
        
        


        void setBool(bool value) const;             
        void setScalar(SkScalar value) const;       
        void setInt(int value) const;               
        void setTriState(TriState value) const;     
        void setString(const char value[]) const;   
        
        



        void postEvent() const { (new SkEvent(*(fEvent)))->post(); }

    private:
        int             fID;
        SkEvent*        fEvent;
        SkString        fLabel;
        SkString        fSlotName;
        Type            fType;
        SkUnichar       fKey;
    };
    
    void        reset();
    const char* getTitle() const { return fTitle.c_str(); }
    void        setTitle (const char title[]) { fTitle.set(title); }
    int         getCount() const { return fItems.count(); }
    const Item* getItemByID(int itemID) const;
    void        getItems(const Item* items[]) const;
    
    



    void        assignKeyEquivalentToItem(int itemID, SkUnichar key);
    







    bool        handleKeyEquivalent(SkUnichar key);
    
    






    int appendItem(const char label[], Type type, const char slotName[], 
                   SkEvent* evt); 
    
    





    int appendAction(const char label[], SkEventSinkID target);
    int appendList(const char label[], const char slotName[], 
                   SkEventSinkID target, int defaultIndex, const char[] ...);
    int appendSlider(const char label[], const char slotName[], 
                     SkEventSinkID target, SkScalar min, SkScalar max, 
                     SkScalar defaultValue);
    int appendSwitch(const char label[], const char slotName[], 
                     SkEventSinkID target, bool defaultState = false);
    int appendTriState(const char label[], const char slotName[],
                       SkEventSinkID target, TriState defaultState = kOffState);
    int appendTextField(const char label[], const char slotName[],
                        SkEventSinkID target, const char placeholder[] = "");
    
    
    



    static bool FindListItemCount(const SkEvent& evt, int* count);
    



    static bool FindListItems(const SkEvent& evt, SkString items[]);
    static bool FindSliderMin(const SkEvent& evt, SkScalar* min);
    static bool FindSliderMax(const SkEvent& evt, SkScalar* max);
    
    


    static bool FindAction(const SkEvent& evt, const char label[]);
    





    static bool FindListIndex(const SkEvent& evt, const char slotName[], int* value);
    static bool FindSliderValue(const SkEvent& evt, const char slotName[], SkScalar* value);
    static bool FindSwitchState(const SkEvent& evt, const char slotName[], bool* value);
    static bool FindTriState(const SkEvent& evt, const char slotName[], TriState* value);
    static bool FindText(const SkEvent& evt, const char slotName[], SkString* value);
    
private:
    SkString fTitle;
    SkTDArray<Item*> fItems;
    
    
    SkOSMenu(const SkOSMenu&);
    SkOSMenu& operator=(const SkOSMenu&);
};

#endif

