#include <bitset>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

// Compile-time log2 function (constexpr)
constexpr int log2_constexpr(int n, int acc = 0) {
	return (n <= 1) ? acc : log2_constexpr(n / 2, acc + 1);
}

// Global constexpr values (assumed known at compile-time)
constexpr int archSize = 32;    // 32-bit addresses
constexpr int dirEntryN = 1024; // 1024 entries per directory
constexpr int pageSize = 4096;  // 4KB page size
constexpr int sizeAddress = archSize / 4;

// Compute bit sizes
constexpr int offsetBits = log2_constexpr(pageSize);                   // log2(4096) = 12 bits
constexpr int tableIndexBits = log2_constexpr(dirEntryN);              // log2(1024) = 10 bits
constexpr int dirIndexBits = archSize - (offsetBits + tableIndexBits); // 32 - (12 + 10) = 10 bits

// Address structure using computed bit sizes
struct Address {
	uint32_t dirIndex : dirIndexBits;
	uint32_t tableIndex : tableIndexBits;
	uint32_t offset : offsetBits;

	void printSizes() {
		std::cout << "DirIndex Bits: " << dirIndexBits << " bits\n";
		std::cout << "TableIndex Bits: " << tableIndexBits << " bits\n";
		std::cout << "Offset Bits: " << offsetBits << " bits\n";
	}
};

// ============================
// FUNCTION: Print Address in Binary Table Format
// ============================
void printAddressBinary(const Address& addr) {
	std::cout << "\nBinary Representation of Address:\n";
	std::cout << "+--------------+--------------+------------+\n";
	std::cout << "| Dir Index    | Table Index  | Offset     |\n";
	std::cout << "+--------------+--------------+------------+\n";
	std::cout << "| " << std::bitset<dirIndexBits>(addr.dirIndex)
	          << " | " << std::bitset<tableIndexBits>(addr.tableIndex)
	          << " | " << std::bitset<offsetBits>(addr.offset) << " |\n";
	std::cout << "+--------------+--------------+------------+\n";
}

// ============================
// FUNCTION: Print Address in Hex Format
// ============================
void printAddressHex(const Address& addr) {
	char byteArray[sizeAddress];
	std::memcpy(byteArray, &addr, sizeAddress);

	std::cout << "\nHexadecimal Representation of Address:\n0x";
	for (int i = 0; i < sizeAddress; i++) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)byteArray[i] << " ";
	}
	std::cout << std::dec << "\n"; // Reset to decimal mode
}

int main() {
	Address addr = {2, 512, 100}; // Example Address values

	// Print actual bit sizes, NOT using sizeof()
	addr.printSizes();

	// Print Address in Binary Table Format
	printAddressBinary(addr);

	// Print Address in Hexadecimal Format
	printAddressHex(addr);

	return 0;
}
