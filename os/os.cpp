#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cmath>

// perguntar p eduardo qual assembly tem syscall

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
Paging* paginacao = nullptr;
ProcessManager* gerenciador_processos = nullptr;
Process* idle = nullptr;

void kill_process();
void rerodar_idle();
bool load_program(const std::string& filename);
void tratar_interrupcao_teclado();
void tratar_excecao();

void tratar_timer() {
    if (!gerenciador_processos) return;

    gerenciador_processos->acordar_processos_dormindo();

    processo_rodando_no_momento = gerenciador_processos->configurar_proximo_processo(processo_rodando_no_momento, idle);
}

void aloca_pagina_fisica(Process* novo_processo) {
    uint16_t paginas_necessarias = std::ceil((double)novo_processo->codigo_processo.size() / Config::page_size);

    for (uint16_t i = 0; i < paginas_necessarias; i++) {
        uint16_t nova_pagina = paging->aloca_pagina_fisica_livre();
        uint16_t endereco_fisico = nova_pagina * Config::page_size;

        for (uint16_t i = 0; i < Config::page_size; i++) cpuglobal->pmem_write(endereco_fisico + i, 0);

        terminal_println(cpuglobal, Terminal::Kernel, "pagina física alocada DE COMEÇO no endereco: " + endereco_fisico);
    }
}

Process* criar_e_configurar_processo(const std::string& filename) {
    std::vector<uint16_t> codigo = Lib::load_from_disk_to_16bit_buffer(filename);

    uint16_t pid = ++proximo_pid;

    Process* novo_processo = new Process(pid, filename, codigo);

    Arch::Cpu::PageTable* nova_tabela = paginacao->cria_tabela_paginas();
    novo_processo->set_tabela_paginas(nova_tabela);
    novo_processo->set_estado(ProcessState::ready);

    uint16_t tamanho_codigo = codigo.size();
    uint16_t paginas_necessarias = std::ceil((double)tamanho_codigo / Config::page_size);

    bool sucesso = paginacao->mapeia_e_carrega_codigo(nova_tabela, 0, paginas_necessarias, codigo, cpuglobal);

    if (!sucesso) {
        delete novo_processo;
        return nullptr;
    }

    novo_processo->set_codigo_processo(codigo);
    novo_processo->set_pc(1);
    novo_processo->set_tempo_criacao(gerenciador_processos->get_tempo_sistema());
    Utils::setando_novos_regs_pro_processo(novo_processo);

    return novo_processo;
}

void rerodar_idle() {
    const auto& procesoss_novos = gerenciador_processos->get_processos_rodando_novo();
    Process* idle = nullptr;
    for (Process* proc : procesoss_novos) {
        if (proc->get_name() == "idle.bin") {
            idle = proc;
            break;
        }
    }

    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        cpuglobal->set_page_table(idle->get_tabela_paginas());
        processo_rodando_no_momento->restaurar_contexto(cpuglobal);
        terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "voltado pro processo: ",
                        processo_rodando_no_momento->get_name());
    }
}

void free_processo() {
    if (processo_rodando_no_momento->get_tabela_paginas())
        paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());
    delete processo_rodando_no_momento;

    const auto& lista = gerenciador_processos->get_processos_rodando_novo();
    Process* idle = nullptr;
    for (Process* proc : lista) {
        if (proc->get_name() == "idle.bin") {
            idle = proc;
            break;
        }
    }

    if (idle) {
        processo_rodando_no_momento = idle;
        processo_rodando_no_momento->set_estado(ProcessState::running);
        cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
        processo_rodando_no_momento->restaurar_contexto(cpuglobal);
    } else {
        processo_rodando_no_momento = nullptr;
    }
}

