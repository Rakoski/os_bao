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
        Process* processo_corente = nullptr;
        uint16_t proximo_pid = 1;

    public:
        uint16_t pc;
        explicit ProcessManager(Arch::Cpu* cpu);
        ~ProcessManager();

        uint16_t create_process(const std::string& name, const std::vector<uint16_t>& code);
        bool kill_process(uint16_t pid);

        void load_idle_process();

        void get_processo_corente();

        void rodar_processo_corente();

        void schedule_next_process();

        bool load_program(const std::string& filename);
        void list_processes();

        void handle_exception(const Arch::Cpu::CpuException& exception);

        void handle_syscall();
    };

    extern ProcessManager* process_manager;

}

#endif // __SO_BAO_HEADER_PROCESS_MANAGER_H__