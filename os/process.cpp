//
// Created by mateus on 29/03/25.
//

#include "process.h"

namespace OS {
    Process::Process(uint16_t pid, const std::string& name, const std::vector<uint16_t>& code)
    : pid(pid), name(name), state_(ProcessState::ready), program_counter(0), base(0), limit(0), code(code) {
        for (auto& registrator : registrators) {
            registrator = 0;
        }
    }

    Process::~Process() {

    }

    uint16_t Process::get_registrator(const uint16_t number) const {
        if (number < Config::nregs) {
            return registrators[number];
        } else {
            return 0;
        }
    }

    void Process::set_registrator(uint16_t number, uint16_t value) {
        if (number < Config::nregs) {
            registrators[number] = value;
        }
    }

    void Process::save_context(Arch::Cpu* cpu) {
        program_counter = cpu->get_pc();

        for (uint16_t i = 0; i < Config::nregs; i++) {
            registrators[i] = cpu->get_gpr(i);
        }
    }

    void Process::restore_context(Arch::Cpu* cpu) const {
        cpu->set_pc(program_counter);

        for (uint16_t i = 0; i < Config::nregs; i++) {
            cpu->set_gpr(i, registrators[i]);
        }

        cpu->set_vmem_mode(Arch::Cpu::VmemMode::BaseLimit);
        cpu->set_vmem_paddr_base(base);
        cpu->set_vmem_size(limit);
    }
}


