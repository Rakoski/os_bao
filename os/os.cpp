#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"

#include "memory_manager.h"
#include "os-lib.h"


namespace OS {

// ---------------------------------------

void boot (Arch::Cpu *cpu)
{
	terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here");
	terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
	terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");
	terminal_print(cpu, Arch::Terminal::Type::Command, "aloooo");
	memory_manager = new MemoryManager(UINT16_MAX);
}

// ---------------------------------------

void interrupt (const Arch::InterruptCode interrupt)
{

}

// ---------------------------------------

void syscall ()
{

}

// ---------------------------------------

void shutdown() {
	if (memory_manager) {
		delete memory_manager;
		free(memory_manager);
		memory_manager = nullptr;
	}
}

} // end namespace OS
