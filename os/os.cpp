#include <string>
#include <sstream>
#include <vector>

#include <cstdint>
#include <algorithm>

#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"

#include "memory_manager.h"
#include "os-lib.h"
#include "process.h"
#include "utils.h"

namespace OS {

Arch::Cpu *cpuglobal;

std::string command_buffer;
Process* proceso_corrente = nullptr;
uint16_t next_pid = 1;
std::vector<Process*> todos_processo;

Process* create_process(const std::string& name, const std::vector<uint16_t>& code) {
    Process* process = new Process(next_pid++, name, code);


    uint16_t size_needed = code.size() + 64; 
    
    if (!memory_manager->allocate_memory_for_process(process, size_needed)) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "sem memória pra alocar o processo ", name);
        delete process;
        return nullptr;
    }
    
    
    for (uint16_t i = 0; i < code.size(); i++) {
        cpuglobal->pmem_write(process->get_base() + i, code[i]);
    }
    
    
    process->set_pc(0);
    
    return process;
}

void kill_proceso_corrente() {
    if (proceso_corrente) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "matando!!! hahahahaha: ", proceso_corrente->get_name());

        auto it = std::find(todos_processo.begin(), todos_processo.end(), proceso_corrente);
        if (it != todos_processo.end()) {
            todos_processo.erase(it);
        }

        memory_manager->free_memory(proceso_corrente);
        delete proceso_corrente;
        proceso_corrente = nullptr;

        Process* processo_do_idlebin = nullptr;
        for (Process* processo : todos_processo) {
            if (processo->get_name() == "idle.bin") {
                processo_do_idlebin = processo;
                break;
            }
        }

        if (processo_do_idlebin) {
            proceso_corrente = processo_do_idlebin;
            proceso_corrente->set_estado(ProcessState::running);
            proceso_corrente->do_mem_protection(cpuglobal);
            proceso_corrente->restore_context(cpuglobal);
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "rpocesso voltando: ", proceso_corrente->get_name());
        } else {
            cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::BaseLimit);
            cpuglobal->set_pc(0);
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "matei!!!!!  (sem mais processos)");
        }

        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "matei!!!!! ");
    } else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo corrente");
    }
}

void run_process(Process* process) {
    
    if (proceso_corrente) {
        kill_proceso_corrente();
    }
    
    
    proceso_corrente = process;
    process->do_mem_protection(cpuglobal);
    process->restore_context(cpuglobal);
    
    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "processo", process->get_name(), " comecou");
}

void kill_process() {
    if (!proceso_corrente) {
        terminal_println(cpuglobal, Terminal::Kernel, "sem processos");
        return;
    }

    terminal_println(cpuglobal, Terminal::Kernel, "MATANDO!! ", proceso_corrente->get_pid(), " (", proceso_corrente->get_name(), ")");

    memory_manager->free_memory(proceso_corrente);
    proceso_corrente = nullptr;
    cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::BaseLimit);
    cpuglobal->set_pc(0);
}

