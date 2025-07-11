//
// Created by mateus on 10/06/25.
//

#include "paging.h"
#include "os.h"

namespace OS {

    Paging* paging = nullptr;
    uint16_t DEU_RUIM_ALOCAR_PAGINA = -1;

    Paging::Paging() : prox_pag_livre(0), paginas_em_uso(0) {
        // paginas todas livres
        for (uint16_t i = 0; i < PAGINAS_TOTAIS; i++) {
            paginas_livres[i] = 1;
        }
        
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

        std::vector<uint16_t> paginas_vao_ser_alocadas;
        paginas_vao_ser_alocadas.reserve(numero_paginas);

        for (uint16_t i = 0; i < numero_paginas; i++) {
            uint16_t pagina_fisica = aloca_pagina_virtual();
            if (pagina_fisica == -1) {
                for (uint16_t pagina : paginas_vao_ser_alocadas) libera_pagina(pagina);
                return false;
            }

            paginas_vao_ser_alocadas.push_back(pagina_fisica);

            uint16_t indice_memoria_virtual = comeco_vmem_pagina + i;

            (*tabela_paginas)[indice_memoria_virtual] = 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::PhyFrameID] = pagina_fisica;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Present] = 1;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Readable] = legivel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Writable] = escrevivel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Executable] = executavel ? 1 : 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Dirty] = 0;
            (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Accessed] = 0;
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
                entrada = 0;
            }
        }
    }
}
