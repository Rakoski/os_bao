//
// Created by mateus on 30/03/25.
//

#include "memory_manager.h"

namespace OS {
    MemoryManager* memory_manager = nullptr;

    MemoryManager::MemoryManager(const uint16_t size) : next_free_addr(1000), PHYSICAL_MEMORY_SIZE(size) {

    }

    MemoryManager* MemoryManager::instance = nullptr;

    void MemoryManager::initialize(uint16_t memory_size) {
        if (instance == nullptr) {
            instance = new MemoryManager(memory_size);
        }
    }

    MemoryManager* MemoryManager::getInstance() {
        return instance;
    }

    void MemoryManager::shutdown() {
        delete instance;
        instance = nullptr;
    }

    MemoryManager::~MemoryManager() = default;

    bool MemoryManager::allocate_memory_for_process(Process *process, uint16_t size_needed) {
        if (next_free_addr + size_needed > PHYSICAL_MEMORY_SIZE) {
            return false;
        }

        process->set_base(next_free_addr);
        process->set_limit(size_needed);

        next_free_addr += size_needed;
        return true;


    }

    void MemoryManager::free_memory(Process *process) {
        std::cout << "memoria foi freeada kk";
        free(process);
        // TODO: fazer isso direito
    }

    uint16_t MemoryManager::get_free_memory() const {
        return PHYSICAL_MEMORY_SIZE - next_free_addr;
    }



}

