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

        auto it = std::find(processos_rodando_novo.begin(), processos_rodando_novo.end(), processo_removivel);
        if (it != processos_rodando_novo.end()) {
            processos_rodando_novo.erase(it);
            return;
        }

        auto sleep_it = std::find(processos_dormindo.begin(), processos_dormindo.end(), processo_removivel);
        if (sleep_it != processos_dormindo.end()) {
            processos_dormindo.erase(sleep_it);
        }
    }

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
        if (!processo_rodando_no_momento || processo_rodando_no_momento->get_estado() != ProcessState::running) return nullptr;

        processo_rodando_no_momento->salvar_contexto(cpuglobal);
        processo_rodando_no_momento->set_estado(ProcessState::ready);

        if (processo_rodando_no_momento != idle) push_back_processos_novos(processo_rodando_no_momento);

        Process* proximo_processo = pop_front_processos_novos();

        if (!proximo_processo) {
            if (!idle) {
                terminal_println(cpuglobal, Terminal::Kernel, "ue idle não existe?? e agr kkkkk");
                cpuglobal->turn_off();
                return nullptr;
            }
            proximo_processo = idle;
        }

        processo_rodando_no_momento = proximo_processo;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        cpuglobal->set_page_table(processo_rodando_no_momento->get_tabela_paginas());
        processo_rodando_no_momento->restaurar_contexto(cpuglobal);
        set_contador_timer(0);

        terminal_println(cpuglobal, Terminal::Kernel, "mudando para ", processo_rodando_no_momento->get_pid(), " (", processo_rodando_no_momento->get_name(), ")");
        return processo_rodando_no_momento;
    }
}
