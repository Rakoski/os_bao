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
            Arch::Cpu* cpuglobal;
            uint16_t ciclos_timer = Config::timer_default_interrupt_cycles;
            uint16_t contador_timer = 0;
            uint16_t pedaco_tempo_padrao = 1;
            uint32_t comeco = 0;

        public:
            ProcessManager(Arch::Cpu* cpuglobal);

            ~ProcessManager();

            Process* get_process();

            uint32_t get_tempo_sistema();

            void acordar_processos_dormindo();

            void push_back_processos_novos(Process* processo);

            void remover_processo(Process* processo);

            void dormir_processo(Process* processo, uint16_t segundos);

            void set_ciclos_timer(uint32_t ciclos);

            Process* pop_front_processos_novos();

            Process* front_processos_novos();

            bool emptar_processos_novos() const;

            Process* configurar_proximo_processo(Process* processo_rodando_no_momento, Process* idle);

            void escalonar(Process* processo);

            Process* encontrar_por_pid(uint16_t pid);

            std::list<Process*> get_processos_rodando_novo() const {
                return processos_rodando_novo;
            }

            void set_processos_rodando_novo(const std::list<Process*> &processos_rodando_novo) {
                this->processos_rodando_novo = processos_rodando_novo;
            }

            std::list<Process*> get_processos_dormindo() const {
                return processos_dormindo;
            }

            void set_processos_dormindo(const std::list<Process*> &processos_dormindo) {
                this->processos_dormindo = processos_dormindo;
            }

            Arch::Cpu* get_cpu() const {
                return cpuglobal;
            }

            void set_cpu(Arch::Cpu *cpu) {
                this->cpuglobal = cpu;
            }

            uint16_t get_ciclos_timer() const {
                return ciclos_timer;
            }

            uint16_t get_contador_timer() const {
                return contador_timer;
            }

            void set_contador_timer(uint16_t contador_timer) {
                this->contador_timer = contador_timer;
            }

            uint16_t get_pedaco_tempo_padrao() const {
                return pedaco_tempo_padrao;
            }

            void set_pedaco_tempo_padrao(uint16_t pedaco_tempo_padrao) {
                this->pedaco_tempo_padrao = pedaco_tempo_padrao;
            }

            uint32_t get_comeco() const {
                return comeco;
            }

            void set_comeco(uint32_t comeco) {
                this->comeco = comeco;
            }
    };
}

#endif