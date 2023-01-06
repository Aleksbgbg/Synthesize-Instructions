#include <sys/mman.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using u8 = uint8_t;
using u32 = uint32_t;
using i32 = int32_t;

// Each enum value represents the 3-bit sequence used when addressing the register
enum class Register : u8 {
  Eax = 0b000,
  Ebx = 0b011,
  Ecx = 0b001,
  Edx = 0b010,
};

// Each enum value represents the 2-bit sequence used when selecting this addressing mode
enum class Mode : u8 {
  Address = 0b00,
  AddressPlus8bitDisplacement = 0b01,
  AddressPlus32bitDisplacement = 0b10,
  Register = 0b11,
};

template <typename TEnum>
constexpr auto EnumToByte(TEnum enum_value) {
  return static_cast<u8>(enum_value);
}

constexpr u8 MakeModRmByte(Mode mode, u8 reg_or_opcode, u8 reg_or_memory) {
  return (EnumToByte(mode) << 6) | (reg_or_opcode << 3) | reg_or_memory;
}

constexpr u8 ExtractByte(i32 value, u32 byte_index) {
  return (value >> (byte_index * 8)) & 0xFF;
}

namespace emit_x86_instruction {

// Inserts bytes for IA-32 instruction "MOV r/m32, imm32", opcode format "C7 /0 id"
void Mov(std::vector<u8>& instruction_stream, i32 value, Register reg) {
  // Opcode (1 byte)
  instruction_stream.push_back(0xC7);
  // MOD R/M byte
  instruction_stream.push_back(MakeModRmByte(Mode::Register, 0b000, EnumToByte(reg)));
  // Immediate data element (4 bytes)
  instruction_stream.push_back(ExtractByte(value, 0));
  instruction_stream.push_back(ExtractByte(value, 1));
  instruction_stream.push_back(ExtractByte(value, 2));
  instruction_stream.push_back(ExtractByte(value, 3));
}

// Inserts bytes for IA-32 instruction "INT imm8", opcode format "CD ib"
void Int(std::vector<u8>& instruction_stream, u8 interrupt_vector) {
  // Opcode (1 byte)
  instruction_stream.push_back(0xCD);
  // Immediate data element (1 byte)
  instruction_stream.push_back(interrupt_vector);
}

}  // namespace emit_x86_instruction

int main() {
  constexpr const char* kString = "Hello, world!\n";

  std::vector<u8> instruction_stream;

  emit_x86_instruction::Mov(instruction_stream, 4, Register::Eax);
  emit_x86_instruction::Mov(instruction_stream, 1, Register::Ebx);
  emit_x86_instruction::Mov(instruction_stream, reinterpret_cast<i32>(kString), Register::Ecx);
  emit_x86_instruction::Mov(instruction_stream, static_cast<i32>(std::strlen(kString)), Register::Edx);
  emit_x86_instruction::Int(instruction_stream, 0x80);

  emit_x86_instruction::Mov(instruction_stream, 1, Register::Eax);
  emit_x86_instruction::Mov(instruction_stream, 0, Register::Ebx);
  emit_x86_instruction::Int(instruction_stream, 0x80);

  u8* memory = static_cast<u8*>(
      mmap(nullptr, instruction_stream.size(), PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  if (memory == MAP_FAILED) {
    std::cout << "Failed to map: " << strerror(errno) << " (" << errno << ")" << std::endl;
    return -1;
  }

  std::memcpy(memory, instruction_stream.data(), instruction_stream.size());

  using Instructions = void (*)();
  Instructions execute_instructions = reinterpret_cast<Instructions>(memory);
  execute_instructions();

  return -500;  // Never executed, instruction stream returns exit code 0
}

// Original Assembly Program
// x86 assembly, AT&T syntax, Linux kernel, Intel 32-bit architecture (i386), little-endian byte order
//   .section .data
//     hello:
//       .ascii "Hello, world!\n"
//
//   .section .text
//   .globl _start
//
//   _start:
//     mov $4, %eax      # write to file
//     mov $1, %ebx      # file descriptor 1 (STDOUT)
//     mov $hello, %ecx  # string address
//     mov $14, %edx     # length
//     int $0x80         # invoke kernel handler for user-mode system calls via interrupt descriptor table entry 128
//
//     mov $1, %eax      # exit
//     mov $0, %ebx      # exit code 0 (success)
//     int $0x80         # invoke kernel handler for user-mode system calls via interrupt descriptor table entry 128
