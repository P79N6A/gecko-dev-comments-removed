










#include "tzfile.h" 

#ifdef WIN32

 #include <windows.h>
 #undef min // windows.h/STL conflict
 #undef max // windows.h/STL conflict
 
 #pragma warning(disable: 4786)

#else

 #include <unistd.h>
 #include <stdio.h>
 #include <dirent.h>
 #include <string.h>
 #include <sys/stat.h>

#endif

#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "tz2icu.h"
#include "unicode/uversion.h"

using namespace std;

bool ICU44PLUS = TRUE;
string TZ_RESOURCE_NAME = ICU_TZ_RESOURCE;





const int64_t SECS_PER_YEAR      = 31536000; 
const int64_t SECS_PER_LEAP_YEAR = 31622400; 
const int64_t LOWEST_TIME32    = (int64_t)((int32_t)0x80000000);
const int64_t HIGHEST_TIME32    = (int64_t)((int32_t)0x7fffffff);

bool isLeap(int32_t y) {
    return (y%4 == 0) && ((y%100 != 0) || (y%400 == 0)); 
}

int64_t secsPerYear(int32_t y) {
    return isLeap(y) ? SECS_PER_LEAP_YEAR : SECS_PER_YEAR;
}





int64_t yearToSeconds(int32_t year) {
    
    int64_t s = 0;
    int32_t y = 1970;
    while (y < year) {
        s += secsPerYear(y++);
    }
    while (y > year) {
        s -= secsPerYear(--y);
    }
    return s;
}





int32_t secondsToYear(int64_t seconds) {
    
    int32_t y = 1970;
    int64_t s = 0;
    if (seconds >= 0) {
        for (;;) {
            s += secsPerYear(y++);
            if (s > seconds) break;
        }
        --y;
    } else {
        for (;;) {
            s -= secsPerYear(--y);
            if (s <= seconds) break;
        }
    }
    return y;
}





struct FinalZone;
struct FinalRule;
struct SimplifiedZoneType;



struct Transition {
    int64_t time;  
    int32_t  type;  
    Transition(int64_t _time, int32_t _type) {
        time = _time;
        type = _type;
    }
};




struct ZoneType {
    int64_t rawoffset; 
    int64_t dstoffset; 

    
    
    int32_t  abbr;      
    bool isdst;
    bool isstd;
    bool isgmt;
    
    ZoneType(const SimplifiedZoneType&); 

    ZoneType() : rawoffset(-1), dstoffset(-1), abbr(-1) {}

    
    bool matches(const ZoneType& other) {
        return rawoffset == other.rawoffset &&
            dstoffset == other.dstoffset;
    }
};







struct ZoneInfo {
    vector<Transition> transitions;
    vector<ZoneType>   types;
    vector<string>     abbrs;

    string finalRuleID;
    int32_t finalOffset;
    int32_t finalYear; 

    
    
    int32_t aliasTo; 

    
    
    set<int32_t> aliases;

    ZoneInfo() : finalYear(-1), aliasTo(-1) {}

    void mergeFinalData(const FinalZone& fz);

    void optimizeTypeList();

    
    void setAliasTo(int32_t index);

    
    void clearAliases();

    
    void addAlias(int32_t index);

    
    bool isAlias() const {
        return aliasTo >= 0;
    }

    
    const set<int32_t>& getAliases() const {
        return aliases;
    }

    void print(ostream& os, const string& id) const;
};

void ZoneInfo::clearAliases() {
    assert(aliasTo < 0);
    aliases.clear();
}

void ZoneInfo::addAlias(int32_t index) {
    assert(aliasTo < 0 && index >= 0 && aliases.find(index) == aliases.end());
    aliases.insert(index);
}

void ZoneInfo::setAliasTo(int32_t index) {
    assert(index >= 0);
    assert(aliases.size() == 0);
    aliasTo = index;
}

typedef map<string, ZoneInfo> ZoneMap;

typedef ZoneMap::const_iterator ZoneMapIter;






ZoneMap ZONEINFO;






int64_t readcoded(ifstream& file, int64_t minv=numeric_limits<int64_t>::min(),
                               int64_t maxv=numeric_limits<int64_t>::max()) {
    unsigned char buf[4]; 
    int64_t val=0;
    file.read((char*)buf, 4);
    for(int32_t i=0,shift=24;i<4;++i,shift-=8) {
        val |= buf[i] << shift;
    }
    if (val < minv || val > maxv) {
        ostringstream os;
        os << "coded value out-of-range: " << val << ", expected ["
           << minv << ", " << maxv << "]";
        throw out_of_range(os.str());
    }
    return val;
}


int64_t readcoded64(ifstream& file, int64_t minv=numeric_limits<int64_t>::min(),
                               int64_t maxv=numeric_limits<int64_t>::max()) {
    unsigned char buf[8]; 
    int64_t val=0;
    file.read((char*)buf, 8);
    for(int32_t i=0,shift=56;i<8;++i,shift-=8) {
        val |= (int64_t)buf[i] << shift;
    }
    if (val < minv || val > maxv) {
        ostringstream os;
        os << "coded value out-of-range: " << val << ", expected ["
           << minv << ", " << maxv << "]";
        throw out_of_range(os.str());
    }
    return val;
}


bool readbool(ifstream& file) {
    char c;
    file.read(&c, 1);
    if (c!=0 && c!=1) {
        ostringstream os;
        os << "boolean value out-of-range: " << (int32_t)c;
        throw out_of_range(os.str());
    }
    return (c!=0);
}





