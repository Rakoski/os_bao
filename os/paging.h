//
// Created by mateus on 10/06/25.
//

#ifndef PAGING_H
#define PAGING_H


#include <cstdint>
#include <vector>
#include <memory>
#include <bitset>

#include "../config.h"
#include "../arch/arch.h"
#include "process.h"

class Paging {

    std::bitset<Config::phys_mem_size_words / Config::page_size> paginas_livres;

    uint16_t prox_pag_livre;

    uint16_t paginas_totais = Config::phys_mem_size_words / Config::page_size;

public
    Paging();
    ~Paging();

    uint16_t aloca_pagina();

    void libera_pagina(uint16_t id_pagina);

    Arch::Cpu::PageTable* cria_tabela_paginas();

    void libera_tabela_paginas(Arch::Cpu::PageTable* tabela_pagina);

    bool mapeia_paginas_pra_um_processo(Arch::Cpu::PageTable* tabela_paginas, uint16_t comeco_vmem_pagina,
        uint16_t numero_paginas, bool legivel, bool escrevivel, bool executavel);

    void libera_mapeamento(Arch::Cpu::PageTable* tabela);


};


#endif //PAGING_H