bool load_program(const std::string& filename) {
    if (!memory_manager) {
        memory_manager = new MemoryManager(Config::phys_mem_size_words);
    }

    try {
        terminal_println(cpuglobal, Arch::Terminal::Type::App, "rodandouu");
        terminal_println(cpuglobal, Arch::Terminal::Type::App, filename.data());
        std::vector<uint16_t> tamanho = Lib::load_from_disk_to_16bit_buffer(filename);
        terminal_println(cpuglobal, Arch::Terminal::Type::App, "rodandouu 2");

        static uint16_t next_pid = 1;
        uint16_t pid = next_pid++;

        auto * process = new Process(pid, filename, tamanho);

        uint16_t tamanho_preciso = tamanho.size(); // espaço p stack e mais um pouco
        if (!memory_manager->allocate_memory_for_process(process, tamanho_preciso)) {
            terminal_println(cpuglobal, Terminal::Kernel, "sem memória pra esse nvo processo!");
            memory_manager->free_memory(proceso_corrente);
            return false;
        }

        // logo quando carrego o processo preciso colocar ele na memória física, mas só agr
        for (uint16_t i = 0; i < tamanho.size(); i++) {
            cpuglobal->pmem_write(process->get_base() + i, tamanho[i]);
        }

        process->set_pc(0);


        Utils::setando_novos_regs_pro_processo(process);

        if (proceso_corrente) {
            proceso_corrente->save_context(cpuglobal);
            proceso_corrente->set_estado(ProcessState::running);

            // seta novo processo
            std::vector<Process*>::iterator processo_gambi = std::find(todos_processo.begin(), todos_processo.end(), proceso_corrente);
            if (processo_gambi == todos_processo.end()) {
                todos_processo.push_back(proceso_corrente);
            }

            proceso_corrente = nullptr;
            for (Process* proc : todos_processo) {
                if (proc->get_name() == "idle.bin") {
                    proceso_corrente = proc;
                    break;
                }
            }

            proceso_corrente = todos_processo[0];

            //setando pra voltar pro idle
            if (proceso_corrente) {
                proceso_corrente->set_estado(ProcessState::running);
                proceso_corrente->do_mem_protection(cpuglobal);
                proceso_corrente->restore_context(cpuglobal);
                terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "rpocesso voltando: ", proceso_corrente->get_name());
            } else {
                cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::BaseLimit);
                cpuglobal->set_pc(0);
                terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "matei!!!!!  (sem mais processos para executar)");
            }
        } else {
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo corrente");
        }

        proceso_corrente = process;
        process->do_mem_protection(cpuglobal);
        todos_processo.push_back(process);

        terminal_println(cpuglobal, Terminal::Kernel, "processo ", pid, " (", filename, ") foi");
        terminal_println(cpuglobal, Terminal::Kernel, "base: ", process->get_base(), ", limite: ", process->get_limite());

        return true;
    } catch (const Mylib::Exception& e) {
        terminal_println(cpuglobal, Terminal::Kernel, "excao brabissima!!!! tratar: ", e.what());
        return false;
    }
}

void process_command(const std::string& palavra) {

    std::vector<std::string> args;
    std::string argomento;
    std::istringstream iss(palavra);

    // parsando comando palavra por palavra
    while (iss >> argomento) args.push_back(argomento);

    if (args.empty()) return;

    const std::string& comando = args[0];

    while (!command_buffer.empty() &&
       (command_buffer.back() == '\n' || command_buffer.back() == '\r' ||
        command_buffer.back() == ' ')) {
        command_buffer.pop_back();
    }

    while (!command_buffer.empty() &&
           (command_buffer.front() == '\n' || command_buffer.front() == '\r' ||
            command_buffer.front() == ' ')) {
        command_buffer.erase(0, 1);
    }

    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, comando);

    if (comando == "exit") {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Desligando...");
        shutdown();
        cpuglobal->turn_off();
    }

    else if (comando == "load") {

        if (args.size() < 2) {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Uso: carregar <arquivo>");
            return;
        }

            load_program(args[1]);
    }

    else if (comando == "kill") {

        if (proceso_corrente) {
            kill_proceso_corrente();
        } else {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nsem processos");
        }
    }

    else if (comando == "help") {

        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nCOmandos:");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  load - carregar um arquivo / processo ");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  kill - mata o programa rodando");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  exit - Sair the simulator");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  help - mostra esse menu");
    }

    else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nnao conheco esse comando: ", comando);
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "digite help pra ver os comandos");
    }
    terminal_print(cpuglobal, Arch::Terminal::Type::Command, "\n");
}

void boot(Arch::Cpu *cpu)
{

    cpuglobal = cpu;

    memory_manager = new MemoryManager(Config::phys_mem_size_words);

    load_program("idle.bin");

    terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here.");
    terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
    terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");
    terminal_println(cpu, Arch::Terminal::Type::Command, "Welcome to the OS terminal. Type 'help' for commands.");
    terminal_print(cpu, Arch::Terminal::Type::Command, ">");
}



