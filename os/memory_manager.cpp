//
// Created by mateus on 30/03/25.
//

#include "memory_manager.h"

#include "os-lib.h"

namespace OS {
    MemoryManager* memory_manager = nullptr;

    MemoryManager::MemoryManager(const uint16_t tamanho) : prox_livre(0), TAMANHO_LMAXIMO(tamanho) {
        uint16_t PROXIMO_PAGE = 4096;
        prox_livre = PROXIMO_PAGE;
    }

    MemoryManager::~MemoryManager() = default;

    bool MemoryManager::allocate_memory_for_process(Process *process, uint16_t precisode) {
        if (prox_livre + precisode > TAMANHO_LMAXIMO) {
            return false;
        }

        process->set_limite(precisode);

        prox_livre += precisode;
        return true;
    }

    void MemoryManager::free_memory(Process *process) {
        std::cout << "memoria foi freeada kk";

        uint16_t limite = process->get_limite();

        // o fim do endereço do processo é igual ao próximo endereço livre?
        // se for, é o processo alocado q eu quero -> posso pegar essa memória de volta
    }

    uint16_t MemoryManager::get_free_memory() const {
        return TAMANHO_LMAXIMO - prox_livre;
    }



}

