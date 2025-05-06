//
// Created by mateus on 03/05/25.
//

#include "utils.h"

void Utils::setando_novos_regs_pro_processo(OS::Process* process) {
        for (uint16_t i = 0; i < Config::nregs; i++) {
            process->set_regs(i, 0);
        }
    };
