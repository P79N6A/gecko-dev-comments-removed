




























#include <atlbase.h>
#include <dia2.h>
#include <ImageHlp.h>
#include <stdio.h>

#include "common/windows/string_utils-inl.h"

#include "common/windows/pdb_source_line_writer.h"
#include "common/windows/guid_string.h"



#ifndef UNDNAME_NO_ECSU
#define UNDNAME_NO_ECSU 0x8000  // Suppresses enum/class/struct/union.
#endif  

namespace google_breakpad {

using std::vector;


class AutoImage {
 public:
  explicit AutoImage(PLOADED_IMAGE img) : img_(img) {}
  ~AutoImage() {
    if (img_)
      ImageUnload(img_);
  }

  operator PLOADED_IMAGE() { return img_; }
  PLOADED_IMAGE operator->() { return img_; }

 private:
  PLOADED_IMAGE img_;
};

PDBSourceLineWriter::PDBSourceLineWriter() : output_(NULL) {
}

PDBSourceLineWriter::~PDBSourceLineWriter() {
}

bool PDBSourceLineWriter::Open(const wstring &file, FileFormat format) {
  Close();

  if (FAILED(CoInitialize(NULL))) {
    fprintf(stderr, "CoInitialize failed\n");
    return false;
  }

  CComPtr<IDiaDataSource> data_source;
  if (FAILED(data_source.CoCreateInstance(CLSID_DiaSource))) {
    const int kGuidSize = 64;
    wchar_t classid[kGuidSize] = {0};
    StringFromGUID2(CLSID_DiaSource, classid, kGuidSize);
    
    
    fprintf(stderr, "CoCreateInstance CLSID_DiaSource %S failed "
            "(msdia*.dll unregistered?)\n", classid);
    return false;
  }

  switch (format) {
    case PDB_FILE:
      if (FAILED(data_source->loadDataFromPdb(file.c_str()))) {
        fprintf(stderr, "loadDataFromPdb failed for %ws\n", file.c_str());
        return false;
      }
      break;
    case EXE_FILE:
      if (FAILED(data_source->loadDataForExe(file.c_str(), NULL, NULL))) {
        fprintf(stderr, "loadDataForExe failed for %ws\n", file.c_str());
        return false;
      }
      code_file_ = file;
      break;
    case ANY_FILE:
      if (FAILED(data_source->loadDataFromPdb(file.c_str()))) {
        if (FAILED(data_source->loadDataForExe(file.c_str(), NULL, NULL))) {
          fprintf(stderr, "loadDataForPdb and loadDataFromExe failed for %ws\n", file.c_str());
          return false;
        }
	code_file_ = file;
      }
      break;
    default:
      fprintf(stderr, "Unknown file format\n");
      return false;
  }

  if (FAILED(data_source->openSession(&session_))) {
    fprintf(stderr, "openSession failed\n");
  }

  return true;
}

bool PDBSourceLineWriter::PrintLines(IDiaEnumLineNumbers *lines) {
  
  
  CComPtr<IDiaLineNumber> line;
  ULONG count;

  while (SUCCEEDED(lines->Next(1, &line, &count)) && count == 1) {
    DWORD rva;
    if (FAILED(line->get_relativeVirtualAddress(&rva))) {
      fprintf(stderr, "failed to get line rva\n");
      return false;
    }

    DWORD length;
    if (FAILED(line->get_length(&length))) {
      fprintf(stderr, "failed to get line code length\n");
      return false;
    }

    DWORD dia_source_id;
    if (FAILED(line->get_sourceFileId(&dia_source_id))) {
      fprintf(stderr, "failed to get line source file id\n");
      return false;
    }
    
    DWORD source_id = GetRealFileID(dia_source_id);

    DWORD line_num;
    if (FAILED(line->get_lineNumber(&line_num))) {
      fprintf(stderr, "failed to get line number\n");
      return false;
    }

    fprintf(output_, "%x %x %d %d\n", rva, length, line_num, source_id);
    line.Release();
  }
  return true;
}

bool PDBSourceLineWriter::PrintFunction(IDiaSymbol *function,
                                        IDiaSymbol *block) {
  
  
  DWORD rva;
  if (FAILED(block->get_relativeVirtualAddress(&rva))) {
    fprintf(stderr, "couldn't get rva\n");
    return false;
  }

  ULONGLONG length;
  if (FAILED(block->get_length(&length))) {
    fprintf(stderr, "failed to get function length\n");
    return false;
  }

  if (length == 0) {
    
    return true;
  }

  CComBSTR name;
  int stack_param_size;
  if (!GetSymbolFunctionName(function, &name, &stack_param_size)) {
    return false;
  }

  
  
  if (stack_param_size < 0) {
    stack_param_size = GetFunctionStackParamSize(function);
  }

  fprintf(output_, "FUNC %x %" WIN_STRING_FORMAT_LL "x %x %ws\n",
          rva, length, stack_param_size, name);

  CComPtr<IDiaEnumLineNumbers> lines;
  if (FAILED(session_->findLinesByRVA(rva, DWORD(length), &lines))) {
    return false;
  }

  if (!PrintLines(lines)) {
    return false;
  }
  return true;
}

bool PDBSourceLineWriter::PrintSourceFiles() {
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComPtr<IDiaEnumSymbols> compilands;
  if (FAILED(global->findChildren(SymTagCompiland, NULL,
                                  nsNone, &compilands))) {
    fprintf(stderr, "findChildren failed\n");
    return false;
  }

  CComPtr<IDiaSymbol> compiland;
  ULONG count;
  while (SUCCEEDED(compilands->Next(1, &compiland, &count)) && count == 1) {
    CComPtr<IDiaEnumSourceFiles> source_files;
    if (FAILED(session_->findFile(compiland, NULL, nsNone, &source_files))) {
      return false;
    }
    CComPtr<IDiaSourceFile> file;
    while (SUCCEEDED(source_files->Next(1, &file, &count)) && count == 1) {
      DWORD file_id;
      if (FAILED(file->get_uniqueId(&file_id))) {
        return false;
      }

      CComBSTR file_name;
      if (FAILED(file->get_fileName(&file_name))) {
        return false;
      }

      wstring file_name_string(file_name);
      if (!FileIDIsCached(file_name_string)) {
        
        CacheFileID(file_name_string, file_id);
        fwprintf(output_, L"FILE %d %s\n", file_id, file_name);
      } else {
        
        
        StoreDuplicateFileID(file_name_string, file_id);
      }
      file.Release();
    }
    compiland.Release();
  }
  return true;
}

bool PDBSourceLineWriter::PrintFunctions() {
  CComPtr<IDiaEnumSymbolsByAddr> symbols;
  if (FAILED(session_->getSymbolsByAddr(&symbols))) {
    fprintf(stderr, "failed to get symbol enumerator\n");
    return false;
  }

  CComPtr<IDiaSymbol> symbol;
  if (FAILED(symbols->symbolByAddr(1, 0, &symbol))) {
    fprintf(stderr, "failed to enumerate symbols\n");
    return false;
  }

  DWORD rva_last = 0;
  if (FAILED(symbol->get_relativeVirtualAddress(&rva_last))) {
    fprintf(stderr, "failed to get symbol rva\n");
    return false;
  }

  ULONG count;
  do {
    DWORD tag;
    if (FAILED(symbol->get_symTag(&tag))) {
      fprintf(stderr, "failed to get symbol tag\n");
      return false;
    }

    
    
    
    
    if (tag == SymTagFunction) {
      if (!PrintFunction(symbol, symbol)) {
        return false;
      }
    } else if (tag == SymTagPublicSymbol) {
      if (!PrintCodePublicSymbol(symbol)) {
        return false;
      }
    }
    symbol.Release();
  } while (SUCCEEDED(symbols->Next(1, &symbol, &count)) && count == 1);

  
  
  
  
  
  
  
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComPtr<IDiaEnumSymbols> compilands;
  if (FAILED(global->findChildren(SymTagCompiland, NULL,
                                  nsNone, &compilands))) {
    fprintf(stderr, "findChildren failed on the global\n");
    return false;
  }

  CComPtr<IDiaSymbol> compiland;
  while (SUCCEEDED(compilands->Next(1, &compiland, &count)) && count == 1) {
    CComPtr<IDiaEnumSymbols> blocks;
    if (FAILED(compiland->findChildren(SymTagBlock, NULL,
                                       nsNone, &blocks))) {
      fprintf(stderr, "findChildren failed on a compiland\n");
      return false;
    }

    CComPtr<IDiaSymbol> block;
    while (SUCCEEDED(blocks->Next(1, &block, &count)) && count == 1) {
      
      CComPtr<IDiaSymbol> parent;
      DWORD tag;
      if (SUCCEEDED(block->get_lexicalParent(&parent)) &&
          SUCCEEDED(parent->get_symTag(&tag)) &&
          tag == SymTagFunction) {
        
        
        DWORD func_rva, block_rva;
        ULONGLONG func_length;
        if (SUCCEEDED(block->get_relativeVirtualAddress(&block_rva)) &&
            SUCCEEDED(parent->get_relativeVirtualAddress(&func_rva)) &&
            SUCCEEDED(parent->get_length(&func_length))) {
          if (block_rva < func_rva || block_rva > (func_rva + func_length)) {
            if (!PrintFunction(parent, block)) {
              return false;
            }
          }
        }
      }
      parent.Release();
      block.Release();
    }
    blocks.Release();
    compiland.Release();
  }

  return true;
}

bool PDBSourceLineWriter::PrintFrameData() {
  
  
  

  CComPtr<IDiaEnumTables> tables;
  if (FAILED(session_->getEnumTables(&tables)))
    return false;

  
  CComPtr<IDiaEnumFrameData> frame_data_enum;
  CComPtr<IDiaTable> table;
  ULONG count;
  while (!frame_data_enum &&
         SUCCEEDED(tables->Next(1, &table, &count)) &&
         count == 1) {
    table->QueryInterface(_uuidof(IDiaEnumFrameData),
                          reinterpret_cast<void**>(&frame_data_enum));
    table.Release();
  }
  if (!frame_data_enum)
    return false;

  DWORD last_type = -1;
  DWORD last_rva = -1;
  DWORD last_code_size = 0;
  DWORD last_prolog_size = -1;

  CComPtr<IDiaFrameData> frame_data;
  while (SUCCEEDED(frame_data_enum->Next(1, &frame_data, &count)) &&
         count == 1) {
    DWORD type;
    if (FAILED(frame_data->get_type(&type)))
      return false;

    DWORD rva;
    if (FAILED(frame_data->get_relativeVirtualAddress(&rva)))
      return false;

    DWORD code_size;
    if (FAILED(frame_data->get_lengthBlock(&code_size)))
      return false;

    DWORD prolog_size;
    if (FAILED(frame_data->get_lengthProlog(&prolog_size)))
      return false;

    
    DWORD epilog_size = 0;

    
    
    
    DWORD parameter_size;
    if (FAILED(frame_data->get_lengthParams(&parameter_size)))
      return false;

    DWORD saved_register_size;
    if (FAILED(frame_data->get_lengthSavedRegisters(&saved_register_size)))
      return false;

    DWORD local_size;
    if (FAILED(frame_data->get_lengthLocals(&local_size)))
      return false;

    
    DWORD max_stack_size = 0;
    if (FAILED(frame_data->get_maxStack(&max_stack_size)))
      return false;

    
    
    HRESULT program_string_result;
    CComBSTR program_string;
    if (FAILED(program_string_result = frame_data->get_program(
        &program_string))) {
      return false;
    }

    
    
    BOOL allocates_base_pointer = FALSE;
    if (program_string_result != S_OK) {
      if (FAILED(frame_data->get_allocatesBasePointer(
          &allocates_base_pointer))) {
        return false;
      }
    }

    
    
    
    
    
    if (type != last_type || rva != last_rva || code_size != last_code_size ||
        prolog_size != last_prolog_size) {
      fprintf(output_, "STACK WIN %x %x %x %x %x %x %x %x %x %d ",
              type, rva, code_size, prolog_size, epilog_size,
              parameter_size, saved_register_size, local_size, max_stack_size,
              program_string_result == S_OK);
      if (program_string_result == S_OK) {
        fprintf(output_, "%ws\n", program_string);
      } else {
        fprintf(output_, "%d\n", allocates_base_pointer);
      }

      last_type = type;
      last_rva = rva;
      last_code_size = code_size;
      last_prolog_size = prolog_size;
    }

    frame_data.Release();
  }

  return true;
}

bool PDBSourceLineWriter::PrintCodePublicSymbol(IDiaSymbol *symbol) {
  BOOL is_code;
  if (FAILED(symbol->get_code(&is_code))) {
    return false;
  }
  if (!is_code) {
    return true;
  }

  DWORD rva;
  if (FAILED(symbol->get_relativeVirtualAddress(&rva))) {
    return false;
  }

  CComBSTR name;
  int stack_param_size;
  if (!GetSymbolFunctionName(symbol, &name, &stack_param_size)) {
    return false;
  }

  fprintf(output_, "PUBLIC %x %x %ws\n", rva,
          stack_param_size > 0 ? stack_param_size : 0, name);
  return true;
}

bool PDBSourceLineWriter::PrintPDBInfo() {
  PDBModuleInfo info;
  if (!GetModuleInfo(&info)) {
    return false;
  }

  
  
  
  fprintf(output_, "MODULE windows %ws %ws %ws\n",
          info.cpu.c_str(), info.debug_identifier.c_str(),
          info.debug_file.c_str());

  return true;
}

bool PDBSourceLineWriter::PrintPEInfo() {
  PEModuleInfo info;
  if (!GetPEInfo(&info)) {
    return false;
  }

  fprintf(output_, "INFO CODE_ID %ws %ws\n",
	  info.code_identifier.c_str(),
	  info.code_file.c_str());
  return true;
}










static bool wcstol_positive_strict(wchar_t *string, int *result) {
  int value = 0;
  for (wchar_t *c = string; *c != '\0'; ++c) {
    int last_value = value;
    value *= 10;
    
    if (value / 10 != last_value || value < 0) {
      return false;
    }
    if (*c < '0' || *c > '9') {
      return false;
    }
    unsigned int c_value = *c - '0';
    last_value = value;
    value += c_value;
    
    if (value < last_value) {
      return false;
    }
    
    if (value == 0 && *(c+1) != '\0') {
      return false;
    }
  }
  *result = value;
  return true;
}

bool PDBSourceLineWriter::FindPEFile() {
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComBSTR symbols_file;
  if (SUCCEEDED(global->get_symbolsFileName(&symbols_file))) {
    wstring file(symbols_file);
    
    
    const wchar_t *extensions[] = { L"exe", L"dll" };
    for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
      size_t dot_pos = file.find_last_of(L".");
      if (dot_pos != wstring::npos) {
	file.replace(dot_pos + 1, wstring::npos, extensions[i]);
	
	if (GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES) {
	  code_file_ = file;
	  return true;
	}
      }
    }
  }