void readzoneinfo(ifstream& file, ZoneInfo& info, bool is64bitData) {
    int32_t i;

    
    
    
    
    
    
    char buf[32];
    file.read(buf, 4);
    if (strncmp(buf, TZ_ICU_MAGIC, 4) != 0) {
        throw invalid_argument("TZ_ICU_MAGIC signature missing");
    }
    
    file.read(buf, 1);
    
    
    if(buf[0]!=0 && buf[0]!='2') {
      throw invalid_argument("Bad Olson version info");
    }

    
    file.read(buf, 15);
    if (*(ICUZoneinfoVersion*)&buf != TZ_ICU_VERSION) {
        throw invalid_argument("File version mismatch");
    }

    
    int64_t isgmtcnt = readcoded(file, 0);
    int64_t isdstcnt = readcoded(file, 0);
    int64_t leapcnt  = readcoded(file, 0);
    int64_t timecnt  = readcoded(file, 0);
    int64_t typecnt  = readcoded(file, 0);
    int64_t charcnt  = readcoded(file, 0);

    
    
    
    if (isgmtcnt != typecnt || isdstcnt != typecnt) {
        throw invalid_argument("count mismatch between tzh_ttisgmtcnt, tzh_ttisdstcnt, tth_typecnt");
    }

    
    
    
    vector<int64_t> transitionTimes(timecnt, -1); 
    vector<int32_t>  transitionTypes(timecnt, -1); 

    
    for (i=0; i<timecnt; ++i) {
        if (is64bitData) {
            transitionTimes[i] = readcoded64(file);
        } else {
            transitionTimes[i] = readcoded(file);
        }
    }

    
    for (i=0; i<timecnt; ++i) {
        unsigned char c;
        file.read((char*) &c, 1);
        int32_t t = (int32_t) c;
        if (t < 0 || t >= typecnt) {
            ostringstream os;
            os << "illegal type: " << t << ", expected [0, " << (typecnt-1) << "]";
            throw out_of_range(os.str());
        }
        transitionTypes[i] = t;
    }

    
    bool insertInitial = false;
    if (is64bitData && !ICU44PLUS) {
        if (timecnt > 0) {
            int32_t minidx = -1;
            for (i=0; i<timecnt; ++i) {
                if (transitionTimes[i] < LOWEST_TIME32) {
                    if (minidx == -1 || transitionTimes[i] > transitionTimes[minidx]) {
                        
                        minidx = i;
                    }
                } else if (transitionTimes[i] > HIGHEST_TIME32) {
                    
                    
                    
                    break;
                } else {
                    info.transitions.push_back(Transition(transitionTimes[i], transitionTypes[i]));
                }
            }
    
            if (minidx != -1) {
                
                
                vector<Transition>::iterator itr = info.transitions.begin();
                info.transitions.insert(itr, Transition(LOWEST_TIME32, transitionTypes[minidx]));
            } else {
                
                insertInitial = true;
            }
        }
    } else {
        for (i=0; i<timecnt; ++i) {
            info.transitions.push_back(Transition(transitionTimes[i], transitionTypes[i]));
        }
    }

    
    for (i=0; i<typecnt; ++i) { 
        ZoneType type;

        type.rawoffset = readcoded(file);
        type.dstoffset = readcoded(file);
        type.isdst = readbool(file);

        unsigned char c;
        file.read((char*) &c, 1);
        type.abbr = (int32_t) c;

        if (type.isdst != (type.dstoffset != 0)) {
            throw invalid_argument("isdst does not reflect dstoffset");
        }

        info.types.push_back(type);
    }

    assert(info.types.size() == (unsigned) typecnt);

    if (insertInitial) {
        assert(timecnt > 0);
        assert(typecnt > 0);

        int32_t initialTypeIdx = -1;

        
        if (info.types.at(0).dstoffset != 0) {
            
            
            int64_t rawoffset0 = (info.types.at(info.transitions.at(0).type)).rawoffset;    
            
            for (i=0; i<(int32_t)info.types.size(); ++i) {
                if (info.types.at(i).rawoffset == rawoffset0
                        && info.types.at(i).dstoffset == 0) {
                    initialTypeIdx = i;
                    break;
                }
            }
        } else {
            initialTypeIdx = 0;
        }
        assert(initialTypeIdx >= 0);
        
        vector<Transition>::iterator itr = info.transitions.begin();
        info.transitions.insert(itr, Transition(LOWEST_TIME32, initialTypeIdx));
    }


    
    if (charcnt) {
        
        
        char* str = new char[charcnt + 8];
        file.read(str, charcnt);

        
        
        vector<int32_t> abbroffset;
        char *limit=str+charcnt;
        for (char* p=str; p<limit; ++p) {
            char* start = p;
            while (*p != 0) ++p;
            info.abbrs.push_back(string(start, p-start));
            abbroffset.push_back(start-str);
        }

        
        
        

        
        vector<bool> abbrseen(abbroffset.size(), false);

        for (vector<ZoneType>::iterator it=info.types.begin();
             it!=info.types.end();
             ++it) {
            vector<int32_t>::const_iterator x=
                find(abbroffset.begin(), abbroffset.end(), it->abbr);
            if (x==abbroffset.end()) {
                
                
                
                
                
                
#if 0                
                
                
                ostringstream os;
                os << "Warning: unusual abbr offset " << it->abbr
                   << ", expected one of";
                for (vector<int32_t>::const_iterator y=abbroffset.begin();
                     y!=abbroffset.end(); ++y) {
                    os << ' ' << *y;
                }
                cerr << os.str() << "; using 0" << endl;
#endif
                it->abbr = 0;
            } else {
                int32_t index = x - abbroffset.begin();
                it->abbr = index;
                abbrseen[index] = true;
            }
        }

        for (int32_t ii=0;ii<(int32_t) abbrseen.size();++ii) {
            if (!abbrseen[ii]) {
                cerr << "Warning: unused abbreviation: " << ii << endl;
            }
        }
    }

    
    
    for (i=0; i<leapcnt; ++i) {
        readcoded(file); 
        readcoded(file); 
    }

    
    for (i=0; i<typecnt; ++i) info.types[i].isstd = readbool(file);

    
    for (i=0; i<typecnt; ++i) info.types[i].isgmt = readbool(file);
}










