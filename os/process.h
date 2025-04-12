//
// Created by mateus on 29/03/25.
//

#ifndef __SO_BAO_HEADER_PROCESS_H__
#define __SO_BAO_HEADER_PROCESS_H__

#include <cstdint>
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

    class Process {
        private:
            uint16_t pid = 0;
            std::string name;
            ProcessState state_ = ProcessState::ready;

        uint16_t program_counter;
        std::array<uint16_t, Config::nregs> registrators;

            uint16_t base = 0;
            uint16_t limit = 0;

        std::vector<uint16_t> code;

    public:

            [[nodiscard]] uint16_t get_pid() const {
                return pid;
            }

            [[nodiscard]] std::string get_name() const {
                return name;
            }

            [[nodiscard]] uint16_t get_base() const {
                return base;
            }

            [[nodiscard]] uint16_t get_limit() const {
                return limit;
            }

            void set_pid(uint16_t pid) {
                this->pid = pid;
            }

            void set_name(const std::string_view &name) {
                this->name = name;
            }

            void set_base(uint16_t base) {
                this->base = base;
            }

            void set_limit(uint16_t limit) {
                this->limit = limit;
            }

            [[nodiscard]] uint16_t pid1() const {
                return pid;
            }

            void set_pid1(uint16_t pid) {
                this->pid = pid;
            }

            [[nodiscard]] std::vector<uint16_t> code1() const {
                return code;
            }

            void set_code(const std::vector<uint16_t> &code) {
                this->code = code;
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
                return state_;
            }

            void set_state(ProcessState state) {
                state_ = state;
            }

            [[nodiscard]] uint16_t program_counter1() const {
                return program_counter;
            }

            void set_program_counter(uint16_t program_counter) {
                this->program_counter = program_counter;
            }

            [[nodiscard]] std::array<uint16_t, Config::nregs> registrators1() const {
                return registrators;
            }

            void set_registrators(const std::array<uint16_t, Config::nregs> &registrators) {
                this->registrators = registrators;
            }

            [[nodiscard]] uint16_t base1() const {
                return base;
            }

            void set_base1(uint16_t base) {
                this->base = base;
            }

            [[nodiscard]] uint16_t limit1() const {
                return limit;
            }

            void set_limit1(uint16_t limit) {
                this->limit = limit;
            }

            [[nodiscard]] std::vector<uint16_t> code2() const {
                return code;
            }

            void set_code1(const std::vector<uint16_t> &code) {
                this->code = code;
            }

            void do_mem_protection(Arch::Cpu* cpu) const {
                cpu->set_vmem_mode(Arch::Cpu::BaseLimit);
                cpu->set_vmem_paddr_base(base);
                cpu->set_vmem_size(limit);
            }

        //context swtiching
            void save_contect_cpu(Arch::Cpu* cpu);

            void save_context(Arch::Cpu *cpu);

            void restore_context(Arch::Cpu* cpu) const;

            Process(uint16_t pid, const std::string &name, const std::vector<uint16_t> &code);

            ~Process();

            uint16_t get_registrator(uint16_t number) const;

            void set_registrator(uint16_t number, uint16_t value);
    };
}



#endif //PROCESS_H
