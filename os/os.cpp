#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"
#include "memory_manager.h"
#include "os-lib.h"
#include "paging.h"
#include "process.h"
#include "process_manager.h"
#include "utils.h"

namespace OS {

Arch::Cpu* cpuglobal = nullptr;
std::string command_buffer;
Process* processo_rodando_no_momento = nullptr;
uint16_t proximo_pid = 1;
std::vector<Process*> processos_rodando;
Paging* paginacao = nullptr;
ProcessManager* gerenciador_processos = nullptr;

void kill_process();
void rerodar_idle();
bool load_program(const std::string& filename);
void tratar_interrupcao_teclado();
void tratar_excecao();

void aloca_pagina_fisica(Process* novo_processo) {
    uint16_t paginas_necessarias = std::ceil((double)novo_processo->codigo_processo.size() / Config::page_size);

    for (uint16_t i = 0; i < paginas_necessarias; i++) {
        uint16_t nova_pagina = paging->aloca_pagina_fisica_livre();
        uint16_t endereco_fisico = nova_pagina * Config::page_size;

        for (uint16_t i = 0; i < Config::page_size; i++) {
            terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada com valor 0: " + endereco_fisico + 1);
            cpuglobal->pmem_write(endereco_fisico + i, 0);
        }

        terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada DE COMEÇO no endereco: " + endereco_fisico);
    }
}

Process* criar_e_configurar_processo(const std::string& filename) {
    std::vector<uint16_t> codigo = Lib::load_from_disk_to_16bit_buffer(filename);

    uint16_t pid = proximo_pid++;

    Process* novo_processo = new Process(pid, filename, codigo);

    Arch::Cpu::PageTable* nova_tabela = paginacao->cria_tabela_paginas();
    novo_processo->set_tabela_paginas(nova_tabela);

    uint16_t tamanho_codigo = codigo.size();
    uint16_t paginas_necessarias = std::ceil((double)tamanho_codigo / Config::page_size);

    bool sucesso = paginacao->mapeia_paginas_pra_um_processo(nova_tabela, 0, paginas_necessarias, true, true, true);

    if (!sucesso) {
        delete novo_processo;
        return nullptr;
    }

    novo_processo->set_codigo_processo(codigo);
    Utils::setando_novos_regs_pro_processo(novo_processo);

    return novo_processo;
}

void run_process() {
    if (!processo_rodando_no_momento) return;

    processo_rodando_no_momento->salvar_contexto(cpuglobal);
    processo_rodando_no_momento->set_estado(ProcessState::running);

    auto processo = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);

    if (processo == processos_rodando.end()) processos_rodando.push_back(processo_rodando_no_momento);
}


void rerodar_idle() {
    Process* idle = Utils::find_idle();

    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);

        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        cpuglobal->set_page_table(idle->get_tabela_paginas());

        processo_rodando_no_momento->restaurar_contexto(cpuglobal);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "vontado pro processo: ", processo_rodando_no_momento->get_name());
    }
}