void kill_process_pid(std::string pid_string) {
    uint32_t pid = std::stoi(pid_string);

    if (idle && idle->get_pid() == pid) {
        terminal_println(cpuglobal, Terminal::Kernel, "Não pode matar o processo idle!");
        return;
    }

    if (processo_rodando_no_momento && processo_rodando_no_momento->get_pid() == pid) {
        terminal_println(cpuglobal, Terminal::Kernel, "matando processo con pid ", pid, " e nome ", processo_rodando_no_momento->get_name());

        gerenciador_processos->remover_processo(processo_rodando_no_momento);

        if (processo_rodando_no_momento->get_tabela_paginas()) paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());

        delete processo_rodando_no_momento;
        processo_rodando_no_momento = nullptr;

        tratar_timer();
    } else {
        Process* processo_q_quero = gerenciador_processos->encontrar_por_pid(pid);

        if (!processo_q_quero) {
            terminal_println(cpuglobal, Terminal::Kernel, "Processo con pid ", pid, " não encontrado");
            return;
        }

        terminal_println(cpuglobal, Terminal::Kernel, "2 matando processo con pid ", pid, " e nome ", processo_q_quero->get_name());

        gerenciador_processos->remover_processo(processo_q_quero);

        if (processo_q_quero->get_tabela_paginas()) paginacao->libera_tabela_paginas(processo_q_quero->get_tabela_paginas());

        delete processo_q_quero;

        tratar_timer();
    }
}

void kill_process() {
    if (!processo_rodando_no_momento) {
        terminal_println(cpuglobal, Terminal::Kernel, "Nenhum processo rodando");
        return;
    }

    if (processo_rodando_no_momento == idle) {
        terminal_println(cpuglobal, Terminal::Kernel, "Não pode matar o processo idle!");
        return;
    }

    terminal_println(cpuglobal, Terminal::Kernel, "matando processo ", processo_rodando_no_momento->get_pid());
    kill_process_pid(std::to_string(processo_rodando_no_momento->get_pid()));
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
        if (args.size() < 2) kill_process();
        else kill_process_pid(args[1]);
    } else if (comando == "list" || comando == "ps") {
        gerenciador_processos->lista_processos(cpuglobal, processo_rodando_no_momento);
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
        if (resultado == ResultadoAlocarPagina::deu_bom) return;

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
            gerenciador_processos->remover_processo(processo_rodando_no_momento);
            if (processo_rodando_no_momento->get_tabela_paginas()) paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());
            delete processo_rodando_no_momento;
            processo_rodando_no_momento = nullptr;
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

    if (!processo_rodando_no_momento) return;

    gerenciador_processos->remover_processo(processo_rodando_no_momento);

    if (processo_rodando_no_momento->get_tabela_paginas()) {
        paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());
    }
    delete processo_rodando_no_momento;
    processo_rodando_no_momento = nullptr;

    if (!gerenciador_processos->emptar_processos_novos()) {
        Process* next = gerenciador_processos->pop_front_processos_novos();
        if (next) {
            processo_rodando_no_momento = next;
            next->set_estado(ProcessState::running);
            cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
            cpuglobal->set_page_table(next->get_tabela_paginas());
            next->restaurar_contexto(cpuglobal);
            terminal_println(cpuglobal, Terminal::Kernel, "Processo terminou, trocando para ", next->get_pid());
        }
    } else {
        terminal_println(cpuglobal, Terminal::Kernel, "Processo terminou, voltando para idle");
        if (idle) {
            processo_rodando_no_momento = idle;
            idle->set_estado(ProcessState::running);
            cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
            cpuglobal->set_page_table(idle->get_tabela_paginas());
            idle->restaurar_contexto(cpuglobal);
        } else {
            terminal_println(cpuglobal, Terminal::Kernel, "ERRO: idle não existe!");
            cpuglobal->turn_off();
        }
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

void syscall_6_dormir_processo() {
    uint16_t segundos = cpuglobal->get_gpr(1);

    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "processo ", processo_rodando_no_momento->get_pid(), " vai dormir por ", segundos, " segundos");

    processo_rodando_no_momento->salvar_contexto(cpuglobal);

    gerenciador_processos->dormir_processo(processo_rodando_no_momento, segundos);

    processo_rodando_no_momento = gerenciador_processos->configurar_proximo_processo(nullptr, idle);
}

void syscall_7_retornar_tempo_iniciacao() {
    uint16_t tempo_sistema = gerenciador_processos->get_tempo_sistema();
    uint16_t tempo_desde_criacao = tempo_sistema - processo_rodando_no_momento->get_tempo_criacao();

    cpuglobal->set_gpr(1, tempo_desde_criacao);

    terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "processo ", processo_rodando_no_momento->get_pid(), " tempo desde criação: ", tempo_desde_criacao, " segundos");
}


