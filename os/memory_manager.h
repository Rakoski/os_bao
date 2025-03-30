//
// Created by mateus on 30/03/25.
//

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <cstdint>

#include "process.h"


namespace OS {

    class MemoryManager {
    private:
        uint16_t next_free_addr;
        const uint16_t PHYSICAL_MEMORY_SIZE;

    public:
         explicit MemoryManager(uint16_t memory_size);
        ~MemoryManager();

        bool allocate_memory_for_process(Process* process, uint16_t size_needed);
        void free_memory(Process* process);

        [[nodiscard]] uint16_t get_free_memory() const;
    };

     extern MemoryManager* memory_manager;

} // end namespace OS



#endif //MEMORY_MANAGER_H