void free_processo() {
    if (processo_rodando_no_momento->get_tabela_paginas()) {
        paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());
    }
    delete processo_rodando_no_momento;

    Process* idle = Utils::find_idle();
    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        processo_rodando_no_momento->restaurar_contexto(cpuglobal);
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

    rerodar_idle();
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
        Utils::printar_help();
    }
    else {
        terminal_println(cpuglobal, Arch::Terminal::Type::Command, "Digite 'help'    para ver os comandos disponíveis");
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
    terminal_println(cpuglobal, Terminal::Kernel, "excecao das braba: ", exception.type, " no endereço: ", exception.vaddr);

    if (!processo_rodando_no_momento) return;

    if (exception.type == Arch::Cpu::CpuException::Type::VmemPageFault) {
        ResultadoAlocarPagina resultado = paginacao->page_fault(exception.vaddr, exception.type, cpuglobal, processo_rodando_no_momento);

        if (resultado == ResultadoAlocarPagina::deu_bom) {
            return;
        }
        if (processo_rodando_no_momento->get_name() != "idle.bin") {
            terminal_println(cpuglobal, Terminal::Kernel, "matando processo:  ",  processo_rodando_no_momento->get_name());
            kill_process();
        }
        return;
    }

    if (exception.type == Arch::Cpu::CpuException::Type::VmemGPFnotReadable ||
        exception.type == Arch::Cpu::CpuException::Type::VmemGPFnotWritable ||
        exception.type == Arch::Cpu::CpuException::Type::VmemGPFnotExecutable) {
        terminal_println(cpuglobal, Terminal::Kernel, "MATANDO EL PROCESSINHO");
        kill_process();
        return;
    }

    if (exception.type == Arch::Cpu::CpuException::Type::GPFinvalidInstruction) {
        if (processo_rodando_no_momento->get_name() == "idle.bin") {
            terminal_println(cpuglobal, Terminal::Kernel, "GPFinvalidInstruction, RELODANDO IDLE...");
            auto it = std::find(processos_rodando.begin(), processos_rodando.end(), processo_rodando_no_momento);
            if (it != processos_rodando.end()) {
                processos_rodando.erase(it);
            }
            free_processo();
            load_program("idle.bin");
        } else {
            terminal_println(cpuglobal, Terminal::Kernel, "matando el processo por conta da GPFinvalidInstruction");
            kill_process();
        }
        return;
    }

    terminal_println(cpuglobal, Terminal::Kernel, "Unhandled exception, killing process");
    kill_process();
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

uint16_t vmem_to_phys(Arch::Cpu::PageTable& tabela, const uint16_t vaddr) {
    uint16_t pagina_virtual = vaddr >> Config::page_size_bits;

    Arch::Cpu::PageTableEntry& pte = tabela[pagina_virtual];

    if (pte[Arch::Cpu::PteField::Present] == 0) {
        return 0xFFFF;
    }

    uint16_t physical_frame = pte[Arch::Cpu::PteField::PhyFrameID];
    uint16_t paddr = Mylib::set_bits(
        vaddr,
        Config::page_size_bits,
        Config::page_frame_id_bits,
        physical_frame
    );

    return paddr;
}


void syscall_0_fechar_processo() {
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

    void syscall_1_imprimir_string() {
    uint16_t str_addr = cpuglobal->get_gpr(1);
    std::string output;
    char caractere;

    Arch::Cpu::PageTable* tabela = processo_rodando_no_momento->get_tabela_paginas();

    while (true) {
        uint16_t paddr = vmem_to_phys(*tabela, str_addr++);
        if (paddr == 0xFFFF) break;

        caractere = (char)(cpuglobal->pmem_read(paddr));
        if (caractere == '\0') break;
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


    void syscall_4_alocar_memoria() {
    if (!processo_rodando_no_momento) {
        cpuglobal->set_gpr(1, 0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "falha ao alocar memória dinamicamente pois nenhum processo rodando");
        return;
    }

    uint16_t tamanho_solicitado = cpuglobal->get_gpr(1);

    if (tamanho_solicitado == 0) {
        cpuglobal->set_gpr(1, 0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "falha ao alocar memória dinamicamente pois  tamanho inválido");
        return;
    }

    uint16_t endereco_virtual = paginacao->aloca_dinamicamente(cpuglobal, tamanho_solicitado, processo_rodando_no_momento);

    if (endereco_virtual == -1) {
        // se der errado - gpr 1 com resultado 0
        cpuglobal->set_gpr(1, 0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "falha ao alocar memória dinamicamente");
    } else {
        // se der bom:
        // 1 - coloca r1 com resultado 1 (sucesso)
        // 2 - coloca r2 com endereço da memória virtual
        cpuglobal->set_gpr(1, 1);
        cpuglobal->set_gpr(2, endereco_virtual);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "memória alocadaaaaa: ", endereco_virtual);
    }
}


void syscall_5_desalocar_memoria() {
    if (!processo_rodando_no_momento) {
        cpuglobal->set_gpr(1, 0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "falha ao desalocar memória possi nenhum processo rodando");
        return;
    }

    uint16_t endereco_para_desalocar = cpuglobal->get_gpr(1);

    bool deu_bom = paginacao->desaloca_a_partir_de_tal_endereco(cpuglobal, endereco_para_desalocar, processo_rodando_no_momento);

    if (deu_bom) {
        //gpr 1 seta 1
        cpuglobal->set_gpr(1, 1);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "memória desalocada com sucesso do endereço: ", endereco_para_desalocar);
    } else {
        //seta gpr 0 no r1
        cpuglobal->set_gpr(1, 0);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "falha ao desalocar memória do endereço: ", endereco_para_desalocar);
    }
}


void boot(Arch::Cpu *cpu) {
    cpuglobal = cpu;
    printa_menu(cpu);
    memory_manager = new MemoryManager(Config::phys_mem_size_words);
    paginacao = new Paging();
    gerenciador_processos = new ProcessManager(cpu);
    load_program("idle.bin");
}

bool load_program(const std::string& filename) {
    if (!memory_manager) {
        memory_manager = new MemoryManager(Config::phys_mem_size_words);
    }

    try {
        Process* processo = criar_e_configurar_processo(filename);
        if (!processo) return false;

        run_process();

        processo_rodando_no_momento = processo;
        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        cpuglobal->set_page_table(processo->get_tabela_paginas());

        processos_rodando.push_back(processo);

        processo->set_pc(1);
        cpuglobal->set_pc(1);
        Utils::exibir_info_processo(processo);

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
            syscall_0_fechar_processo();
            break;
        }

        case 1: {
            syscall_1_imprimir_string();
            break;
        }

        case 2:
            terminal_print(cpuglobal, Arch::Terminal::Type::App, "\n");
        break;

        case 3: {
            uint16_t gpr = cpuglobal->get_gpr(1);
            terminal_print(cpuglobal, Arch::Terminal::Type::App, gpr);
            break;
        }

        case 4: {
            syscall_4_alocar_memoria();
            break;
        }

        case 5: {
            syscall_5_desalocar_memoria();
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

    if (gerenciador_processos) {
        delete gerenciador_processos;
        gerenciador_processos = nullptr;
    }

    if (paginacao) {
        delete paginacao;
        paginacao = nullptr;
    }

    processos_rodando.clear();
    cpuglobal->turn_off();
}

}