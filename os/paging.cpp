//
// Created by mateus on 10/06/25.
//

#include "paging.h"
#include "os.h"

namespace OS {

    Paging* paging = nullptr;

    Paging::Paging() : prox_pag_livre(0), paginas_em_uso(0) {
        for (uint16_t i = 0; i < PAGINAS_TOTAIS; i++) {
            paginas_livres[i] = 1;
        }

        for (uint16_t i = 0; i < 16 && i < PAGINAS_TOTAIS; i++) {
            paginas_livres[i] = 0;
            paginas_em_uso++;
        }
        
        prox_pag_livre = 16;
    }

    uint16_t Paging::aloca_pagina() {
        for (uint16_t i = prox_pag_livre; i < PAGINAS_TOTAIS; i++) {
            if (paginas_livres[i]) {
                paginas_livres[i] = 0;
                paginas_em_uso++;

                prox_pag_livre = i + 1;
                if (prox_pag_livre >= PAGINAS_TOTAIS) {
                    prox_pag_livre = 0;
                }

                return i;
            }
        }

        for (uint16_t i = 0; i < prox_pag_livre; i++) {
            if (paginas_livres[i]) {
                paginas_livres[i] = 0;
                paginas_em_uso++;
                prox_pag_livre = i + 1;
                return i;
            }
        }

        return -1;
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
        uint16_t page_id = aloca_pagina();
        if (page_id == -1) {
            return nullptr;
        }

        uint32_t phys_addr = page_id * Config::page_size;
        
        Arch::Cpu::PageTable* page_table = new Arch::Cpu::PageTable();
        
        return page_table;
    }

    void Paging::libera_tabela_paginas(Arch::Cpu::PageTable* tabela_pagina) {
        libera_mapeamento(tabela_pagina);
        
        delete tabela_pagina;
    }

    bool Paging::mapeia_paginas_pra_um_processo(Arch::Cpu::PageTable* tabela_paginas, 
                                                uint16_t comeco_vmem_pagina,
                                                uint16_t numero_paginas, 
                                                bool legivel, bool escrevivel, bool executavel) {
        std::vector<uint16_t> allocated_pages;
        
        for (uint16_t i = 0; i < numero_paginas; i++) {
            uint16_t phys_page = aloca_pagina();
            if (phys_page == -1) {
                for (uint16_t page : allocated_pages) {
                    libera_pagina(page);
                }
                return false;
            }
            allocated_pages.push_back(phys_page);
            
        }
        
        return true;
    }

}