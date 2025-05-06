#ifndef __ARQSIM_HEADER_OS_H__
#define __ARQSIM_HEADER_OS_H__

#include <cstdint>
#include <string>

#include <my-lib/std.h>
#include <my-lib/macros.h>

#include "../config.h"
#include "../arch/arch.h"

namespace OS {

    

    using VmemMode = Arch::Cpu::VmemMode;
    using CpuException = Arch::Cpu::CpuException;
    using PageTableEntry = Arch::Cpu::PageTableEntry;
    using PageTable = Arch::Cpu::PageTable;
    using InterruptCode = Arch::InterruptCode;
    using IO_Port = Arch::IO_Port;
    using Terminal = Arch::Terminal::Type;
    using DiskState = Arch::Disk::State;
    using DiskCmd = Arch::Disk::Cmd;
    using DiskError = Arch::Disk::Error;

    

    void boot(Arch::Cpu *cpu);

    void interrupt(const InterruptCode interrupt);

    void syscall();

    void shutdown();

    void process_command(const std::string& cmd);

    
    void process_keyboard_input(char c);

    

} 

#endif