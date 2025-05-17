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

Arch::Cpu* cpuglobal = nullptr;
std::string command_buffer;
Process* processo_rodando_no_momento = nullptr;
uint16_t proximo_pid = 1;
std::vector<Process*> processos_rodando;

void kill_process();
void rerodar_idle();
bool load_program(const std::string& filename);
void tratar_interrupcao_teclado();
void tratar_excecao();
void syscall_0();
void syscall_1();

Process* find_idle() {
    for (Process* processo : processos_rodando) {
        if (processo->get_name() == "idle.bin") {
            return processo;
        }
    }
    return nullptr;
}

bool alocar_memoria_para_processo(Process* process, const std::vector<uint16_t>& codigo) {
    if (uint16_t tamanho_preciso = codigo.size(); !memory_manager->allocate_memory_for_process(process, tamanho_preciso)) {
        terminal_println(cpuglobal, Terminal::Kernel, "sem memória pra esse nvo processo!");
        memory_manager->free_memory(processo_rodando_no_momento);
        return false;
    }

    for (uint16_t i = 0; i < codigo.size(); i++) {
        cpuglobal->pmem_write(process->get_base() + i, codigo[i]);
    }

    return true;
}

Process* criar_e_configurar_processo(const std::string& filename, const std::vector<uint16_t>& codigo) {
    static uint16_t proximo_pid = 1;
    uint16_t pid = proximo_pid++;

    Process* process = new Process(pid, filename, codigo);

    if (!alocar_memoria_para_processo(process, codigo)) {
        return nullptr;
    }

    process->set_pc(1);
    Utils::setando_novos_regs_pro_processo(process);

    return process;
}

std::vector<uint16_t> carregar_codigo_do_arquivo(const std::string& filename) {
    terminal_println(cpuglobal, Arch::Terminal::Type::App, "ta rodando: ", filename);
    std::vector<uint16_t> codigo = Lib::load_from_disk_to_16bit_buffer(filename);
    terminal_println(cpuglobal, Arch::Terminal::Type::App, "ta rodando 2: ", filename);
    return codigo;
}

void gerenciar_contexto() {
    Process* idle = find_idle();
    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        processo_rodando_no_momento->restore_context(cpuglobal);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "voltndo para o processo: ", processo_rodando_no_momento->get_name());
    } else {
        cpuglobal->set_vmem_mode(VmemMode::BaseLimit);
        cpuglobal->set_pc(0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "cade o idle que tava aqui ?");
    }
}

void gerenciar_troca_de_contexto() {
    processo_rodando_no_momento = processos_rodando[0];
    processo_rodando_no_momento->set_estado(ProcessState::running);
    processo_rodando_no_momento->restore_context(cpuglobal);
}

void exibir_info_processo(Process* process) {
    terminal_println(cpuglobal, Terminal::Kernel, "processo ", process->get_pid()," (", process->get_name(), ") foi carregado\n");
    terminal_println(cpuglobal, Terminal::Kernel, "base: ", process->get_base(),", limite: ", process->get_limite(), "\n");
}

Process* create_process(const std::string& name, const std::vector<uint16_t>& code) {
    Process* process = new Process(proximo_pid++, name, code);

    if (uint16_t size_needed = code.size() + 64; !memory_manager->allocate_memory_for_process(process, size_needed)) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "sem memória para alocar p processo: ", name);
        delete process;
        return nullptr;
    }

    for (uint16_t i = 0; i < code.size(); i++) {
        cpuglobal->pmem_write(process->get_base() + i, code[i]);
    }

    process->set_pc(1);

    return process;
}

void run_process() {
    if (!processo_rodando_no_momento) return;

    processo_rodando_no_momento->save_context(cpuglobal);
    processo_rodando_no_momento->set_estado(ProcessState::running);

    auto processo = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);
    if (processo == processos_rodando.end()) {
        processos_rodando.push_back(processo_rodando_no_momento);
    }
}


void rerodar_idle() {
    Process* idle = find_idle();

    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        processo_rodando_no_momento->protege(cpuglobal);
        processo_rodando_no_momento->restore_context(cpuglobal);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "vontado pro processo: ", processo_rodando_no_momento->get_name());
    }
}

void free_processo() {
    memory_manager->free_memory(processo_rodando_no_momento);
    delete processo_rodando_no_momento;

    Process* idle = find_idle();
    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        processo_rodando_no_momento->restore_context(cpuglobal);
    } else {
        processo_rodando_no_momento = nullptr;
    }
}

void kill_process() {
    if (processo_rodando_no_momento && processo_rodando_no_momento->get_name() == "idle.bin") {
        terminal_println(cpuglobal, Terminal::Kernel, "Não pode matar o processo idle!");
        return;
    }

    auto it = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);
    if (it != processos_rodando.end()) {
        processos_rodando.erase(it);
    }

    free_processo();
}

void printar_help() {
    terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nComandos disponíveis:");
    terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  load <arquivo> - Carrega um programa/processo");
    terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  kill - Termina o processo atual");
    terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  exit - Sai do simulador");
    terminal_println(cpuglobal, Arch::Terminal::Type::Command, "  help - Mostra este menu de ajuda");
}

