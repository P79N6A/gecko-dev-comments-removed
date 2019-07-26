
































#include "processor/synth_minidump.h"

namespace google_breakpad {

namespace SynthMinidump {

Section::Section(const Dump &dump)
  : test_assembler::Section(dump.endianness()) { }

void Section::CiteLocationIn(test_assembler::Section *section) const {
  if (this)
    (*section).D32(size_).D32(file_offset_);
  else
    (*section).D32(0).D32(0);
}

void Stream::CiteStreamIn(test_assembler::Section *section) const {
  section->D32(type_);
  CiteLocationIn(section);
}

SystemInfo::SystemInfo(const Dump &dump,
                       const MDRawSystemInfo &system_info,
                       const String &csd_version)
    : Stream(dump, MD_SYSTEM_INFO_STREAM) {
  D16(system_info.processor_architecture);
  D16(system_info.processor_level);
  D16(system_info.processor_revision);
  D8(system_info.number_of_processors);
  D8(system_info.product_type);
  D32(system_info.major_version);
  D32(system_info.minor_version);
  D32(system_info.build_number);
  D32(system_info.platform_id);
  csd_version.CiteStringIn(this);
  D16(system_info.suite_mask);
  D16(system_info.reserved2);           

  
  if (system_info.processor_architecture == MD_CPU_ARCHITECTURE_X86) {
    D32(system_info.cpu.x86_cpu_info.vendor_id[0]);
    D32(system_info.cpu.x86_cpu_info.vendor_id[1]);
    D32(system_info.cpu.x86_cpu_info.vendor_id[2]);
    D32(system_info.cpu.x86_cpu_info.version_information);
    D32(system_info.cpu.x86_cpu_info.feature_information);
    D32(system_info.cpu.x86_cpu_info.amd_extended_cpu_features);
  } else {
    D64(system_info.cpu.other_cpu_info.processor_features[0]);
    D64(system_info.cpu.other_cpu_info.processor_features[1]);
  }
}

const MDRawSystemInfo SystemInfo::windows_x86 = {
  MD_CPU_ARCHITECTURE_X86,              
  6,                                    
  0xd08,                                
  1,                                    
  1,                                    
  5,                                    
  1,                                    
  2600,                                 
  2,                                    
  0xdeadbeef,                           
  0x100,                                
  0,                                    
  {                                     
    { 
      { 0x756e6547, 0x49656e69, 0x6c65746e }, 
      0x6d8,                                  
      0xafe9fbff,                             
      0xffffffff                              
    }
  }
};

const string SystemInfo::windows_x86_csd_version = "Service Pack 2";

String::String(const Dump &dump, const string &contents) : Section(dump) {
  D32(contents.size() * 2);
  for (string::const_iterator i = contents.begin(); i != contents.end(); i++)
    D16(*i);
}

void String::CiteStringIn(test_assembler::Section *section) const {
  section->D32(file_offset_);
}

void Memory::CiteMemoryIn(test_assembler::Section *section) const {
  section->D64(address_);
  CiteLocationIn(section);
}

Context::Context(const Dump &dump, const MDRawContextX86 &context)
  : Section(dump) {
  
  
  
  assert(((context.context_flags & MD_CONTEXT_CPU_MASK) == 0) ||
         (context.context_flags & MD_CONTEXT_X86));
  
  assert(dump.endianness() == kLittleEndian);
  D32(context.context_flags);
  D32(context.dr0);
  D32(context.dr1);
  D32(context.dr2);
  D32(context.dr3);
  D32(context.dr6);
  D32(context.dr7);
  D32(context.float_save.control_word);
  D32(context.float_save.status_word);
  D32(context.float_save.tag_word);
  D32(context.float_save.error_offset);
  D32(context.float_save.error_selector);
  D32(context.float_save.data_offset);
  D32(context.float_save.data_selector);
  
  
  Append(context.float_save.register_area,
         sizeof(context.float_save.register_area));
  D32(context.float_save.cr0_npx_state);
  D32(context.gs);
  D32(context.fs);
  D32(context.es);
  D32(context.ds);
  D32(context.edi);
  D32(context.esi);
  D32(context.ebx);
  D32(context.edx);
  D32(context.ecx);
  D32(context.eax);
  D32(context.ebp);
  D32(context.eip);
  D32(context.cs);
  D32(context.eflags);
  D32(context.esp);
  D32(context.ss);
  
  
  Append(context.extended_registers, sizeof(context.extended_registers));
  assert(Size() == sizeof(MDRawContextX86));
}

Context::Context(const Dump &dump, const MDRawContextARM &context)
  : Section(dump) {
  
  assert((context.context_flags & MD_CONTEXT_ARM) ||
         (context.context_flags & MD_CONTEXT_ARM_OLD));
  
  assert(dump.endianness() == kLittleEndian);
  D32(context.context_flags);
  for (int i = 0; i < MD_CONTEXT_ARM_GPR_COUNT; ++i)
    D32(context.iregs[i]);
  D32(context.cpsr);
  D64(context.float_save.fpscr);
  for (int i = 0; i < MD_FLOATINGSAVEAREA_ARM_FPR_COUNT; ++i)
    D64(context.float_save.regs[i]);
  for (int i = 0; i < MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT; ++i)
    D32(context.float_save.extra[i]);
  assert(Size() == sizeof(MDRawContextARM));
}

Thread::Thread(const Dump &dump,
               uint32_t thread_id, const Memory &stack, const Context &context,
               uint32_t suspend_count, uint32_t priority_class,
               uint32_t priority, uint64_t teb) : Section(dump) {
  D32(thread_id);
  D32(suspend_count);
  D32(priority_class);
  D32(priority);
  D64(teb);
  stack.CiteMemoryIn(this);
  context.CiteLocationIn(this);
  assert(Size() == sizeof(MDRawThread));
}

Module::Module(const Dump &dump,
               uint64_t base_of_image,
               uint32_t size_of_image,
               const String &name,
               uint32_t time_date_stamp,
               uint32_t checksum,
               const MDVSFixedFileInfo &version_info,
               const Section *cv_record,
               const Section *misc_record) : Section(dump) {
  D64(base_of_image);
  D32(size_of_image);
  D32(checksum);
  D32(time_date_stamp);
  name.CiteStringIn(this);
  D32(version_info.signature);
  D32(version_info.struct_version);
  D32(version_info.file_version_hi);
  D32(version_info.file_version_lo);
  D32(version_info.product_version_hi);
  D32(version_info.product_version_lo);
  D32(version_info.file_flags_mask);
  D32(version_info.file_flags);
  D32(version_info.file_os);
  D32(version_info.file_type);
  D32(version_info.file_subtype);
  D32(version_info.file_date_hi);
  D32(version_info.file_date_lo);
  cv_record->CiteLocationIn(this);
  misc_record->CiteLocationIn(this);
  D64(0).D64(0);
}

const MDVSFixedFileInfo Module::stock_version_info = {
  MD_VSFIXEDFILEINFO_SIGNATURE,         
  MD_VSFIXEDFILEINFO_VERSION,           
  0x11111111,                           
  0x22222222,                           
  0x33333333,                           
  0x44444444,                           
  MD_VSFIXEDFILEINFO_FILE_FLAGS_DEBUG,  
  MD_VSFIXEDFILEINFO_FILE_FLAGS_DEBUG,  
  MD_VSFIXEDFILEINFO_FILE_OS_NT | MD_VSFIXEDFILEINFO_FILE_OS__WINDOWS32,
                                        
  MD_VSFIXEDFILEINFO_FILE_TYPE_APP,     
  MD_VSFIXEDFILEINFO_FILE_SUBTYPE_UNKNOWN, 
  0,                                    
  0                                     
};

Exception::Exception(const Dump &dump,
                     const Context &context,
                     uint32_t thread_id,
                     uint32_t exception_code,
                     uint32_t exception_flags,
                     uint64_t exception_address)
  : Stream(dump, MD_EXCEPTION_STREAM) {
  D32(thread_id);
  D32(0);  
  D32(exception_code);
  D32(exception_flags);
  D64(0);  
  D64(exception_address);
  D32(0);  
  D32(0);  
  for (int i = 0; i < MD_EXCEPTION_MAXIMUM_PARAMETERS; ++i)
    D64(0);  
  context.CiteLocationIn(this);
  assert(Size() == sizeof(MDRawExceptionStream));
}

Dump::Dump(uint64_t flags,
           Endianness endianness,
           uint32_t version,
           uint32_t date_time_stamp)
    : test_assembler::Section(endianness),
      file_start_(0),
      stream_directory_(*this),
      stream_count_(0),
      thread_list_(*this, MD_THREAD_LIST_STREAM),
      module_list_(*this, MD_MODULE_LIST_STREAM),
      memory_list_(*this, MD_MEMORY_LIST_STREAM)
 {
  D32(MD_HEADER_SIGNATURE);
  D32(version);
  D32(stream_count_label_);
  D32(stream_directory_rva_);
  D32(0);
  D32(date_time_stamp);
  D64(flags);
  assert(Size() == sizeof(MDRawHeader));
}

Dump &Dump::Add(SynthMinidump::Section *section) {
  section->Finish(file_start_ + Size());
  Append(*section);
  return *this;
}

Dump &Dump::Add(Stream *stream) {
  Add(static_cast<SynthMinidump::Section *>(stream));
  stream->CiteStreamIn(&stream_directory_);
  stream_count_++;
  return *this;
}

Dump &Dump::Add(Memory *memory) {
  
  Add(static_cast<SynthMinidump::Section *>(memory));

  
  
  SynthMinidump::Section descriptor(*this);
  memory->CiteMemoryIn(&descriptor);
  memory_list_.Add(&descriptor);
  return *this;
}

Dump &Dump::Add(Thread *thread) {
  thread_list_.Add(thread);
  return *this;
}

Dump &Dump::Add(Module *module) {
  module_list_.Add(module);
  return *this;
}

void Dump::Finish() {
  if (!thread_list_.Empty()) Add(&thread_list_);
  if (!module_list_.Empty()) Add(&module_list_);
  if (!memory_list_.Empty()) Add(&memory_list_);

  
  
  
  
  stream_count_label_ = stream_count_;
  stream_directory_rva_ = file_start_ + Size();
  Append(static_cast<test_assembler::Section &>(stream_directory_));
}

} 
          
} 
