//
// Created by mateus on 10/06/25.
//

#ifndef PAGING_H
#define PAGING_H

#include <cstdint>
#include <memory>

#include "process.h"
#include "../config.h"
#include "../arch/arch.h"
#include "my-lib/bit.h"

namespace OS {
    enum class ResultadoAlocarPagina {
        deu_bom,
        acesso_invalido,
        acesso_violante,
        erro_processo_ou_na_tabela
    };

    class Paging {

    private:
        bool encontrar_espaco_consecutivo_pras_pags(Arch::Cpu::PageTable* tabela, uint16_t paginas_necessarias, uint16_t &pagina_inicial, Arch::Cpu* cpuglobal);

    public:

        Mylib::BitSet<Config::phys_mem_size_words / Config::page_size> paginas_livres;
        uint16_t prox_pag_livre;
        uint16_t PAGINAS_TOTAIS = Config::phys_mem_size_words / Config::page_size;
        uint16_t paginas_em_uso;

        Paging();

        uint16_t aloca_pagina_fisica_livre();

        void libera_pagina(uint16_t id_pagina);

        Arch::Cpu::PageTable* cria_tabela_paginas();

        void libera_tabela_paginas(Arch::Cpu::PageTable* tabela_pagina);

        bool mapeia_paginas_pra_um_processo(Arch::Cpu::PageTable* tabela_paginas, uint16_t comeco_vmem_pagina,
            uint16_t numero_paginas, bool legivel, bool escrevivel, bool executavel);

        void libera_paginas_fisicas(Arch::Cpu::PageTable* tabela);

        ResultadoAlocarPagina page_fault(uint16_t endereco, Arch::Cpu::CpuException::Type codigo_erro, Arch::Cpu* cpuglobal, Process* processo_do_momento);

        uint16_t aloca_dinamicamente(Arch::Cpu* cpuglobal, uint16_t tamanho_words, Process* processo);
    };

    extern Paging* paging;
}


#endif //PAGING_H
