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
Process* current_process = nullptr;
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

void kill_current_process() {
    if (current_process) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Killing process: ", current_process->get_name());
        
        memory_manager->free_memory(current_process);
        
        delete current_process;
        current_process = nullptr;
        
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Process killed");
    } else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "No process running");
    }
}

void run_process(Process* process) {
    
    if (current_process) {
        kill_current_process();
    }
    
    
    current_process = process;
    
    
    process->do_mem_protection(cpuglobal);
    
    
    process->restore_context(cpuglobal);
    
    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Process ", process->get_name(), " started");
}

void process_command() {

    std::istringstream iss(command_buffer);
    std::vector<std::string> tokens;
    std::string token;

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


    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return;
    }

    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, tokens[0]);


    if (tokens[0] == "exit") {

        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Desligando...");
        shutdown();
        cpuglobal->turn_off();
    }
    else if (tokens[0] == "load") {

        if (tokens.size() < 2) {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Uso: carregar <arquivo>");
            return;
        }

        std::string filename = tokens[1];
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Carregando: ", filename);

        try {

            std::vector<uint16_t> program_code = Lib::load_from_disk_to_16bit_buffer(filename);


            Process* process = create_process(filename, program_code);

            if (process) {
                terminal_println(cpuglobal, Arch::Terminal::Type::Command, "deu muito bommm");


                run_process(process);
            } else {
                terminal_println(cpuglobal, Arch::Terminal::Type::Command, "deu muito ruimm");
            }
        } catch (const std::exception& e) {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Erro: ", e.what());
        }
    }
    else if (tokens[0] == "kill") {

        if (current_process) {
            kill_current_process();
        } else {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "sem processos");
        }
    }
    else if (tokens[0] == "help") {

        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "COmandos:");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  load - carregar em arquivo, ");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  kill - mata o programa rodando");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  exit - Sair the simulator");
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  help - mostra esse menu né sonso");
    }
    else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "não conheçi esse comando: ", tokens[0]);
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "digite help pra ver os comandos");
    }
}

void boot(Arch::Cpu *cpu)
{
    terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here.");
    terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
    terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");

    cpuglobal = cpu;

    memory_manager = new MemoryManager(Config::phys_mem_size_words);

    terminal_println(cpu, Arch::Terminal::Type::Command, "Welcome to the OS terminal. Type help for commands.");

    terminal_print(cpu, Arch::Terminal::Type::Command, ">");
}

// implementar ger memoria -> impl carregamento processo -> colocar processo p executar -> e ver se ta funfando pela syscall

void interrupt(const Arch::InterruptCode interrupt)
{
	if (interrupt == Arch::InterruptCode::Keyboard) {
		uint16_t io = cpuglobal->read_io(Arch::IO_Port::TerminalReadTypedChar);
	    char letra = (char) io;

	     if (letra >= 32 && letra <= 126) {
	        command_buffer += letra;
	        terminal_print(cpuglobal, Arch::Terminal::Type::Command, letra);
	    }

	    if (letra == '\n' || letra == '\r') {
	        terminal_println(cpuglobal, Arch::Terminal::Type::Command, '\n');
	        process_command();

	        command_buffer.pop_back();
	        terminal_print(cpuglobal, Arch::Terminal::Type::Command, ">");
	    } else if (letra == '\b' || letra == 8 || letra == 127) {
	        if (!command_buffer.empty()) {
	            command_buffer.pop_back();
	            terminal_print(cpuglobal, Arch::Terminal::Type::Command, "\r>");
	            terminal_print(cpuglobal, Arch::Terminal::Type::Command, command_buffer);
	        }
	    }
	}
}



void syscall ()
{
    if (!current_process) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo rodando");
        return;
    }
    
    
    uint16_t syscall_code = cpuglobal->get_gpr(0);
    
    switch (syscall_code) {
        case 0: 
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "=( o processo pediu pra sair ----> tadinho <---");
            kill_current_process();
            break;
            
        case 1: 
            {
                
                uint16_t str_addr = cpuglobal->get_gpr(1);
                std::string output;
                
                
                uint16_t char_addr = str_addr;
                while (true) {
                    uint16_t char_val = cpuglobal->pmem_read(current_process->get_base() + char_addr);
                    char c = static_cast<char>(char_val);
                    
                    if (c == 0) break; 
                    
                    output += c;
                    char_addr++;
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
    if (current_process) {
        delete current_process;
        current_process = nullptr;
    }
    
    if (memory_manager) {
        delete memory_manager;
        memory_manager = nullptr;
    }
}

} 