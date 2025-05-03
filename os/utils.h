//
// Created by mateus on 03/05/25.
//

#ifndef UTILS_H
#define UTILS_H
#include <cstdint>

#include "process.h"
#include "../config.h"


class Utils {
public:
    static void setando_novos_regs_pro_processo(OS::Process* process);
};



#endif //UTILS_H
