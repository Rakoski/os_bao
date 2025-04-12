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

Arch::Cpu *cpuglobal;

void boot (Arch::Cpu *cpu)
{
	terminal_println(cpu, Arch::Terminal::Type::Command, "Type commands here");
	terminal_println(cpu, Arch::Terminal::Type::App, "Apps output here");
	terminal_println(cpu, Arch::Terminal::Type::Kernel, "Kernel output here");
	cpuglobal = cpu;
}

// ---------------------------------------


void interrupt (const Arch::InterruptCode interrupt)
{
	if (interrupt == Arch::InterruptCode::Keyboard) {
		uint16_t io = cpuglobal->read_io(Arch::IO_Port::TerminalReadTypedChar);
		terminal_print(cpuglobal, Arch::Terminal::Type::Command, (char) io);
	}
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
