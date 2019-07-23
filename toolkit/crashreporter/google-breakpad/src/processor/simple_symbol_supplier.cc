


































#include "processor/simple_symbol_supplier.h"
#include "google_airbag/processor/minidump.h"
#include "processor/pathname_stripper.h"

namespace google_airbag {

string SimpleSymbolSupplier::GetSymbolFileAtPath(MinidumpModule *module,
                                                 const string &root_path) {
  
  

  if (!module)
    return "";

  const MDCVInfoPDB70 *cv_record =
      reinterpret_cast<const MDCVInfoPDB70*>(module->GetCVRecord());
  if (!cv_record)
    return "";

  if (cv_record->cv_signature != MD_CVINFOPDB70_SIGNATURE)
    return "";

  
  string path = root_path;

  
  path.append("/");
  string pdb_file_name = PathnameStripper::File(
      reinterpret_cast<const char *>(cv_record->pdb_file_name));
  path.append(pdb_file_name);

  
  path.append("/");
  char uuid_age_string[43];
  snprintf(uuid_age_string, sizeof(uuid_age_string),
           "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
           cv_record->signature.data1, cv_record->signature.data2,
           cv_record->signature.data3,
           cv_record->signature.data4[0], cv_record->signature.data4[1],
           cv_record->signature.data4[2], cv_record->signature.data4[3],
           cv_record->signature.data4[4], cv_record->signature.data4[5],
           cv_record->signature.data4[6], cv_record->signature.data4[7],
           cv_record->age);
  path.append(uuid_age_string);

  
  
  
  path.append("/");
  string pdb_file_extension = pdb_file_name.substr(pdb_file_name.size() - 4);
  transform(pdb_file_extension.begin(), pdb_file_extension.end(),
            pdb_file_extension.begin(), tolower);
  if (pdb_file_extension == ".pdb") {
    path.append(pdb_file_name.substr(0, pdb_file_name.size() - 4));
  } else {
    path.append(pdb_file_name);
  }
  path.append(".sym");

  return path;
}

}  
