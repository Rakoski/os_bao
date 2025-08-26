//
// Created by mateus on 29/03/25.
//

#ifndef __SO_BAO_HEADER_PROCESS_H__
#define __SO_BAO_HEADER_PROCESS_H__

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../config.h"
#include "../arch/arch.h"

// um reg vai ter o tamanho base
// o outro o tamanho da memória
// quando um processo for tentar pegar a memória:
// o endereço virtual é menor que o tamanho limite?
// sim: pode ser pego
// não: VMEMPAGEFAULT
// lembrando que a arch é de 16 bits

namespace OS {

    enum class ProcessState {
       ready,
       running,
       bloked,
       finished
    };

    struct AreaMemoriaVirtual {
        uint16_t endereco;
        uint16_t numero_pag;
        uint16_t tamanho_words;

        AreaMemoriaVirtual(uint16_t endereco, uint16_t numero_pag, uint16_t tamanho_words) : endereco(endereco), numero_pag(numero_pag), tamanho_words(tamanho_words) {}
    };

    class Process {
    public:
        uint16_t pid = 0;
        std::string name;
        ProcessState estado = ProcessState::ready;

        Process();

        uint16_t pc;
        std::array<uint16_t, Config::nregs> regs;

        // pode ser tanto o tamanho quanto o final (eu fiz como tamanho)
        uint16_t limite = 0;

        // tirar daqui colocar var local
        std::vector<uint16_t> codigo_processo;

        Arch::Cpu::PageTable* tabela_paginas = nullptr;

        std::map<uint16_t, AreaMemoriaVirtual> alocacoes;

        [[nodiscard]] uint16_t get_pid() const {
                return pid;
            }

            [[nodiscard]] std::string get_name() const {
                return name;
            }

            [[nodiscard]] uint16_t get_limite() const {
                return limite;
            }

            void set_pid(uint16_t pid) {
                this->pid = pid;
            }

            void set_name(const std::string_view &name) {
                this->name = name;
            }

            void set_limite(uint16_t limit) {
                this->limite = limit;
            }

            [[nodiscard]] uint16_t pid1() const {
                return pid;
            }

            void set_pid1(uint16_t pid) {
                this->pid = pid;
            }

            [[nodiscard]] std::vector<uint16_t> codigo_processo1() const {
                return codigo_processo;
            }

            [[nodiscard]] const std::vector<uint16_t>& get_codigo_processo() const {
                return codigo_processo;
            }

            void set_codigo_processo(const std::vector<uint16_t> &codigo_processo) {
                this->codigo_processo = codigo_processo;
            }

            [[nodiscard]] uint16_t pid2() const {
                return pid;
            }

            void set_pid2(uint16_t pid) {
                this->pid = pid;
            }

            [[nodiscard]] std::string name1() const {
                return name;
            }

            void set_name1(const std::string &name) {
                this->name = name;
            }

            [[nodiscard]] ProcessState state() const {
                return estado;
            }

            void set_estado(ProcessState state) {
                estado = state;
            }

            [[nodiscard]] uint16_t program_counter1() const {
                return pc;
            }

            void set_pc(uint16_t program_counter) {
                this->pc = program_counter;
            }

            [[nodiscard]] std::array<uint16_t, Config::nregs> regs1() const {
                return regs;
            }

            void set_regs(const std::array<uint16_t, Config::nregs> &regs) {
                this->regs = regs;
            }

            [[nodiscard]] std::vector<uint16_t> codigo_processo2() const {
                return codigo_processo;
            }

            void set_codigo_processo1(const std::vector<uint16_t> &codigo_processo) {
                this->codigo_processo = codigo_processo;
            }

        [[nodiscard]] Arch::Cpu::PageTable * get_tabela_paginas() const {
            return tabela_paginas;
        }

        void set_tabela_paginas(Arch::Cpu::PageTable *tabela) {
            this->tabela_paginas = tabela;
        }

        void protege(Arch::Cpu* cpu) const {

                // n deixa acessar o end de cada um
                cpu->set_vmem_mode(Arch::Cpu::Paging);
                cpu->set_vmem_size(limite);
            }

        //context swtiching
            void save_contect_cpu(Arch::Cpu* cpu);

            void salvar_contexto(Arch::Cpu *cpu);

            void restaurar_contexto(Arch::Cpu* cpu) const;

            Process(uint16_t pid, const std::string &name, const std::vector<uint16_t> &codigo_processo);

            uint16_t get_regs(uint16_t number) const;

            void set_regs(uint16_t number, uint16_t value);

        void colocar_alocacao(uint16_t endereco, uint16_t numero_pag, uint16_t tamanho_words);

        AreaMemoriaVirtual* obter_alocacao(uint16_t endereco);
    };
}



#endif //PROCESS_H