void interrupt(const Arch::InterruptCode interrupt)
{
	if (interrupt == InterruptCode::Keyboard) {
		uint16_t io = cpuglobal->read_io(IO_Port::TerminalReadTypedChar);
	    char letra = (char) io;

	        if (letra == '\n' || letra == '\r') {
	            process_command(command_buffer);
                command_buffer.clear();
	            terminal_print(cpuglobal, Arch::Terminal::Type::Command, ">");
	        } else if (terminal_is_backspace(letra)) {
	            if (!command_buffer.empty()) {
	                command_buffer.pop_back();
	                terminal_print(cpuglobal, Arch::Terminal::Type::Command, "\r>");
	                terminal_print(cpuglobal, Arch::Terminal::Type::Command, command_buffer);
	            }
	        } else if (letra >= 32 && letra <= 126) {
	            command_buffer += letra;
	            terminal_print(cpuglobal, Arch::Terminal::Type::Command, letra);
	        }
	} else if (interrupt == Arch::InterruptCode::CpuException) {
	    const auto& exception = cpuglobal->get_ref_cpu_exception();

	    terminal_println(cpuglobal, Terminal::Kernel, "excesao das braba", exception.type, "no endereço: ", exception.vaddr);

	    if (proceso_corrente) {
	        terminal_println(cpuglobal, Terminal::Kernel, "MATANDO O PROCESSO!!!!!! dale excessao nele");
	        kill_proceso_corrente();
	    }
	}

}

void syscall () {
    if (!proceso_corrente) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo rodando");
        return;
    }
    
    // pega gpr (general purpose register)
    uint16_t cod_syscall = cpuglobal->get_gpr(0);
    
    switch (cod_syscall) {
        case 0: {
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "=( o processo pediu pra sair ----> tadinho <---");
            std::string process_name = proceso_corrente->get_name();

            auto processo_dea_agora = std::find(todos_processo.begin(), todos_processo.end(), proceso_corrente);
            if (processo_dea_agora != todos_processo.end()) {
                todos_processo.erase(processo_dea_agora);
            }

            memory_manager->free_memory(proceso_corrente);
            proceso_corrente = nullptr;
            Process* processo_do_idlebin = nullptr;

            for (auto* processo : todos_processo) {
                if (processo->get_name() == "idle.bin") processo_do_idlebin = processo;
            }

            if (processo_do_idlebin) {
                proceso_corrente = processo_do_idlebin;
                proceso_corrente->set_estado(ProcessState::running);
                proceso_corrente->do_mem_protection(cpuglobal);
                proceso_corrente = todos_processo[0];
                proceso_corrente->restore_context(cpuglobal);
                terminal_println(cpuglobal, Terminal::Kernel, "voltndo pro idle");
                terminal_println(cpuglobal, Terminal::Kernel, "base idle: ", proceso_corrente->get_base(), " limite idle: ", proceso_corrente->get_limite());
            }
            break;
        }

        case 1:
        {
            uint16_t str_addr = cpuglobal->get_gpr(1);
            std::string output;
            char caracterekk;

            while ((caracterekk = (char) (cpuglobal->vmem_read(str_addr++))) != '\0') {
                output += caracterekk;
            }


            terminal_print(cpuglobal, Arch::Terminal::Type::App, output);
        }
        break;

        case 2:
            terminal_print(cpuglobal, Arch::Terminal::Type::App, "\n");
        break;

        case 3:
        {

            //registrador 1
            uint16_t value = cpuglobal->get_gpr(1);
            terminal_print(cpuglobal, Arch::Terminal::Type::App, value);
        }
        break;

        default:
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "kakakka q porra de cod syscall é esse vei: ", cod_syscall);
        break;
    }
}


void shutdown() {
    if (proceso_corrente) {
        delete proceso_corrente;
        proceso_corrente = nullptr;
    }
    
    if (memory_manager) {
        delete memory_manager;
        memory_manager = nullptr;
    }
}
    
} 