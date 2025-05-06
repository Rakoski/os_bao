#include "process.h"

namespace OS {
    Process::Process(uint16_t pid, const std::string& name, const std::vector<uint16_t>& codigo_proceso)
    : pid(pid), name(name), state_(ProcessState::ready), pc(0), base(0), limite(0), codigo_processo(codigo_proceso) {
        
        for (auto& reg : regs) {
            reg = 0;
        }
    }

    Process::~Process() {
        
    }

    uint16_t Process::get_regs(const uint16_t number) const {
        if (number < Config::nregs) {
            return regs[number];
        } else {
            return 0;
        }
    }

    void Process::set_regs(uint16_t number, uint16_t value) {
        if (number < Config::nregs) {
            regs[number] = value;
        }
    }

    void Process::save_context(Arch::Cpu* cpu) {
        pc = cpu->get_pc();

        for (uint16_t i = 0; i < Config::nregs; i++) {
            regs[i] = cpu->get_gpr(i);
        }
    }

    void Process::restore_context(Arch::Cpu* cpu) const {
        cpu->set_pc(pc);

        for (uint16_t i = 0; i < Config::nregs; i++) {
            cpu->set_gpr(i, regs[i]);
        }

        
        cpu->set_vmem_mode(Arch::Cpu::VmemMode::BaseLimit);
        cpu->set_vmem_paddr_base(base);
        cpu->set_vmem_size(limite);
    }
}