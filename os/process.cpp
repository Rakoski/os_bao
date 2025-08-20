#ifndef __SO_BAO_HEADER_PROCESS_MANAGER_H__
#define __SO_BAO_HEADER_PROCESS_MANAGER_H__

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "../arch/arch.h"
#include "process.h"

namespace OS {
    Process::Process(uint16_t pid, const std::string& name, const std::vector<uint16_t>& codigo_proceso)
    : pid(pid), name(name), estado(ProcessState::ready), pc(0), limite(0), codigo_processo(codigo_proceso) {

        for (uint16_t& reg : regs) {
            reg = 0;
        }
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

    void Process::salvar_contexto(Arch::Cpu* cpu) {
        pc = cpu->get_pc();

        for (uint16_t i = 0; i < Config::nregs; i++) {
            regs[i] = cpu->get_gpr(i);
        }
    }

    void Process::restaurar_contexto(Arch::Cpu* cpu) const {
        cpu->set_pc(pc);

        for (uint16_t i = 0; i < Config::nregs; i++) {
            cpu->set_gpr(i, regs[i]);
        }

        cpu->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
    }

    void Process::colocar_alocacao(uint16_t endereco, uint16_t numero_pag, uint16_t tamanho_words) {
        alocacoes.insert({endereco, AreaMemoriaVirtual(endereco, numero_pag, tamanho_words)});
    }

    AreaMemoriaVirtual* Process::obter_alocacao(uint16_t endereco) {
        auto it = alocacoes.find(endereco);
        return (it != alocacoes.end()) ? &it->second : nullptr;
    }
}

#endif // __SO_BAO_HEADER_PROCESS_MANAGER_H__