  return false;
}


bool PDBSourceLineWriter::GetSymbolFunctionName(IDiaSymbol *function,
                                                BSTR *name,
                                                int *stack_param_size) {
  *stack_param_size = -1;
  const DWORD undecorate_options = UNDNAME_NO_MS_KEYWORDS |
                                   UNDNAME_NO_FUNCTION_RETURNS |
                                   UNDNAME_NO_ALLOCATION_MODEL |
                                   UNDNAME_NO_ALLOCATION_LANGUAGE |
                                   UNDNAME_NO_THISTYPE |
                                   UNDNAME_NO_ACCESS_SPECIFIERS |
                                   UNDNAME_NO_THROW_SIGNATURES |
                                   UNDNAME_NO_MEMBER_TYPE |
                                   UNDNAME_NO_RETURN_UDT_MODEL |
                                   UNDNAME_NO_ECSU;

  
  if (function->get_undecoratedNameEx(undecorate_options, name) != S_OK) {
    if (function->get_name(name) != S_OK) {
      fprintf(stderr, "failed to get function name\n");
      return false;
    }
    
    
    
    
    
    
    
    
  } else {
    
    
    
    const wchar_t *replace_string = L"(void)";
    const size_t replace_length = wcslen(replace_string);
    const wchar_t *replacement_string = L"()";
    size_t length = wcslen(*name);
    if (length >= replace_length) {
      wchar_t *name_end = *name + length - replace_length;
      if (wcscmp(name_end, replace_string) == 0) {
        WindowsStringUtils::safe_wcscpy(name_end, replace_length,
                                        replacement_string);
        length = wcslen(*name);
      }
    }

    
    
    
    
    
    
    if (!wcschr(*name, ':') && !wcschr(*name, '(') &&
        (*name[0] == '_' || *name[0] == '@')) {
      wchar_t *last_at = wcsrchr(*name + 1, '@');
      if (last_at && wcstol_positive_strict(last_at + 1, stack_param_size)) {
        
        
        
        
        if (*name[0] == '@') {
          if (*stack_param_size > 8) {
            *stack_param_size -= 8;
          } else {
            *stack_param_size = 0;
          }
        }

        
        
        WindowsStringUtils::safe_wcsncpy(*name, length,
                                         *name + 1, last_at - *name - 1);
     } else if (*name[0] == '_') {
        
        
        
        
        WindowsStringUtils::safe_wcsncpy(*name, length, *name + 1, length);
      }
    }
  }

  return true;
}


