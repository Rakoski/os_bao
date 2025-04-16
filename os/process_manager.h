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
        std::vector<std::unique_ptr<Process>> processes;
        Process* current_process = nullptr;
        uint16_t next_pid = 1;

        
        std::unique_ptr<Process> idle_process = nullptr;

    public:
        explicit ProcessManager(Arch::Cpu* cpu);
        ~ProcessManager();

        
        uint16_t create_process(const std::string& name, const std::vector<uint16_t>& code);
        bool kill_process(uint16_t pid);
        Process* get_current_process();

        
        void load_idle_process();

        
        void run_current_process();

        
        void schedule_next_process();

        
        bool load_program(const std::string& filename);
        void list_processes();

        
        void handle_exception(const Arch::Cpu::CpuException& exception);

        
        void handle_syscall();
    };

    
    extern ProcessManager* process_manager;

} 

#endif 