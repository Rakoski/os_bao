#include "process_manager.h"
#include "process.h"

namespace OS {
    extern Process* processo_rodando_no_momento;
    extern void tratar_excecao();

    ProcessManager* gerenciador_processos = nullptr;

    Process *ProcessManager::get_process() {
        return processo_rodando_no_momento;
    }

    void ProcessManager::handle_exception(const Arch::Cpu::CpuException &exception) {
        tratar_excecao();
    }
}