void handleFile(string path, string id) {
    
    if (ZONEINFO.find(id) != ZONEINFO.end()) {
        ostringstream os;
        os << "duplicate zone ID: " << id;
        throw invalid_argument(os.str());
    }

    ifstream file(path.c_str(), ios::in | ios::binary);
    if (!file) {
        throw invalid_argument("can't open file");
    }

    
    ZoneInfo info;
    readzoneinfo(file, info, false);

    
    if (!file) {
        throw invalid_argument("read error");
    }

    
    ZoneInfo info64;
    readzoneinfo(file, info64, true);

    bool alldone = false;
    int64_t eofPos = (int64_t) file.tellg();

    
    char ch = file.get();
    if (ch == 0x0a) {
        bool invalidchar = false;
        while (file.get(ch)) {
            if (ch == 0x0a) {
                break;
            }
            if (ch < 0x20) {
                
                invalidchar = true;
                break;
            }
        }
        if (!invalidchar) {
            eofPos = (int64_t) file.tellg();
            file.seekg(0, ios::end);
            eofPos = eofPos - (int64_t) file.tellg();
            if (eofPos == 0) {
                alldone = true;
            }
        }
    }
    if (!alldone) {
        ostringstream os;
        os << (-eofPos) << " unprocessed bytes at end";
        throw invalid_argument(os.str());
    }

    ZONEINFO[id] = info64;
}







#ifdef WIN32

void scandir(string dirname, string prefix="") {
    HANDLE          hList;
    WIN32_FIND_DATA FileData;
    
    
    hList = FindFirstFile((dirname + "\\*").c_str(), &FileData);
    if (hList == INVALID_HANDLE_VALUE) {
        cerr << "Error: Invalid directory: " << dirname << endl;
        exit(1);
    }
    for (;;) {
        string name(FileData.cFileName);
        string path(dirname + "\\" + name);
        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (name != "." && name != "..") {
                scandir(path, prefix + name + "/");
            }
        } else {
            try {
                string id = prefix + name;
                handleFile(path, id);
            } catch (const exception& e) {
                cerr << "Error: While processing \"" << path << "\", "
                     << e.what() << endl;
                exit(1);
            }
        }
        
        if (!FindNextFile(hList, &FileData)) {
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                break;
            } 
        }
    }
    FindClose(hList);
}

#else

void scandir(string dir, string prefix="") {
    DIR *dp;
    struct dirent *dir_entry;
    struct stat stat_info;
    char pwd[512];
    vector<string> subdirs;
    vector<string> subfiles;

    if ((dp = opendir(dir.c_str())) == NULL) {
        cerr << "Error: Invalid directory: " << dir << endl;
        exit(1);
    }
    if (!getcwd(pwd, sizeof(pwd))) {
        cerr << "Error: Directory name too long" << endl;
        exit(1);
    }
    chdir(dir.c_str());
    while ((dir_entry = readdir(dp)) != NULL) {
        string name = dir_entry->d_name;
        string path = dir + "/" + name;
        lstat(dir_entry->d_name,&stat_info);
        if (S_ISDIR(stat_info.st_mode)) {
            if (name != "." && name != "..") {
                subdirs.push_back(path);
                subdirs.push_back(prefix + name + "/");
                
            }
        } else {
            try {
                string id = prefix + name;
                subfiles.push_back(path);
                subfiles.push_back(id);
                
            } catch (const exception& e) {
                cerr << "Error: While processing \"" << path << "\", "
                     << e.what() << endl;
                exit(1);
            }
        }
    }
    closedir(dp);
    chdir(pwd);

    for(int32_t i=0;i<(int32_t)subfiles.size();i+=2) {
        try {
            handleFile(subfiles[i], subfiles[i+1]);
        } catch (const exception& e) {
            cerr << "Error: While processing \"" << subfiles[i] << "\", "
                 << e.what() << endl;
            exit(1);
        }
    }
    for(int32_t i=0;i<(int32_t)subdirs.size();i+=2) {
        scandir(subdirs[i], subdirs[i+1]);
    }
}

#endif








void consumeLine(istream& in) {
    int32_t c;
    do {
        c = in.get();
    } while (c != EOF && c != '\n');
}

enum {
    DOM = 0,
    DOWGEQ = 1,
    DOWLEQ = 2
};

const char* TIME_MODE[] = {"w", "s", "u"};



const int32_t MONTH_LEN[] = {31,29,31,30,31,30,31,31,30,31,30,31};

const int32_t HOUR = 3600;