int PDBSourceLineWriter::GetFunctionStackParamSize(IDiaSymbol *function) {
  

  
  CComPtr<IDiaEnumSymbols> data_children;
  if (FAILED(function->findChildren(SymTagData, NULL, nsNone,
                                    &data_children))) {
    return 0;
  }

  
  
  
  
  int lowest_base = INT_MAX;
  int highest_end = INT_MIN;

  CComPtr<IDiaSymbol> child;
  DWORD count;
  while (SUCCEEDED(data_children->Next(1, &child, &count)) && count == 1) {
    
    
    
    
    CComPtr<IDiaSymbol> child_type;

    
    
    
    DWORD child_kind;
    if (FAILED(child->get_dataKind(&child_kind)) ||
        (child_kind != DataIsParam && child_kind != DataIsObjectPtr)) {
      goto next_child;
    }

    
    
    
    DWORD child_location_type;
    if (FAILED(child->get_locationType(&child_location_type)) ||
        child_location_type != LocIsRegRel) {
      goto next_child;
    }

    
    
    
    
    
    
    
    
    DWORD child_register;
    if (FAILED(child->get_registerId(&child_register)) ||
        child_register != CV_REG_EBP) {
      goto next_child;
    }

    LONG child_register_offset;
    if (FAILED(child->get_offset(&child_register_offset))) {
      goto next_child;
    }

    
    if (FAILED(child->get_type(&child_type)) || !child_type) {
      goto next_child;
    }

    ULONGLONG child_length;
    if (FAILED(child_type->get_length(&child_length))) {
      goto next_child;
    }

    int child_end = child_register_offset + static_cast<ULONG>(child_length);
    if (child_register_offset < lowest_base) {
      lowest_base = child_register_offset;
    }
    if (child_end > highest_end) {
      highest_end = child_end;
    }

next_child:
    child.Release();
  }

  int param_size = 0;
  
  
  
  
  
  
  if (lowest_base < 4) {
    lowest_base = 4;
  }
  if (highest_end > lowest_base) {
    
    
    
    
    
    int remainder = highest_end % 4;
    if (remainder) {
      highest_end += 4 - remainder;
    }

    param_size = highest_end - lowest_base;
  }

  return param_size;
}