void processar_comandos(std::string comando, std::vector<std::string> args) {
    if (comando == "exit") {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "Desligando...");
        shutdown();
    } else if (comando == "load") {
        if (args.size() < 2 || args[1] == "idle.bin") {
            terminal_println(cpuglobal, Arch::Terminal::Type::Command, "\nUso: load <arquivo>");
            return;
        }
        load_program(args[1]);
    } else if (comando == "kill") {
        kill_process();
    } else if (comando == "help") {
        printar_help();
    }
    else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Digite 'help' para ver os comandos disponíveis");
    }
}

void process_command(const std::string& palavra) {
    std::vector<std::string> args;
    std::string argomento;
    std::istringstream iss(palavra);

    while (iss >> argomento) {
        args.push_back(argomento);
    }

    if (args.empty()) return;

    const std::string& comando = args[0];

    std::string clean_command = command_buffer;
    while (!clean_command.empty() && (clean_command.back() == '\n' || clean_command.back() == '\r' || clean_command.back() == ' ')) {
        clean_command.pop_back();
    }

    while (!clean_command.empty() && (clean_command.front() == '\n' || clean_command.front() == '\r' || clean_command.front() == ' ')) {
        clean_command.erase(0, 1);
    }

    processar_comandos(comando, args);
    terminal_print(cpuglobal, Arch::Terminal::Type::Command, "\n");
}


void tratar_excecao() {
    const auto& exception = cpuglobal->get_ref_cpu_exception();
    terminal_println(cpuglobal, Terminal::Kernel, "excesao brabissima\n", exception.type, "\nno endereço: \n", exception.vaddr);

    if (!processo_rodando_no_momento) return;

    if (processo_rodando_no_momento->get_name() == "idle.bin") {
        terminal_println(cpuglobal, Terminal::Kernel, "Exceção no processo idle, reiniciando...");
        auto it = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);
        if (it != processos_rodando.end()) {
            processos_rodando.erase(it);
        }
        free_processo();
        load_program("idle.bin");
    } else {
        terminal_println(cpuglobal, Terminal::Kernel, "MATANDO O PROCESSO!!!!!! dale excessao nele");
        kill_process();
    }
}

void tratar_interrupcao_teclado() {
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
}

void syscall_0() {
    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "=( o processo pediu pra sair ----> tadinho <---");
    std::string process_name = processo_rodando_no_momento->get_name();

    auto it = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);
    if (it != processos_rodando.end()) {
        processos_rodando.erase(it);
    }

    free_processo();
    if (!processos_rodando.empty()) {
        rerodar_idle();
    }
}

void syscall_1() {
    uint16_t str_addr = cpuglobal->get_gpr(1);
    std::string output;
    char caractere;

    while ((caractere = (char)(cpuglobal->vmem_read(str_addr++))) != '\0') {
        output += caractere;
    }

    terminal_print(cpuglobal, Arch::Terminal::Type::App, output);
}


void printa_menu(Arch::Cpu *cpu) {
    terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here.");
    terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
    terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");
    terminal_println(cpu, Arch::Terminal::Type::Command, "Welcome to the OS terminal. Type 'help' for commands.");
    terminal_print(cpu, Arch::Terminal::Type::Command, ">");
}

void boot(Arch::Cpu *cpu) {
    cpuglobal = cpu;
    printa_menu(cpu);
    memory_manager = new MemoryManager(Config::phys_mem_size_words);
    load_program("idle.bin");
}

bool load_program(const std::string& filename) {
    if (!memory_manager) {
        memory_manager = new MemoryManager(Config::phys_mem_size_words);
    }

    try {
        std::vector<uint16_t> codigo = carregar_codigo_do_arquivo(filename);

        Process* process = criar_e_configurar_processo(filename, codigo);
        if (!process) return false;

        run_process();
        gerenciar_contexto();

        processo_rodando_no_momento = process;
        process->protege(cpuglobal);
        processos_rodando.push_back(process);

        exibir_info_processo(process);

        return true;
    } catch (const Mylib::Exception& e) {
        terminal_println(cpuglobal, Terminal::Kernel, "execao brabissima!!!!!! tratandooou vamo matar ele!! ", e.what());
        kill_process();
        return false;
    }
}

void interrupt(const Arch::InterruptCode interrupt) {
    if (interrupt == InterruptCode::Keyboard) tratar_interrupcao_teclado();
    else if (interrupt == Arch::InterruptCode::CpuException) tratar_excecao();
}

void syscall() {
    if (!processo_rodando_no_momento) {
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "nenhum processo rodando");
        return;
    }

    uint16_t cod_syscall = cpuglobal->get_gpr(0);

    switch (cod_syscall) {
        case 0: {
            syscall_0();
            break;
        }

        case 1: {
            syscall_1();
            break;
        }

        case 2:
            terminal_print(cpuglobal, Arch::Terminal::Type::App, "\n");
        break;

        case 3: {
            uint16_t value = cpuglobal->get_gpr(1);
            terminal_print(cpuglobal, Arch::Terminal::Type::App, value);
            break;
        }

        default:
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "kakakka q porra de cod syscall é esse vei: ", cod_syscall);
            break;

    }
}

void shutdown() {
    if (processo_rodando_no_momento) {
        delete processo_rodando_no_momento;
        processo_rodando_no_momento = nullptr;
    }

    if (memory_manager) {
        delete memory_manager;
        memory_manager = nullptr;
    }

    for (Process* proc : processos_rodando) {
        delete proc;
    }
    processos_rodando.clear();
    cpuglobal->turn_off();
}

}