struct FinalZone {
    int32_t offset; 
    int32_t year; 
    string ruleid;
    set<string> aliases;
    FinalZone(int32_t _offset, int32_t _year, const string& _ruleid) :
        offset(_offset), year(_year), ruleid(_ruleid)  {
        if (offset <= -16*HOUR || offset >= 16*HOUR) {
            ostringstream os;
            os << "Invalid input offset " << offset
               << " for year " << year
               << " and rule ID " << ruleid;
            throw invalid_argument(os.str());
        }
        if (year < 1900 || year >= 2050) {
            ostringstream os;
            os << "Invalid input year " << year
               << " with offset " << offset
               << " and rule ID " << ruleid;
            throw invalid_argument(os.str());
        }
    }
    FinalZone() : offset(-1), year(-1) {}
    void addLink(const string& alias) {
        if (aliases.find(alias) != aliases.end()) {
            ostringstream os;
            os << "Duplicate alias " << alias;
            throw invalid_argument(os.str());
        }
        aliases.insert(alias);
    }
};

struct FinalRulePart {
    int32_t mode;
    int32_t month;
    int32_t dom;
    int32_t dow;
    int32_t time;
    int32_t offset; 

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool isstd;
    bool isgmt;

    bool isset; 

    FinalRulePart() : isset(false) {}
    void set(const string& id,
             const string& _mode,
             int32_t _month,
             int32_t _dom,
             int32_t _dow,
             int32_t _time,
             bool _isstd,
             bool _isgmt,
             int32_t _offset) {
        if (isset) {
            throw invalid_argument("FinalRulePart set twice");
        }
        isset = true;
        if (_mode == "DOWLEQ") {
            mode = DOWLEQ;
        } else if (_mode == "DOWGEQ") {
            mode = DOWGEQ;
        } else if (_mode == "DOM") {
            mode = DOM;
        } else {
            throw invalid_argument("Unrecognized FinalRulePart mode");
        }
        month = _month;
        dom = _dom;
        dow = _dow;
        time = _time;
        isstd = _isstd;
        isgmt = _isgmt;
        offset = _offset;

        ostringstream os;
        if (month < 0 || month >= 12) {
            os << "Invalid input month " << month;
        }
        if (dom < 1 || dom > MONTH_LEN[month]) {
            os << "Invalid input day of month " << dom;
        }
        if (mode != DOM && (dow < 0 || dow >= 7)) {
            os << "Invalid input day of week " << dow;
        }
        if (offset < 0 || offset > HOUR) {
            os << "Invalid input offset " << offset;
        }
        if (isgmt && !isstd) {
            os << "Invalid input isgmt && !isstd";
        }
        if (!os.str().empty()) {
            os << " for rule "
               << id
               << _mode
               << month << dom << dow << time
               << isstd << isgmt
               << offset;
            throw invalid_argument(os.str());
        }
    }

    



    int32_t timemode() const {
        if (isgmt) {
            assert(isstd);
            return 2; 
        }
        if (isstd) {
            return 1; 
        }
        return 0; 
    }

    
    
    
    
    
    
    
    
    
    

    


    int32_t stz_dowim() const {
        return (mode == DOWLEQ) ? -dom : dom;
    }

    


    int32_t stz_dow() const {
        return (mode == DOM) ? 0 : -(dow+1);
    }
};

struct FinalRule {
    FinalRulePart part[2];

    bool isset() const {
        return part[0].isset && part[1].isset;
    }

    void print(ostream& os) const;
};

map<string,FinalZone> finalZones;
map<string,FinalRule> finalRules;

map<string, set<string> > links;
map<string, string> reverseLinks;
map<string, string> linkSource; 





bool isNotSet(const pair<const string,FinalRule>& p) {
    return !p.second.isset();
}





bool mapsToUnknownRule(const pair<const string,FinalZone>& p) {
    return finalRules.find(p.second.ruleid) == finalRules.end();
}







set<string> ruleIDset;

void insertRuleID(const pair<string,FinalRule>& p) {
    ruleIDset.insert(p.first);
}

void eraseRuleID(const pair<string,FinalZone>& p) {
    ruleIDset.erase(p.second.ruleid);
}




void readFinalZonesAndRules(istream& in) {

    for (;;) {
        string token;
        in >> token;
        if (in.eof() || !in) {
            break;
        } else if (token == "zone") {
            
            string id, ruleid;
            int32_t offset, year;
            in >> id >> offset >> year >> ruleid;
            consumeLine(in);
            finalZones[id] = FinalZone(offset, year, ruleid);
        } else if (token == "rule") {
            
            
            string id, mode;
            int32_t month, dom, dow, time, offset;
            bool isstd, isgmt;
            in >> id >> mode >> month >> dom >> dow >> time >> isstd >> isgmt >> offset;
            consumeLine(in);
            FinalRule& fr = finalRules[id];
            int32_t p = fr.part[0].isset ? 1 : 0;
            fr.part[p].set(id, mode, month, dom, dow, time, isstd, isgmt, offset);
        } else if (token == "link") {
            string fromid, toid; 
            in >> fromid >> toid;
            
            if (finalZones.find(toid) != finalZones.end()) {
                throw invalid_argument("Bad link: `to' id is a \"real\" zone");
            }

            links[fromid].insert(toid);
            reverseLinks[toid] = fromid;

            linkSource[fromid] = "Olson link";
            linkSource[toid] = "Olson link";
        } else if (token.length() > 0 && token[0] == '#') {
            consumeLine(in);
        } else {
            throw invalid_argument("Unrecognized keyword");
        }
    }

    if (!in.eof() && !in) {
        throw invalid_argument("Parse failure");
    }

    
    if (count_if(finalRules.begin(), finalRules.end(), isNotSet) != 0) {
        throw invalid_argument("One or more incomplete rule pairs");
    }

    
    if (count_if(finalZones.begin(), finalZones.end(), mapsToUnknownRule) != 0) {
        throw invalid_argument("One or more zones refers to an unknown rule");
    }

    
    ruleIDset.clear();
    for_each(finalRules.begin(), finalRules.end(), insertRuleID);
    for_each(finalZones.begin(), finalZones.end(), eraseRuleID);
    if (ruleIDset.size() != 0) {
        throw invalid_argument("Unused rules");
    }
}







