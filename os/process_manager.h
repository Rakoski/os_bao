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

        std::list<Process*> processos_rodando_novo;
        std::list<Process*> processos_dormindo;

        private:
            Arch::Cpu* cpu;

        public:
            ProcessManager(Arch::Cpu* cpu);

            Process* get_process();

        };
}

#endif