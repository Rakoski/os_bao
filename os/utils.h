//
// Created by mateus on 03/05/25.
//

#ifndef UTILS_H
#define UTILS_H

#include "process.h"

namespace OS {
    class Utils {
    public:
        static void setando_novos_regs_pro_processo(OS::Process* process);
        static void printar_help();
        static void exibir_info_processo(OS::Process* processo);
        static OS::Process* find_idle();
    };
}

#endif //UTILS_H