void ZoneInfo::print(ostream& os, const string& id) const {
    
  os << "  /* " << id << " */ ";

    if (aliasTo >= 0) {
        assert(aliases.size() == 0);
        os << ":int { " << aliasTo << " } "; 
        return;
    }

    if (ICU44PLUS) {
        os << ":table {" << endl;
    } else {
        os << ":array {" << endl;
    }

    vector<Transition>::const_iterator trn;
    vector<ZoneType>::const_iterator typ;

    bool first;

    if (ICU44PLUS) {
        trn = transitions.begin();

        
        if (trn != transitions.end() && trn->time < LOWEST_TIME32) {
            os << "    transPre32:intvector { ";
            for (first = true; trn != transitions.end() && trn->time < LOWEST_TIME32; ++trn) {
                if (!first) {
                    os<< ", ";
                }
                first = false;
                os << (int32_t)(trn->time >> 32) << ", " << (int32_t)(trn->time & 0x00000000ffffffff);
            }
            os << " }" << endl;
        }

        
        if (trn != transitions.end() && trn->time < HIGHEST_TIME32) {
            os << "    trans:intvector { ";
            for (first = true; trn != transitions.end() && trn->time < HIGHEST_TIME32; ++trn) {
                if (!first) {
                    os << ", ";
                }
                first = false;
                os << trn->time;
            }
            os << " }" << endl;
        }

        
        if (trn != transitions.end()) {
            os << "    transPost32:intvector { ";
            for (first = true; trn != transitions.end(); ++trn) {
                if (!first) {
                    os<< ", ";
                }
                first = false;
                os << (int32_t)(trn->time >> 32) << ", " << (int32_t)(trn->time & 0x00000000ffffffff);
            }
            os << " }" << endl;
        }
    } else {
        os << "    :intvector { ";
        for (trn = transitions.begin(), first = true; trn != transitions.end(); ++trn) {
            if (!first) os << ", ";
            first = false;
            os << trn->time;
        }
        os << " }" << endl;
    }


    first=true;
    if (ICU44PLUS) {
        os << "    typeOffsets:intvector { ";
    } else {
        os << "    :intvector { ";
    }
    for (typ = types.begin(); typ != types.end(); ++typ) {
        if (!first) os << ", ";
        first = false;
        os << typ->rawoffset << ", " << typ->dstoffset;
    }
    os << " }" << endl;

    if (ICU44PLUS) {
        if (transitions.size() != 0) {
            os << "    typeMap:bin { \"" << hex << setfill('0');
            for (trn = transitions.begin(); trn != transitions.end(); ++trn) {
                os << setw(2) << trn->type;
            }
            os << dec << "\" }" << endl;
        }
    } else {
        os << "    :bin { \"" << hex << setfill('0');
        for (trn = transitions.begin(); trn != transitions.end(); ++trn) {
            os << setw(2) << trn->type;
        }
        os << dec << "\" }" << endl;
    }

    
    if (finalYear != -1) {
        if (ICU44PLUS) {
            os << "    finalRule { \"" << finalRuleID << "\" }" << endl;
            os << "    finalRaw:int { " << finalOffset << " }" << endl;
            os << "    finalYear:int { " << finalYear << " }" << endl;
        } else {
            os << "    \"" << finalRuleID << "\"" << endl;
            os << "    :intvector { " << finalOffset << ", "
               << finalYear << " }" << endl;
        }
    }

    
    if (aliases.size() != 0) {
        first = true;
        if (ICU44PLUS) {
            os << "    links:intvector { ";
        } else {
            os << "    :intvector { ";
        }
        for (set<int32_t>::const_iterator i=aliases.begin(); i!=aliases.end(); ++i) {
            if (!first) os << ", ";
            first = false;
            os << *i;
        }
        os << " }" << endl;
    }

    os << "  } "; 
}

inline ostream&
operator<<(ostream& os, const ZoneMap& zoneinfo) {
    int32_t c = 0;
    for (ZoneMapIter it = zoneinfo.begin();
         it != zoneinfo.end();
         ++it) {
        if(c && !ICU44PLUS)  os << ",";
        it->second.print(os, it->first);
        os << "//Z#" << c++ << endl;
    }
    return os;
}


ostream& printStringList( ostream& os, const ZoneMap& zoneinfo) {
  int32_t n = 0; 
  int32_t col = 0; 
  os << " Names {" << endl
     << "    ";
  for (ZoneMapIter it = zoneinfo.begin();
       it != zoneinfo.end();
       ++it) {
    if(n) {
      os << ",";
      col ++;
    }
    const string& id = it->first;
    os << "\"" << id << "\"";
    col += id.length() + 2;
    if(col >= 50) {
      os << " // " << n << endl
         << "    ";
      col = 0;
    }
    n++;
  }
  os << " // " << (n-1) << endl
     << " }" << endl;

  return os;
}






bool isAfter(const Transition t, int64_t thresh) {
    return t.time >= thresh;
}





