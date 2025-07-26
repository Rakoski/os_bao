//
// Created by mateus on 10/06/25.
//

#include "paging.h"

#include "os-lib.h"
#include "os.h"
#include "process.h"
#include "process_manager.h"

namespace OS {

    Paging* paging = nullptr;
    uint16_t DEU_RUIM_ALOCAR_PAGINA = -1;

    std::vector<bool> paginas_livres;
    uint16_t prox_pag_livre;
    uint16_t paginas_em_uso;

    Paging::Paging() : prox_pag_livre(0), paginas_em_uso(0) {
        // paginas todas livres
        paginas_livres.set(0, PAGINAS_TOTAIS, 1);
        prox_pag_livre = 0;
    }

    uint16_t Paging::aloca_pagina_fisica_livre() {
        // procurandou
        for (uint16_t i = 0; i < prox_pag_livre; i++) {
            if (paginas_livres[i]) {
                paginas_livres[i] = 0;
                paginas_em_uso++;
                prox_pag_livre = i + 1;
                return i;
            }
        }

        return DEU_RUIM_ALOCAR_PAGINA;
    }

    void Paging::libera_pagina(uint16_t id_pagina) {
        if (id_pagina < PAGINAS_TOTAIS && !paginas_livres[id_pagina]) {
            paginas_livres[id_pagina] = 1;
            paginas_em_uso--;
            
            if (id_pagina < prox_pag_livre) {
                prox_pag_livre = id_pagina;
            }
        }
    }

    Arch::Cpu::PageTable* Paging::cria_tabela_paginas() {
        Arch::Cpu::PageTable* novaTabela = new Arch::Cpu::PageTable();

        // present 0 nao ta na mem fis
        // present 1 ta na mem fis; isso confunde pakas
        // pte - page table entries
        for (uint16_t i = 0; i < Config::ptes_per_table; i++) {
            (*novaTabela)[i] = 0;
            (*novaTabela)[i][Arch::Cpu::PteField::Present] = 0;
            (*novaTabela)[i][Arch::Cpu::PteField::PhyFrameID] = 0; // pra ter demand paging precisa de informacao nos bits nao usados
        }
        return novaTabela;
    }

    void Paging::libera_tabela_paginas(Arch::Cpu::PageTable* tabela_pagina) {
        if (!tabela_pagina) return;

        libera_paginas_fisicas(tabela_pagina);
        
        delete tabela_pagina;
    }

    bool Paging::mapeia_paginas_pra_um_processo(Arch::Cpu::PageTable* tabela_paginas, 
                                                uint16_t comeco_vmem_pagina,
                                                uint16_t numero_paginas, 
                                                bool legivel, bool escrevivel, bool executavel) {
        bool mapeamento_passa_limite_tabela = comeco_vmem_pagina + numero_paginas > Config::ptes_per_table;

        if (!tabela_paginas || mapeamento_passa_limite_tabela) return false;

        for (uint16_t i = 0; i < numero_paginas; i++) {
            uint16_t indice_memoria_virtual = comeco_vmem_pagina + i;
            (*tabela_paginas)[indice_memoria_virtual] = 0; // "alocação de memória só será efetivada no primeiro acesso."
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Present] = 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Readable] = legivel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Writable] = escrevivel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Executable] = executavel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Dirty] = 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Accessed] = 0;

            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Foo] = 1; // valido mas nao tem nada ainda

        }
        return true;
    }

    void Paging::libera_paginas_fisicas(Arch::Cpu::PageTable* tabela_do_processo) {
        if (!tabela_do_processo) return;

        for (uint16_t i = 0; i < Config::ptes_per_table; i++) {
            Arch::Cpu::PageTableEntry& entrada = (*tabela_do_processo)[i];

            //pagina ta on, temos que liberar a página física
            if (entrada[Arch::Cpu::PteField::Present] == 1) {
                uint16_t pagina_fisica = entrada[Arch::Cpu::PteField::PhyFrameID];
                libera_pagina(pagina_fisica);
            }
            entrada = 0;
        }
    }

    ResultadoAlocarPagina Paging::page_fault(uint16_t endereco, Arch::Cpu::CpuException::Type codigo_erro, Arch::Cpu* cpuglobal, Process* processo_do_momento) {
        uint16_t pagina_memoria_virtual = endereco >> Config::page_size_bits; /// kkkkk divisao não pode pqp its over

        Arch::Cpu::PageTable* tabela = processo_do_momento->get_tabela_paginas();

        Arch::Cpu::PageTableEntry& entrada = (*tabela)[pagina_memoria_virtual];
        bool valida_mas_nao_presente = entrada[Arch::Cpu::PteField::Foo] == 1 && entrada[Arch::Cpu::PteField::Present] == 0;
        bool acesso_incorreto = entrada[Arch::Cpu::PteField::Foo] == 0;
        bool acesso_violante = entrada[Arch::Cpu::PteField::Present] == 1;

        if (valida_mas_nao_presente) {
            terminal_println(cpuglobal, Terminal::Kernel, "demanda paging ok");
            uint16_t pagina_finsica = aloca_pagina_fisica_livre();

            if (pagina_finsica == DEU_RUIM_ALOCAR_PAGINA) {
                terminal_println(cpuglobal, Terminal::Kernel, "DEU RUIM ALOCAR PÁGINA");
                return ResultadoAlocarPagina::erro_processo_ou_na_tabela;
            }

            entrada[Arch::Cpu::PteField::PhyFrameID] = pagina_finsica;
            entrada[Arch::Cpu::PteField::Present] = 1;
            entrada[Arch::Cpu::PteField::Accessed] = 1;

            uint16_t endereco_fisico = pagina_finsica * Config::page_size;

            for (uint16_t i = 0; i < Config::page_size; i++) {
                cpuglobal->pmem_write(endereco_fisico + 1, 0);
            }

            terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada no endereco: " + endereco_fisico);
            return ResultadoAlocarPagina::deu_bom;
        }
        if (acesso_incorreto) {
            return ResultadoAlocarPagina::acesso_invalido;
        }
        if (acesso_violante) {
            terminal_println(cpuglobal, Terminal::Kernel, "acesso_violante");

            switch (codigo_erro) {
                case Arch::Cpu::CpuException::Type::VmemGPFnotReadable:
                    terminal_println(cpuglobal, Terminal::Kernel, "VmemGPFnotReadable");
                    break;
                case Arch::Cpu::CpuException::Type::VmemGPFnotWritable:
                    terminal_println(cpuglobal, Terminal::Kernel, "VmemGPFnotWritable");
                    break;
                case Arch::Cpu::CpuException::Type::VmemGPFnotExecutable:
                    terminal_println(cpuglobal, Terminal::Kernel, "VmemGPFnotExecutable");
                    break;
                default:
                    break;
            }

            return ResultadoAlocarPagina::acesso_violante;
        }
        return ResultadoAlocarPagina::erro_processo_ou_na_tabela;
    }

    uint16_t Paging::aloca_dinamicamente(uint16_t tamanho_words, Process *processo) {
        Arch::Cpu::PageTable* tabela = processo->get_tabela_paginas();

        uint16_t paginas_necessarias = (tamanho_words + Config::page_size - 1) / Config::page_size;

        uint16_t pagina_inicial = 0;

    }


}
