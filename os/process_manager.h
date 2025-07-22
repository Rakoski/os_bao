#ifndef __SO_BAO_HEADER_PROCESS_MANAGER_H__
#define __SO_BAO_HEADER_PROCESS_MANAGER_H__

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "../arch/arch.h"
#include "process.h"

namespace OS {

    class ProcessManager {
    private:
        Arch::Cpu* cpu;

    public:
        ProcessManager(Arch::Cpu* cpu);

        Process* get_process();
        
        void handle_exception(const Arch::Cpu::CpuException& exception);
    };

    extern ProcessManager* process_manager;
}

#endif