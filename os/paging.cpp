//
// Created by mateus on 10/06/25.
//

#include "paging.h"
#include <cmath>
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
        paginas_livres.resize(PAGINAS_TOTAIS, true);
        prox_pag_livre = 0;
    }

    uint16_t Paging::aloca_pagina_fisica_livre() {
        for (uint16_t i = prox_pag_livre; i < PAGINAS_TOTAIS; i++) {
            if (paginas_livres[i]) {
                paginas_livres[i] = false;
                paginas_em_uso++;
                prox_pag_livre = i + 1;
                return i;
            }
        }

        for (uint16_t i = 0; i < prox_pag_livre; i++) {
            if (paginas_livres[i]) {
                paginas_livres[i] = false;
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

    bool Paging::mapeia_e_carrega_codigo(Arch::Cpu::PageTable* tabela_paginas,
                                     uint16_t comeco_vmem_pagina,
                                     uint16_t numero_paginas,
                                     const std::vector<uint16_t>& codigo,
                                     Arch::Cpu* cpuglobal) {
    bool mapeamento_passa_limite_tabela = comeco_vmem_pagina + numero_paginas > Config::ptes_per_table;
        terminal_println(cpuglobal, Terminal::Kernel, "mapeia_e_carrega_codigo: carregando ", numero_paginas, " páginas");

    if (!tabela_paginas || mapeamento_passa_limite_tabela) return false;

    for (uint16_t pagina_memoria = 0; pagina_memoria < numero_paginas; pagina_memoria++) {
        uint16_t indice_memoria_virtual = comeco_vmem_pagina + pagina_memoria;

        uint16_t pagina_fisica = aloca_pagina_fisica_livre();
        if (pagina_fisica == DEU_RUIM_ALOCAR_PAGINA) {
            for (uint16_t j = 0; j < pagina_memoria; j++) {
                uint16_t pagina_para_liberar = (*tabela_paginas)[comeco_vmem_pagina + j][Arch::Cpu::PteField::PhyFrameID];
                libera_pagina(pagina_para_liberar);
                (*tabela_paginas)[comeco_vmem_pagina + j] = 0;
            }
            return false;
        }

        (*tabela_paginas)[indice_memoria_virtual] = 0;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Present] = 1;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::PhyFrameID] = pagina_fisica;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Readable] = 1;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Writable] = 1;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Executable] = 1;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Dirty] = 0;
        (*tabela_paginas)[indice_memoria_virtual][Arch::Cpu::PteField::Accessed] = 0;

        uint16_t endereco_fisico = pagina_fisica * Config::page_size;
        carregar_codigo_direto(pagina_memoria, endereco_fisico, cpuglobal, codigo);
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

    void Paging::carregar_codigo_direto(uint16_t pagina_virtual, uint16_t endereco_fisico,
                                   Arch::Cpu *cpuglobal, const std::vector<uint16_t>& codigo) {
        for (uint16_t i = 0; i < Config::page_size; i++) {
            uint16_t offset = pagina_virtual * Config::page_size + i;
            uint16_t valor = (offset < codigo.size()) ? codigo[offset] : 0;
            cpuglobal->pmem_write(endereco_fisico + i, valor);
        }
    }

    bool Paging::verificar_violacao_protecao(Arch::Cpu::PageTableEntry& entrada, Arch::Cpu::CpuException::Type codigo_erro) {
        if (entrada[Arch::Cpu::PteField::Present] != 1) {
            return false;
        }

        switch (codigo_erro) {
            case Arch::Cpu::CpuException::Type::VmemGPFnotReadable:
                return entrada[Arch::Cpu::PteField::Readable] == 0;
            case Arch::Cpu::CpuException::Type::VmemGPFnotWritable:
                return entrada[Arch::Cpu::PteField::Writable] == 0;
            case Arch::Cpu::CpuException::Type::VmemGPFnotExecutable:
                return entrada[Arch::Cpu::PteField::Executable] == 0;
            default:
                return false;
        }
    }

    ResultadoAlocarPagina Paging::page_fault(uint16_t endereco, Arch::Cpu::CpuException::Type codigo_erro, Arch::Cpu* cpuglobal, Process* processo_do_momento) {
        uint16_t pagina_memoria_virtual = endereco >> Config::page_size_bits; /// kkkkk divisao não pode pqp its over
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "chamando page fault pro processo: ", processo_do_momento->get_name());

        Arch::Cpu::PageTable* tabela = processo_do_momento->get_tabela_paginas();

        Arch::Cpu::PageTableEntry& entrada = (*tabela)[pagina_memoria_virtual];
        bool valida_mas_nao_presente = entrada[Arch::Cpu::PteField::Foo] == 1 && entrada[Arch::Cpu::PteField::Present] == 0;
        bool acesso_incorreto = entrada[Arch::Cpu::PteField::Foo] == 0;
        bool violante = verificar_violacao_protecao(entrada, codigo_erro);

        if (valida_mas_nao_presente) {
            terminal_println(cpuglobal, Terminal::Kernel, "demanda paging ok");
            uint16_t pagina_finsica = aloca_pagina_fisica_livre();
            terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada: " + pagina_finsica);

            if (pagina_finsica == DEU_RUIM_ALOCAR_PAGINA) {
                terminal_println(cpuglobal, Terminal::Kernel, "DEU RUIM ALOCAR PÁGINA");
                return ResultadoAlocarPagina::erro_processo_ou_na_tabela;
            }

            entrada[Arch::Cpu::PteField::PhyFrameID] = pagina_finsica;
            entrada[Arch::Cpu::PteField::Present] = 1;
            entrada[Arch::Cpu::PteField::Accessed] = 1;

            uint16_t endereco_fisico = pagina_finsica * Config::page_size;

            for (uint16_t i = 0; i < Config::page_size; i++) {
                cpuglobal->pmem_write(endereco_fisico + i, 0);
            }

            terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada no endereco: " + endereco_fisico);

            return ResultadoAlocarPagina::deu_bom;
        }
        if (acesso_incorreto) {
            return ResultadoAlocarPagina::acesso_invalido;
        }
        if (violante) {
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
            }

            return ResultadoAlocarPagina::acesso_violante;
        }
        return ResultadoAlocarPagina::erro_processo_ou_na_tabela;
    }

    uint16_t Paging::aloca_dinamicamente(Arch::Cpu* cpuglobal, uint16_t tamanho_solicitado, Process *processo) {
        Arch::Cpu::PageTable* tabela = processo->get_tabela_paginas();

        // quantas paginas são necessárioas
        // a conta - tanto de words (tipo 20) + configuração (16) - 1 / 16  (ceiling division) OU ceil(tanto de words / config)
        // esse menos 1 pesquisei era tipo se vc tem 17 words e cada página tem 16 eu preciso de 2 páginas inteiras, não 1,0625 páginas
        // perguntar isso pro prof nao entendi muito bem
        uint16_t paginas_necessarias = std::ceil((double) (tamanho_solicitado) / Config::page_size);

        uint16_t pagina_inicial = 0;
        bool tem_espaco = encontrar_espaco_consecutivo_pras_pags(tabela, paginas_necessarias, pagina_inicial, cpuglobal);

        if (!tem_espaco) {
            terminal_println(cpuglobal, Terminal::Kernel, "não encontrou espaço p alocacao dinamica");
            return DEU_RUIM_ALOCAR_PAGINA;
        }

        mapeia_paginas_pra_um_processo(tabela, pagina_inicial, paginas_necessarias, true, true, false); // area de dados nao pode executar? perguntar pro prof

        // como calcular o endereço virtual é dividido em offset e numero
        // Endereço físico 7 × 4096 + 742 = 29414 // do pdf
        // TENHO UM ENDEREÇO E QUERO ACHAR A PÁGINA FÍSICA
        // número_da_página = endereço_virtual / tamanho_da_página;
        // offset = endereço_virtual % tamanho_da_página;

        // TENHO A PÁGINA virtual - como as páginas são consecutivas na memória
        // a página 3 -> sempre vai ser dos endereço 12288 ao 16383

        uint16_t endereco_virtual = pagina_inicial * Config::page_size;

        processo->colocar_alocacao(endereco_virtual, paginas_necessarias, tamanho_solicitado);

        // alocar
        // 1 - se der bom coloca r1 com resultado 1
        // 2 - se der bom r2 resultado mem virtual

        // se der errado
        // gpr 1 com resultado 0

        // desalocar
        // lembrar de desalocar todo o espaco consecutivo que foi alocado no 4
        // 1 se desalocar
        // 2 se nao desalocar

        // guardar a informacao de quanto foi alocado a partir de tal endereço

        return endereco_virtual;
    }

    bool Paging::encontrar_espaco_consecutivo_pras_pags(Arch::Cpu::PageTable* tabela, uint16_t paginas_necessarias, uint16_t &pagina_inicial, Arch::Cpu* cpuglobal) {
        terminal_println(cpuglobal, Terminal::Kernel, "encontrando espaço junto consec: ");
        uint16_t pags_consec = 0;
        pagina_inicial = 0;


        for (uint16_t i = 0; i < Config::ptes_per_table; i++) {
            if ((*tabela)[i][Arch::Cpu::PteField::Foo] == 0 && (*tabela)[i][Arch::Cpu::PteField::Present] == 0) {
                if (pags_consec == 0) pagina_inicial = i;

                pags_consec++;
                if (pags_consec >= paginas_necessarias) return true;

            }
            else pags_consec = 0;
        }

        terminal_println(cpuglobal, Terminal::Kernel, "retornou false ao encontrar espaço consecutivo: ");
        return false;
    }

    bool Paging::desaloca_a_partir_de_tal_endereco(Arch::Cpu* cpuglobal, uint16_t endereco, Process* processo) {
        AreaMemoriaVirtual* alocacao_area = processo->obter_alocacao(endereco);

        if (!alocacao_area) {
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "kk não tem área q alocou");
            return false;
        }

        Arch::Cpu::PageTable* tabela = processo->get_tabela_paginas();
        uint16_t pagina_inicial = endereco / Config::page_size;

        for (uint16_t i = 0; i < alocacao_area->numero_pag; i++) {
            uint16_t pagina_atual = pagina_inicial + i;

            if (pagina_atual < Config::ptes_per_table) {
                Arch::Cpu::PageTableEntry& entrada = (*tabela)[pagina_atual];

                if (entrada[Arch::Cpu::PteField::Present] == 1) {
                    uint16_t pagina_fisica = entrada[Arch::Cpu::PteField::PhyFrameID];
                    libera_pagina(pagina_fisica);
                }

                entrada = 0;
            }
        }

        return processo->alocacoes.erase(endereco);
    }
}