struct SimplifiedZoneType {
    int64_t rawoffset;
    int64_t dstoffset;
    SimplifiedZoneType() : rawoffset(-1), dstoffset(-1) {}
    SimplifiedZoneType(const ZoneType& t) : rawoffset(t.rawoffset),
                                            dstoffset(t.dstoffset) {}
    bool operator<(const SimplifiedZoneType& t) const {
        return rawoffset < t.rawoffset ||
            (rawoffset == t.rawoffset &&
             dstoffset < t.dstoffset);
    }
};








ZoneType::ZoneType(const SimplifiedZoneType& t) :
    rawoffset(t.rawoffset), dstoffset(t.dstoffset),
    abbr(-1), isdst(false), isstd(false), isgmt(false) {}









void ZoneInfo::optimizeTypeList() {
    
    
    
    

    if (aliasTo >= 0) return; 

    if (!ICU44PLUS) {
        
        
        

        
        if (transitions.size() == 0) {
            if (types.size() != 1) {
                cerr << "Error: transition count = 0, type count = " << types.size() << endl;
            }
            return;
        }

        set<SimplifiedZoneType> simpleset;
        for (vector<Transition>::const_iterator i=transitions.begin();
             i!=transitions.end(); ++i) {
            assert(i->type < (int32_t)types.size());
            simpleset.insert(types[i->type]);
        }

        
        map<SimplifiedZoneType,int32_t> simplemap;
        int32_t n=0;
        for (set<SimplifiedZoneType>::const_iterator i=simpleset.begin();
             i!=simpleset.end(); ++i) {
            simplemap[*i] = n++;
        }

        
        for (vector<Transition>::iterator i=transitions.begin();
             i!=transitions.end(); ++i) {
            assert(i->type < (int32_t)types.size());
            ZoneType oldtype = types[i->type];
            SimplifiedZoneType newtype(oldtype);
            assert(simplemap.find(newtype) != simplemap.end());
            i->type = simplemap[newtype];
        }

        
        types.clear();
        copy(simpleset.begin(), simpleset.end(), back_inserter(types));

    } else {
        if (types.size() > 1) {
            
            

            
            ZoneType initialType = types[0];
            for (vector<ZoneType>::const_iterator i=types.begin(); i!=types.end(); ++i) {
                if (i->dstoffset == 0) {
                    initialType = *i;
                    break;
                }
            }

            SimplifiedZoneType initialSimplifiedType(initialType);

            
            set<SimplifiedZoneType> simpleset;
            simpleset.insert(initialSimplifiedType);
            for (vector<Transition>::const_iterator i=transitions.begin(); i!=transitions.end(); ++i) {
                assert(i->type < (int32_t)types.size());
                simpleset.insert(types[i->type]);
            }

            
            map<SimplifiedZoneType,int32_t> simplemap;
            simplemap[initialSimplifiedType] = 0;
            int32_t n = 1;
            for (set<SimplifiedZoneType>::const_iterator i=simpleset.begin(); i!=simpleset.end(); ++i) {
                if (*i < initialSimplifiedType || initialSimplifiedType < *i) {
                    simplemap[*i] = n++;
                }
            }

            
            for (vector<Transition>::iterator i=transitions.begin();
                 i!=transitions.end(); ++i) {
                assert(i->type < (int32_t)types.size());
                ZoneType oldtype = types[i->type];
                SimplifiedZoneType newtype(oldtype);
                assert(simplemap.find(newtype) != simplemap.end());
                i->type = simplemap[newtype];
            }

            
            types.clear();
            types.push_back(initialSimplifiedType);
            for (set<SimplifiedZoneType>::const_iterator i=simpleset.begin(); i!=simpleset.end(); ++i) {
                if (*i < initialSimplifiedType || initialSimplifiedType < *i) {
                    types.push_back(*i);
                }
            }

            
            
            int32_t prevTypeIdx = 0;
            for (vector<Transition>::iterator i=transitions.begin(); i!=transitions.end();) {
                if (i->type == prevTypeIdx) {
                    
                    
                    transitions.erase(i);
                } else {
                    prevTypeIdx = i->type;
                    i++;
                }
            }
        }
    }

}




void ZoneInfo::mergeFinalData(const FinalZone& fz) {
    int32_t year = fz.year;
    int64_t seconds = yearToSeconds(year);

    if (!ICU44PLUS) {
        if (seconds > HIGHEST_TIME32) {
            
            
            
            
            seconds = HIGHEST_TIME32;
        }
    }

    vector<Transition>::iterator it =
        find_if(transitions.begin(), transitions.end(),
                bind2nd(ptr_fun(isAfter), seconds));
    transitions.erase(it, transitions.end());

    if (finalYear != -1) {
        throw invalid_argument("Final zone already merged in");
    }
    finalYear = fz.year;
    finalOffset = fz.offset;
    finalRuleID = fz.ruleid;
}





void mergeOne(const string& zoneid, const FinalZone& fz) {
    if (ZONEINFO.find(zoneid) == ZONEINFO.end()) {
        throw invalid_argument("Unrecognized final zone ID");
    }
    ZONEINFO[zoneid].mergeFinalData(fz);
}






void mergeFinalZone(const pair<string,FinalZone>& p) {
    const string& id = p.first;
    const FinalZone& fz = p.second;

    mergeOne(id, fz);
}





void FinalRule::print(ostream& os) const {
    
    
    int32_t whichpart = (part[0].offset != 0) ? 0 : 1;
    assert(part[whichpart].offset != 0);
    assert(part[1-whichpart].offset == 0);

    os << "    ";
    for (int32_t i=0; i<2; ++i) {
        const FinalRulePart& p = part[whichpart];
        whichpart = 1-whichpart;
        os << p.month << ", " << p.stz_dowim() << ", " << p.stz_dow() << ", "
           << p.time << ", " << p.timemode() << ", ";
    }
    os << part[whichpart].offset << endl;
}

