#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include <cstdint>
#include <cstdlib>

#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"

#include "memory_manager.h"
#include "os-lib.h"
#include "process.h"

namespace OS {

Arch::Cpu *cpuglobal;

std::string command_buffer;
Process* proceso_corrente = nullptr;
uint16_t next_pid = 1;

Process* create_process(const std::string& name, const std::vector<uint16_t>& code) {
    Process* process = new Process(next_pid++, name, code);


    uint16_t size_needed = code.size() + 64; 
    
    if (!memory_manager->allocate_memory_for_process(process, size_needed)) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Error: Not enough memory to allocate process ", name);
        delete process;
        return nullptr;
    }
    
    
    for (uint16_t i = 0; i < code.size(); i++) {
        cpuglobal->pmem_write(process->get_base() + i, code[i]);
    }
    
    
    process->set_program_counter(0);
    
    return process;
}

void kill_proceso_corrente() {
    if (proceso_corrente) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "matando!!! hahahahaha: ", proceso_corrente->get_name());
        
        memory_manager->free_memory(proceso_corrente);
        
        delete proceso_corrente;
        proceso_corrente = nullptr;
        
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
    
    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Process ", process->get_name(), " started");
}

void kill_process() {
    if (!proceso_corrente) {
        terminal_println(cpuglobal, Terminal::Kernel, "No process is currently running");
        return;
    }

    terminal_println(cpuglobal, Terminal::Kernel, "Killing process ", proceso_corrente->get_pid(), " (", proceso_corrente->get_name(), ")");

    if (memory_manager) {
        memory_manager->free_memory(proceso_corrente);
    }

    delete proceso_corrente;
    proceso_corrente = nullptr;
    cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Disabled);
    cpuglobal->set_pc(0);
}

bool load_program(const std::string& filename) {
    if (!memory_manager) {
        memory_manager = new MemoryManager(Config::phys_mem_size_words);
    }

    try {
        terminal_println(cpuglobal, Arch::Terminal::Type::App, "rodandouu");
        terminal_println(cpuglobal, Arch::Terminal::Type::App, filename.data());
        std::vector<uint16_t> code = Lib::load_from_disk_to_16bit_buffer(filename);
        terminal_println(cpuglobal, Arch::Terminal::Type::App, "rodandouu 2");

        static uint16_t next_pid = 1;
        uint16_t pid = next_pid++;

        auto * process = new Process(pid, filename, code);

        uint16_t size_needed = code.size();
        if (!memory_manager->allocate_memory_for_process(process, size_needed)) {
            terminal_println(cpuglobal, Terminal::Kernel, "Error: Not enough memory to load process");
            delete process;
            return false;
        }

        for (uint16_t i = 0; i < code.size(); i++) {
            cpuglobal->pmem_write(process->get_base() + i, code[i]);
        }

        process->set_program_counter(0);
        for (uint16_t i = 0; i < Config::nregs; i++) {
            process->set_registrator(i, 0);
        }

        if (proceso_corrente) {
            kill_process();
        }

        proceso_corrente = process;
        process->do_mem_protection(cpuglobal);
        cpuglobal->set_pc(0);

        terminal_println(cpuglobal, Terminal::Kernel, "Process ", pid, " (", filename, ") loaded successfully");
        terminal_println(cpuglobal, Terminal::Kernel, "Base: ", process->get_base(), ", Limit: ", process->get_limit());

        return true;
    } catch (const Mylib::Exception& e) {
        terminal_println(cpuglobal, Terminal::Kernel, e.what());
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
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "sem processos");
        }
    }
    else if (comando == "help") {

        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nCOmandos:");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  load - carregar em arquivo, ");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  kill - mata o programa rodando");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  exit - Sair the simulator");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  help - mostra esse menu né sonso");
    }
    else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "não conheçi esse comando: ", comando);
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "digite help pra ver os comandos");
    }
    terminal_print(cpuglobal, Arch::Terminal::Type::Command, "\n");
}

void boot(Arch::Cpu *cpu)
{

    cpuglobal = cpu;

    memory_manager = new MemoryManager(Config::phys_mem_size_words);

    terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here.");
    terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
    terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");
    terminal_println(cpu, Arch::Terminal::Type::Command, "Welcome to the OS terminal. Type 'help' for commands.");
    terminal_print(cpu, Arch::Terminal::Type::Command, ">");
}



void interrupt(const Arch::InterruptCode interrupt)
{
	if (interrupt == Arch::InterruptCode::Keyboard) {
		uint16_t io = cpuglobal->read_io(Arch::IO_Port::TerminalReadTypedChar);
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

void syscall ()
{
    if (!proceso_corrente) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo rodando");
        return;
    }
    
    
    uint16_t syscall_code = cpuglobal->get_gpr(0);
    
    switch (syscall_code) {
        case 0: 
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "=( o processo pediu pra sair ----> tadinho <---");
            kill_proceso_corrente();
            break;
            
        case 1: 
            {
                uint16_t str_addr = cpuglobal->get_gpr(1);
                std::string output;
                char caracterekk;
                
                while ((caracterekk = static_cast<char>(cpuglobal->vmem_read(str_addr++))) != '\0') {
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

                uint16_t value = cpuglobal->get_gpr(1);
                terminal_print(cpuglobal, Arch::Terminal::Type::App, value);
            }
            break;
            
        default:
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "kakakka q porra de cod syscall é esse vei: ", syscall_code);
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