bool PDBSourceLineWriter::WriteMap(FILE *map_file) {
  output_ = map_file;

  bool ret = PrintPDBInfo();
  
  PrintPEInfo();
  ret = ret &&
    PrintSourceFiles() && 
    PrintFunctions() &&
    PrintFrameData();

  output_ = NULL;
  return ret;
}

void PDBSourceLineWriter::Close() {
  session_.Release();
}

bool PDBSourceLineWriter::GetModuleInfo(PDBModuleInfo *info) {
  if (!info) {
    return false;
  }

  info->debug_file.clear();
  info->debug_identifier.clear();
  info->cpu.clear();

  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    return false;
  }

  DWORD machine_type;
  
  if (global->get_machineType(&machine_type) == S_OK) {
    
    
    
    
    
    switch (machine_type) {
      case IMAGE_FILE_MACHINE_I386:
        info->cpu = L"x86";
        break;
      case IMAGE_FILE_MACHINE_AMD64:
        info->cpu = L"x86_64";
        break;
      default:
        info->cpu = L"unknown";
        break;
    }
  } else {
    
    info->cpu = L"unknown";
  }

  
  DWORD age;
  if (FAILED(global->get_age(&age))) {
    return false;
  }

  bool uses_guid;
  if (!UsesGUID(&uses_guid)) {
    return false;
  }

  if (uses_guid) {
    GUID guid;
    if (FAILED(global->get_guid(&guid))) {
      return false;
    }

    
    
    wchar_t age_string[9];
    swprintf(age_string, sizeof(age_string) / sizeof(age_string[0]),
             L"%x", age);

    
    age_string[sizeof(age_string) / sizeof(age_string[0]) - 1] = L'\0';

    info->debug_identifier = GUIDString::GUIDToSymbolServerWString(&guid);
    info->debug_identifier.append(age_string);
  } else {
    DWORD signature;
    if (FAILED(global->get_signature(&signature))) {
      return false;
    }

    
    
    wchar_t identifier_string[17];
    swprintf(identifier_string,
             sizeof(identifier_string) / sizeof(identifier_string[0]),
             L"%08X%x", signature, age);

    
    identifier_string[sizeof(identifier_string) /
                      sizeof(identifier_string[0]) - 1] = L'\0';

    info->debug_identifier = identifier_string;
  }

  CComBSTR debug_file_string;
  if (FAILED(global->get_symbolsFileName(&debug_file_string))) {
    return false;
  }
  info->debug_file =
      WindowsStringUtils::GetBaseName(wstring(debug_file_string));

  return true;
}