int main(int argc, char *argv[]) {
    string rootpath, zonetab, version;
    bool validArgs = FALSE;

    if (argc == 4 || argc == 5) {
        validArgs = TRUE;
        rootpath = argv[1];
        zonetab = argv[2];
        version = argv[3];
        if (argc == 5) {
            if (strcmp(argv[4], "--old") == 0) {
                ICU44PLUS = FALSE;
                TZ_RESOURCE_NAME = ICU_TZ_RESOURCE_OLD;
            } else {
                validArgs = FALSE;
            }
        }
    }
    if (!validArgs) {
        cout << "Usage: tz2icu <dir> <cmap> <tzver> [--old]" << endl
             << " <dir>    path to zoneinfo file tree generated by" << endl
             << "          ICU-patched version of zic" << endl
             << " <cmap>   country map, from tzdata archive," << endl
             << "          typically named \"zone.tab\"" << endl
             << " <tzver>  version string, such as \"2003e\"" << endl
             << " --old    generating resource format before ICU4.4" << endl;
        exit(1);
    }

    cout << "Olson data version: " << version << endl;
    cout << "ICU 4.4+ format: " << (ICU44PLUS ? "Yes" : "No") << endl;

    try {
        ifstream finals(ICU_ZONE_FILE);
        if (finals) {
            readFinalZonesAndRules(finals);

            cout << "Finished reading " << finalZones.size()
                 << " final zones and " << finalRules.size()
                 << " final rules from " ICU_ZONE_FILE << endl;
        } else {
            cerr << "Error: Unable to open " ICU_ZONE_FILE << endl;
            return 1;
        }
    } catch (const exception& error) {
        cerr << "Error: While reading " ICU_ZONE_FILE ": " << error.what() << endl;
        return 1;
    }

    try {
        
        
        
        scandir(rootpath);
    } catch (const exception& error) {
        cerr << "Error: While scanning " << rootpath << ": " << error.what() << endl;
        return 1;
    }

    cout << "Finished reading " << ZONEINFO.size() << " zoneinfo files ["
         << (ZONEINFO.begin())->first << ".."
         << (--ZONEINFO.end())->first << "]" << endl;

    try {
        for_each(finalZones.begin(), finalZones.end(), mergeFinalZone);
    } catch (const exception& error) {
        cerr << "Error: While merging final zone data: " << error.what() << endl;
        return 1;
    }

    
    
    
    
    
    for (map<string,set<string> >::const_iterator i = links.begin();
         i!=links.end(); ++i) {
        const string& olson = i->first;
        const set<string>& aliases = i->second;
        if (ZONEINFO.find(olson) == ZONEINFO.end()) {
            cerr << "Error: Invalid " << linkSource[olson] << " to non-existent \""
                 << olson << "\"" << endl;
            return 1;
        }
        for (set<string>::const_iterator j=aliases.begin();
             j!=aliases.end(); ++j) {
            ZONEINFO[*j] = ZoneInfo();
        }
    }
 
    
    map<string,int32_t> zoneIDs;
    vector<string> zoneIDlist;
    int32_t z=0;
    for (ZoneMap::iterator i=ZONEINFO.begin(); i!=ZONEINFO.end(); ++i) {
        zoneIDs[i->first] = z++;
        zoneIDlist.push_back(i->first);
    }
    assert(z == (int32_t) ZONEINFO.size());

    
    
    map<string,set<string> > links2;
    map<string,string> reverse2;
    for (map<string,set<string> >::const_iterator i = links.begin();
         i!=links.end(); ++i) {
        string olson = i->first;
        while (reverseLinks.find(olson) != reverseLinks.end()) {
            olson = reverseLinks[olson];
        }
        for (set<string>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j) {
            links2[olson].insert(*j);
            reverse2[*j] = olson;
        }
    }
    links = links2;
    reverseLinks = reverse2;

    if (false) { 
        for (map<string,set<string> >::const_iterator i = links.begin();
             i!=links.end(); ++i) {
            cout << i->first << ": ";
            for (set<string>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j) {
                cout << *j << ", ";
            }
            cout << endl;
        }
    }

    
    for (map<string,set<string> >::const_iterator i = links.begin();
         i!=links.end(); ++i) {
        const string& olson = i->first;
        const set<string>& aliases = i->second;
        ZONEINFO[olson].clearAliases();
        ZONEINFO[olson].addAlias(zoneIDs[olson]);
        for (set<string>::const_iterator j=aliases.begin();
             j!=aliases.end(); ++j) {
            assert(zoneIDs.find(olson) != zoneIDs.end());
            assert(zoneIDs.find(*j) != zoneIDs.end());
            assert(ZONEINFO.find(*j) != ZONEINFO.end());
            ZONEINFO[*j].setAliasTo(zoneIDs[olson]);
            ZONEINFO[olson].addAlias(zoneIDs[*j]);
        }
    }

    
    for (ZoneMap::iterator i=ZONEINFO.begin(); i!=ZONEINFO.end(); ++i) {
        i->second.optimizeTypeList();
    }

    
    map<string, set<string> > countryMap;  
    map<string, string> reverseCountryMap; 
    try {
        ifstream f(zonetab.c_str());
        if (!f) {
            cerr << "Error: Unable to open " << zonetab << endl;
            return 1;
        }
        int32_t n = 0;
        string line;
        while (getline(f, line)) {
            string::size_type lb = line.find('#');
            if (lb != string::npos) {
                line.resize(lb); 
            }
            string country, coord, zone;
            istringstream is(line);
            is >> country >> coord >> zone;
            if (country.size() == 0) continue;
            if (country.size() != 2 || zone.size() < 1) {
                cerr << "Error: Can't parse " << line << " in " << zonetab << endl;
                return 1;
            }
            if (ZONEINFO.find(zone) == ZONEINFO.end()) {
                cerr << "Error: Country maps to invalid zone " << zone
                     << " in " << zonetab << endl;
                return 1;
            }
            countryMap[country].insert(zone);
            reverseCountryMap[zone] = country;
            
            ++n;
        }
        cout << "Finished reading " << n
             << " country entries from " << zonetab << endl;
    } catch (const exception& error) {
        cerr << "Error: While reading " << zonetab << ": " << error.what() << endl;
        return 1;
    }

    
    
    
    
    for (map<string,set<string> >::const_iterator i = links.begin();
         i!=links.end(); ++i) {
        const string& olson(i->first);
        if (reverseCountryMap.find(olson) == reverseCountryMap.end()) {
            continue;
        }
        string c = reverseCountryMap[olson];
        const set<string>& aliases(i->second);
        for (set<string>::const_iterator j=aliases.begin();
             j != aliases.end(); ++j) {
            if (reverseCountryMap.find(*j) == reverseCountryMap.end()) {
                countryMap[c].insert(*j);
                reverseCountryMap[*j] = c;
                
            }
        }
    }

    
    set<string> nocountry;
    for (ZoneMap::iterator i=ZONEINFO.begin(); i!=ZONEINFO.end(); ++i) {
        if (reverseCountryMap.find(i->first) == reverseCountryMap.end()) {
            nocountry.insert(i->first);
        }
    }
    countryMap[""] = nocountry;

    
    time_t sec;
    time(&sec);
    struct tm* now = localtime(&sec);
    int32_t thisYear = now->tm_year + 1900;

    string filename = TZ_RESOURCE_NAME + ".txt";
    
    
    ofstream file(filename.c_str());
    if (file) {
        file << "//---------------------------------------------------------" << endl
             << "// Copyright (C) 2003";
        if (thisYear > 2003) {
            file << "-" << thisYear;
        }
        file << ", International Business Machines" << endl
             << "// Corporation and others.  All Rights Reserved." << endl
             << "//---------------------------------------------------------" << endl
             << "// Build tool: tz2icu" << endl
             << "// Build date: " << asctime(now) 
             << "// Olson source: ftp://elsie.nci.nih.gov/pub/" << endl
             << "// Olson version: " << version << endl
             << "// ICU version: " << U_ICU_VERSION << endl
             << "//---------------------------------------------------------" << endl
             << "// >> !!! >>   THIS IS A MACHINE-GENERATED FILE   << !!! <<" << endl
             << "// >> !!! >>>            DO NOT EDIT             <<< !!! <<" << endl
             << "//---------------------------------------------------------" << endl
             << endl
             << TZ_RESOURCE_NAME << ":table(nofallback) {" << endl
             << " TZVersion { \"" << version << "\" }" << endl
             << " Zones:array { " << endl
             << ZONEINFO 
             << " }" << endl;

        
        printStringList ( file, ZONEINFO ); 

        
        file << " Rules { " << endl;
        
        int32_t frc = 0;
        for(map<string,FinalRule>::iterator i=finalRules.begin();
            i!=finalRules.end(); ++i) {
            const string& id = i->first;
            const FinalRule& r = i->second;
            file << "  " << id << ":intvector {" << endl;
            r.print(file);
            file << "  } //_#" << frc++ << endl;
        }
        file << " }" << endl;

        
        if (ICU44PLUS) {
            file << " Regions:array {" << endl;
            int32_t zn = 0;
            for (ZoneMap::iterator i=ZONEINFO.begin(); i!=ZONEINFO.end(); ++i) {
                map<string, string>::iterator cit = reverseCountryMap.find(i->first);
                if (cit == reverseCountryMap.end()) {
                    file << "  \"001\",";
                } else {
                    file << "  \"" << cit->second << "\", ";
                }
                file << "//Z#" << zn++ << " " << i->first << endl;
            }
            file << " }" << endl;
        } else {
            file << " Regions { " << endl;
            int32_t  rc = 0;
            for (map<string, set<string> >::const_iterator i=countryMap.begin();
                 i != countryMap.end(); ++i) {
                string country = i->first;
                const set<string>& zones(i->second);
                file << "  ";
                if(country[0]==0) {
                  file << "Default";
                }
                file << country << ":intvector { ";
                bool first = true;
                for (set<string>::const_iterator j=zones.begin();
                     j != zones.end(); ++j) {
                    if (!first) file << ", ";
                    first = false;
                    if (zoneIDs.find(*j) == zoneIDs.end()) {
                        cerr << "Error: Nonexistent zone in country map: " << *j << endl;
                        return 1;
                    }
                    file << zoneIDs[*j]; 
                }
                file << " } //R#" << rc++ << endl;
            }
            file << " }" << endl;
        }

        file << "}" << endl;
    }

    file.close();
     
    if (file) { 
        cout << "Finished writing " << TZ_RESOURCE_NAME << ".txt" << endl;
    } else {
        cerr << "Error: Unable to open/write to " << TZ_RESOURCE_NAME << ".txt" << endl;
        return 1;
    }
}

