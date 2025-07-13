//
// Created by mateus on 03/05/25.
//

#include "utils.h"
#include "os-lib.h"

namespace OS {
    extern Arch::Cpu* cpuglobal;
    extern std::vector<Process*> processos_rodando;

    void Utils::setando_novos_regs_pro_processo(OS::Process* process) {
        for (uint16_t i = 0; i < Config::nregs; i++) {
            process->set_regs(i, 0);
        }
    };

    void Utils::printar_help() {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nComandos disponíveis:");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  load <arquivo> - Carrega um programa/processo");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  kill - Termina o processo atual");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  exit - Sai do simulador");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  help - Mostra este menu de ajuda");
    }

    void Utils::exibir_info_processo(Process* processo) {
        terminal_println(cpuglobal, Terminal::Kernel, "processo ", processo->get_pid()," (", processo->get_name(), ") foi carregado\n");
        terminal_println(cpuglobal, Terminal::Kernel, "base: ", processo->get_base(),", limite: ", processo->get_limite(), "\n");
    }

    Process* Utils::find_idle() {
        for (Process* processo : processos_rodando) {
            if (processo->get_name() == "idle.bin") {
                return processo;
            }
        }
        return nullptr;
    }
}


