//
// Created by mateus on 29/03/25.
//

#ifndef __SO_BAO_HEADER_PROCESS_H__
#define __SO_BAO_HEADER_PROCESS_H__

#include <cstdint>
#include <string>
#include <vector>

#include "../config.h"
#include "../arch/arch.h"


namespace OS {

    enum class ProcessState {
       pronto,
       rodando,
       bloqueado,
       terminado
    }

    class Process {
        private:


    };
}



#endif //PROCESS_H