bool PDBSourceLineWriter::GetPEInfo(PEModuleInfo *info) {
  if (!info) {
    return false;
  }

  if (code_file_.empty() && !FindPEFile()) {
    fprintf(stderr, "Couldn't locate EXE or DLL file.\n");
    return false;
  }

  
  
  string code_file;
  if (!WindowsStringUtils::safe_wcstombs(code_file_, &code_file)) {
    return false;
  }

  AutoImage img(ImageLoad((PSTR)code_file.c_str(), NULL));
  if (!img) {
    fprintf(stderr, "Failed to open PE file: %s\n", code_file.c_str());
    return false;
  }

  info->code_file = WindowsStringUtils::GetBaseName(code_file_);

  
  DWORD TimeDateStamp = img->FileHeader->FileHeader.TimeDateStamp;
  
  DWORD SizeOfImage = 0;
  PIMAGE_OPTIONAL_HEADER64 opt =
    &((PIMAGE_NT_HEADERS64)img->FileHeader)->OptionalHeader;
  if (opt->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    
    SizeOfImage = opt->SizeOfImage;
  }
  else {
    
    SizeOfImage = img->FileHeader->OptionalHeader.SizeOfImage;
  }
  wchar_t code_identifier[32];
  swprintf(code_identifier,
	   sizeof(code_identifier) / sizeof(code_identifier[0]),
	   L"%08X%X", TimeDateStamp, SizeOfImage);
  info->code_identifier = code_identifier;

  return true;
}

bool PDBSourceLineWriter::UsesGUID(bool *uses_guid) {
  if (!uses_guid)
    return false;

  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global)))
    return false;

  GUID guid;
  if (FAILED(global->get_guid(&guid)))
    return false;

  DWORD signature;
  if (FAILED(global->get_signature(&signature)))
    return false;

  
  
  
  
  
  
  
  
  
  
  
  
  

  GUID signature_guid = {signature};  
  *uses_guid = !IsEqualGUID(guid, signature_guid);
  return true;
}

}  
