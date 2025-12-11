#include "process_manager.h"

#include "os-lib.h"
#include "process.h"
#include <algorithm>

namespace OS {
    ProcessManager::ProcessManager(Arch::Cpu* cpuglobal) : cpuglobal(cpuglobal) {
        comeco = cpuglobal->read_io(Arch::IO_Port::TimerGetTimeSeconds);
        cpuglobal->write_io(Arch::IO_Port::TimerInterruptCycles, ciclos_timer);
    }

    ProcessManager::~ProcessManager() {
        for (Process* processo : processos_rodando_novo) delete processo;
        for (Process* processo : processos_dormindo) delete processo;
        processos_rodando_novo.clear();
        processos_dormindo.clear();
    }
    uint32_t ProcessManager::get_tempo_sistema() {
        uint32_t tempo_de_agora = cpuglobal->read_io(Arch::IO_Port::TimerGetTimeSeconds);
        return  tempo_de_agora - comeco;
    }

    void ProcessManager::acordar_processos_dormindo() {
        uint32_t tempo_de_agora = get_tempo_sistema();

        auto it = processos_dormindo.begin();
        while (it != processos_dormindo.end()) {
            Process* processo_vai_acordar = *it;

            if (processo_vai_acordar->get_dormir_ate() <= tempo_de_agora) {
                terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "acordandooou ", processo_vai_acordar->get_pid());

                processo_vai_acordar->set_estado(ProcessState::ready);
                processos_rodando_novo.push_back(processo_vai_acordar);
                it = processos_dormindo.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ProcessManager::remover_processo(Process* processo_removivel) {
        if (!processo_removivel) return;

        // arrumar nomes aq colocar it
        // usar uma remove / remove if
        auto processo = processos_rodando_novo.begin();
        while (processo != processos_rodando_novo.end()) {
            terminal_println(cpuglobal, Terminal::Kernel, "processo ", (*processo)->get_pid(), " passando");
            if (*processo == processo_removivel) {
                terminal_println(cpuglobal, Terminal::Kernel, "DEBUG: removendooou while ", processo_removivel->get_pid(), " das lista td");
                processo = processos_rodando_novo.erase(processo);
            } else {
                ++processo;
            }
        }


        terminal_println(cpuglobal, Terminal::Kernel, "nova lista de processos rodando: ");
        for (Process* processo : processos_rodando_novo) {
            terminal_println(cpuglobal, Terminal::Kernel, "nova lista de processos rodando: ", processo->get_pid());
        }

        auto dormindo = std::find(processos_dormindo.begin(), processos_dormindo.end(), processo_removivel);
        if (dormindo != processos_dormindo.end()) {
            processos_dormindo.erase(dormindo);
            terminal_println(cpuglobal, Terminal::Kernel, "DEBUG: removendoooo da lista dormindooo!!");
        }
    }

    // fora isso, por interrupt do timer deixar como ready
    // syscall 6 que vai p dormir
    void ProcessManager::dormir_processo(Process *processos_dormante, uint16_t segundos) {
        if (!processos_dormante) return;

        processos_dormante->set_dormir_ate(get_tempo_sistema() + segundos);
        processos_dormante->set_estado(ProcessState::sleeping);

        auto it = std::find(processos_rodando_novo.begin(), processos_rodando_novo.end(), processos_dormante);
        if (it != processos_rodando_novo.end()) processos_rodando_novo.erase(it);

        processos_dormindo.push_back(processos_dormante);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "processo: ", processos_dormante->get_name(), " vai dormir por ", segundos);
    }

    void ProcessManager::set_ciclos_timer(uint32_t ciclos) {
        ciclos_timer = ciclos;
        cpuglobal->write_io(IO_Port::TimerInterruptCycles, ciclos);
    }

    void ProcessManager::push_back_processos_novos(Process* processo) {
        processos_rodando_novo.push_back(processo);
    }

    Process* ProcessManager::pop_front_processos_novos() {
        if (processos_rodando_novo.empty()) return nullptr;
        Process* proc = processos_rodando_novo.front();
        processos_rodando_novo.pop_front();
        return proc;
    }

    Process* ProcessManager::front_processos_novos() {
        if (processos_rodando_novo.empty()) return nullptr;
        return processos_rodando_novo.front();
    }

    bool ProcessManager::emptar_processos_novos() const {
        return processos_rodando_novo.empty();
    }

    Process* ProcessManager::configurar_proximo_processo(Process* processo_rodando_no_momento, Process* idle) {
        if (processo_rodando_no_momento) {
            processo_rodando_no_momento->salvar_contexto(cpuglobal);
            processo_rodando_no_momento->set_estado(ProcessState::ready);
            if (processo_rodando_no_momento != idle) push_back_processos_novos(processo_rodando_no_momento);
        }

        Process* proximo_processo = pop_front_processos_novos();

        if (!proximo_processo) {
            proximo_processo = idle;
        }

        terminal_println(cpuglobal,  Terminal::Kernel, "proximo processo ", proximo_processo->get_name());

        proximo_processo->set_estado(ProcessState::running);
        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        cpuglobal->set_page_table(proximo_processo->get_tabela_paginas());
        proximo_processo->restaurar_contexto(cpuglobal);
        set_contador_timer(0);

        terminal_println(cpuglobal,  Terminal::Kernel, "mudando para: ", proximo_processo->get_pid(), "(", proximo_processo->get_name(), ")");
        return proximo_processo;
    }

    Process *ProcessManager::encontrar_por_pid(uint16_t pid) {
        for (Process* processo : processos_rodando_novo) if (processo->get_pid() == pid) return processo;
        for (Process* processo_dormente : processos_dormindo) if (processo_dormente->get_pid() == pid) return processo_dormente;

        return nullptr;
    }

void ProcessManager::lista_processos(Arch::Cpu *cpuglobal, Process* processo_rodando_no_momento) {
    terminal_println(cpuglobal, Terminal::Command, "\n=== PROCESSOS ===\n");
    terminal_println(cpuglobal, Terminal::Command, " PID\tNOME\t\tESTADO\t\tPC\t\tTABELA PAGINA\t\tTEMPO DE CRIACAO\n");

    if (processo_rodando_no_momento && processo_rodando_no_momento) {
        std::string estado_processo = "RUNNING";
        terminal_println(cpuglobal, Terminal::Command, " *", processo_rodando_no_momento->get_pid(), "\t",
                       processo_rodando_no_momento->get_name(), "\t\t", estado_processo, "\t\t",
                       processo_rodando_no_momento->get_pc(), "\t\t", processo_rodando_no_momento->get_tabela_paginas(),
                       "\t\t", processo_rodando_no_momento->get_tempo_criacao());
    }

    const auto& processos_novo = get_processos_rodando_novo();
    for (Process* processo : processos_novo) {
        std::string estado_processo;
        switch (processo->get_estado()) {
            case ProcessState::ready: estado_processo = "READY"; break;
            case ProcessState::running: estado_processo = "RUNNING"; break;
            case ProcessState::sleeping: estado_processo = "SLEEPING"; break;
            case ProcessState::finished: estado_processo = "FINISHED"; break;
        }

        std::string marca = (processo == processo_rodando_no_momento) ? " *" : "  ";
        terminal_println(cpuglobal, Terminal::Command, marca, processo->get_pid(), "\t",
                       processo->get_name(), "\t\t", estado_processo, "\t\t", processo->get_pc(), "\t\t",
                       processo->get_tabela_paginas(), "\t\t", processo->get_tempo_criacao());
    }

    const auto& dormindo = get_processos_dormindo();
    if (!dormindo.empty()) {
        terminal_println(cpuglobal, Terminal::Command, "");
        for (Process* processo : dormindo) {
            uint16_t tempo_atual = get_tempo_sistema();
            uint16_t tempo_vida = tempo_atual - processo->get_tempo_criacao();
            uint16_t tempo_restante = (processo->get_dormir_ate() > tempo_atual)
                                      ? (processo->get_dormir_ate() - tempo_atual)
                                      : 0;

            terminal_println(cpuglobal, Terminal::Command,
                           "  ", processo->get_pid(), " | ",
                           processo->get_name(), " | ",
                           "SLEEPING  | ",
                           processo->get_pc(), " | ",
                           processo->get_tempo_criacao(), "s (", tempo_vida, "s) | ",
                           processo->get_dormir_ate(), "s (", tempo_restante, "s restantes)");
        }
    }
}
}