void boot(Arch::Cpu *cpu) {
    cpuglobal = cpu;

    paginacao = new Paging();

    cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);

    gerenciador_processos = new ProcessManager(cpuglobal);
    gerenciador_processos->set_comeco(cpuglobal->read_io(IO_Port::TimerGetTimeSeconds));
    cpuglobal->write_io(IO_Port::TimerInterruptCycles, gerenciador_processos->get_ciclos_timer());
    printa_menu(cpuglobal);
    memory_manager = new MemoryManager(Config::phys_mem_size_words);

    idle = criar_e_configurar_processo("idle.bin");
    if (!idle) {
        terminal_println(cpuglobal, Terminal::Kernel, "ERRO: não conseguiu carregar idle!");
        cpuglobal->turn_off();
        return;
    }

    processo_rodando_no_momento = idle;
    idle->set_estado(ProcessState::running);

    cpuglobal->set_page_table(idle->get_tabela_paginas());
    cpuglobal->set_pc(idle->get_pc());

    for (uint16_t i = 0; i < Config::nregs; i++) cpuglobal->set_gpr(i, idle->get_regs(i));

    terminal_println(cpuglobal, Terminal::Kernel, "processo idle iniciado com PID ", idle->get_pid());
}

bool load_program(const std::string& filename) {
    if (!memory_manager) {
        memory_manager = new MemoryManager(Config::phys_mem_size_words);
    }

    try {
        Process* processo = criar_e_configurar_processo(filename);
        if (!processo) return false;

        gerenciador_processos->push_back_processos_novos(processo);

        if (processo_rodando_no_momento == idle) {
            processo_rodando_no_momento = gerenciador_processos->pop_front_processos_novos();
            processo->set_estado(ProcessState::running);
            cpuglobal->set_vmem_mode(Arch::Cpu::VmemMode::Paging);
            cpuglobal->set_page_table(processo->get_tabela_paginas());
            cpuglobal->set_pc(processo->get_pc());
            for (uint16_t i = 0; i < Config::nregs; i++) cpuglobal->set_gpr(i, processo->get_regs(i));
            terminal_println(cpuglobal, Terminal::Kernel, "trocando para processo: ", processo->get_name());
        } else terminal_println(cpuglobal, Terminal::Kernel, "processo: ", processo->get_name(), " add a fila ");
        Utils::exibir_info_processo(processo);
        return true;

    } catch (const Mylib::Exception& e) {
        terminal_println(cpuglobal, Terminal::Kernel, "execao brabissima!!!!!! tratandooou vamo matar ele!! ", e.what());
        kill_process();
        return false;
    }
}

void interrupt(const Arch::InterruptCode interrupt) {
    if (interrupt == InterruptCode::Keyboard) {
        tratar_interrupcao_teclado();
    } else if (interrupt == InterruptCode::CpuException) {
        tratar_excecao();
    } else if (interrupt == Arch::InterruptCode::Timer) {
        tratar_timer();
    }
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

        case 6: {
            syscall_6_dormir_processo();
            break;
        }

        case 7: {
            syscall_7_retornar_tempo_iniciacao();
            break;
        }

        default:
            terminal_println(cpuglobal, Arch::Terminal::Type::Kernel, "kakakka q porra de cod syscall é esse vei: ", cod_syscall);
            break;

    }
}


void shutdown() {
    if (gerenciador_processos) {
        delete gerenciador_processos;
        gerenciador_processos = nullptr;
    }

    if (idle) {
        if (idle->get_tabela_paginas()) {
            paginacao->libera_tabela_paginas(idle->get_tabela_paginas());
        }
        delete idle;
        idle = nullptr;
    }

    if (processo_rodando_no_momento && processo_rodando_no_momento != idle) {
        if (processo_rodando_no_momento->get_tabela_paginas()) {
            paginacao->libera_tabela_paginas(processo_rodando_no_momento->get_tabela_paginas());
        }
        delete processo_rodando_no_momento;
    }
    processo_rodando_no_momento = nullptr;

    if (memory_manager) {
        delete memory_manager;
        memory_manager = nullptr;
    }

    if (paginacao) {
        delete paginacao;
        paginacao = nullptr;
    }

    cpuglobal->turn_off();
}

}