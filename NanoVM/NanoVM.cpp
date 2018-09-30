// NanoVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "NanoVM.h"

NanoVM::NanoVM(unsigned char* code, uint64_t size) {
	// Initialize cpu
	memset(&cpu, 0x00, sizeof(cpu));
	// Zero out registers
	memset(cpu.registers, 0x00, sizeof(cpu.registers));
	cpu.codeSize = (PAGE_SIZE * (1 + (size / PAGE_SIZE)));
	cpu.stackSize = PAGE_SIZE;
	// allocate whole memory, code pages, stack, +10 bytes
	// +10 bytes is for instruction fetching which might read more bytes than the instruction size
	// This avoids reading memory out side of the VM
	cpu.codeBase = (unsigned char*) malloc(cpu.stackSize + 10 + cpu.codeSize);
	// Set stack base. Stack grows up instead of down like in x86
	cpu.stackBase = cpu.codeBase + cpu.codeSize - cpu.stackSize;
	// Zero out stack + the last 10 bytes
	memset(cpu.stackBase, 0x00, sizeof(cpu.stackSize) + 10);
	// copy the bytecode to the vm
	memcpy(cpu.codeBase, code, size);
	// Set IP to the beginning of code
	cpu.registers[ip] = reinterpret_cast<uint64_t>(cpu.codeBase);
}

NanoVM::NanoVM(std::string fileName) {
	memset(&cpu, 0x00, sizeof(cpu));
	// Zero out registers
	memset(cpu.registers, 0x00, sizeof(cpu.registers));

	std::streampos size;

	std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = file.tellg();

		cpu.codeSize = (PAGE_SIZE * (1 + (size / PAGE_SIZE)));
		cpu.stackSize = PAGE_SIZE;

		// allocate whole memory, code pages, stack, +10 bytes
		// +10 bytes is for instruction fetching which might read more bytes than the instruction size
		// This avoids reading memory out side of the VM
		cpu.codeBase = (unsigned char*)malloc(cpu.stackSize + 10 + cpu.codeSize);

		file.seekg(0, std::ios::beg);
		file.read((char*)cpu.codeBase, size);
		file.close();

		// Set stack base. Stack grows up instead of down like in x86
		cpu.stackBase = cpu.codeBase + cpu.codeSize;
		// Zero out stack + the last 10 bytes
		memset(cpu.stackBase, 0x00, sizeof(cpu.stackSize) + 10);
		// Set IP to the beginning of code
		cpu.registers[ip] = 0;
	}
	else std::cout << "Unable to open file";
}
NanoVM::~NanoVM() {
	free(cpu.codeBase);
}

bool NanoVM::Run() {
	while (true) {
		Instruction inst;
		std::cout << "Ip: " << cpu.registers[ip] << std::endl;
		if (fetch(inst)) {
			std::cout << "Opcode:" << (int)inst.opcode << std::endl;
			getchar();
			if (inst.opcode == Halt) {
				std::cout << "VM halted!" << std::endl;
				return true;
			}
			if (!execute(inst)) {
				std::cout << "Failed to execute instruction!" << std::endl;
				return false;
			}
		}
		else {
			std::cout << "Invalid instruction!" << std::endl;
			return false;
		}
	}
}

bool NanoVM::execute(Instruction &inst) {
	// For now do not actually execute instruction but skip over it. Used to test that the assembler and instruction parsing work
	cpu.registers[ip] += inst.instructionSize;
	return true;
}

bool NanoVM::fetch(Instruction &inst) {
	// Read 64bit to try and minimize the required memory reading
	// This increases the performance

	// Sanity check the ip that it is within code page
	if (cpu.registers[ip] > cpu.codeSize) {
		std::cout << "IP out of bounds" << std::endl;
		return false;
	}
	// Parse the instruction
	unsigned char* rawIp = cpu.codeBase + cpu.registers[ip];
	uint64_t value = *((uint64_t*)(rawIp));
	std::printf("%016X\n", value);
	inst.opcode   =  (value & (unsigned char)OPCODE_MASK) >> (3);
	inst.dstReg   =  (value & DST_REG_MASK);
	inst.srcType  =  (value >> 8) & SRC_TYPE;
	inst.srcReg   =   (value >> 8) & SRC_REG;
	inst.srcSize  =  ((value >> 8) & SRC_SIZE) >> 5;
	inst.isDstMem =  ((value >> 8) & DST_MEM);
	inst.isSrcMem =  ((value >> 8) & SRC_MEM);
	// If source is immediate value, read it to the instruction struct
	if (inst.srcType) {
		// If the immediate value fit in the initial value. Parse it with bitshift. It is faster than reading memory again
		switch (inst.srcSize) {
		case Byte:
			inst.immediate = (uint8_t)(value >> 16);
			inst.instructionSize = 3;
			break;
		case Short:
			inst.immediate = (uint16_t)(value >> 24);
			inst.instructionSize = 4;
			break;
		case Dword:
			inst.immediate = (uint32_t)(value >> 32);
			inst.instructionSize = 6;
			break;
		case Qword:
			// In the case of qword we have to perform another read operations
			inst.immediate = *(uint64_t*)(rawIp + 2);
			inst.instructionSize = 10;
			break;
		}
	}
	else
		inst.instructionSize = 2;
	return true;

}

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		std::cout << "Usage NanoVM.exe [FILE]" << std::endl;
		return 0;
	}
	NanoVM vm(argv[1]);
	vm.Run();
	system("pause");
	return 0;
}