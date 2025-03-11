#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

void* requestAddress(void* desired_addr, size_t size) {
	// Attempt to map at the desired address (without forcing)
	void* ptr = mmap(desired_addr, size,
	                 PROT_READ | PROT_WRITE,
	                 MAP_PRIVATE | MAP_ANONYMOUS,
	                 -1, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap failed");
		return NULL;
	}

	printf("Allocated memory at %p\n", ptr);
	return ptr;
}
// Compile-time log2 function (constexpr)
constexpr int log2_constexpr(int n, int acc = 0) {
	return (n <= 1) ? acc : log2_constexpr(n / 2, acc + 1);
}

// Global constexpr values (assumed known at compile-time)
constexpr int archSize = 32;                         // Can be adjusted dynamically
constexpr int dirEntryN = 1024;                      // 1024 entries per directory
constexpr int pageSize = 4096;                       // 4KB page size
constexpr int sizeAddress = archSize / sizeof(char); // size of 1 byte.

// Compute bit sizes
constexpr int offsetBits = log2_constexpr(pageSize);                   // log2(4096) = 12 bits
constexpr int tableIndexBits = log2_constexpr(dirEntryN);              // log2(1024) = 10 bits
constexpr int dirIndexBits = archSize - (offsetBits + tableIndexBits); // 32 - (12 + 10) = 10 bits

constexpr int pageEntryN = 1u << (offsetBits - log2_constexpr(sizeAddress));

constexpr int metadataSize = 4;
// Frame number mask (masks out metadata bits, leaves frame number bits)
constexpr uint32_t FRAME_NUMBER_MASK = ~((1u << metadataSize) - 1);

// Metadata mask (masks out everything except metadata bits)
constexpr uint32_t METADATA_MASK = (1u << metadataSize) - 1;

// ============================
// GLOBAL PAGE DIRECTORY AND PAGE TABLE ARRAYS
// ============================
/* char* pageDirectory[dirEntryN * sizeAddress]; // Maps dirIndex -> Page Table Base */
char* pageDirectory;
// We do not define pageEntryN, it's not a variable it's access using memory addressing, and we assume its already allocated when deferencing.
// using only pointer arithmetic.

// ============================
// Address Class
// ============================
class Address {
  private:
	uint32_t dirIndex;
	uint32_t tableIndex;
	uint32_t offset;

  public:
	// Constructor: Decomposes a Virtual Address
	Address(uint32_t virtualAddress) {
		offset = virtualAddress & ((1 << offsetBits) - 1);
		tableIndex = (virtualAddress >> offsetBits) & ((1 << tableIndexBits) - 1);
		dirIndex = (virtualAddress >> (offsetBits + tableIndexBits)) & ((1 << dirIndexBits) - 1);
	}

	// Method to print sizes
	void printSizes() {
		std::cout << "DirIndex Bits: " << dirIndexBits << " bits\n";
		std::cout << "TableIndex Bits: " << tableIndexBits << " bits\n";
		std::cout << "Offset Bits: " << offsetBits << " bits\n";
	}

	// Print Address in Binary Table Format
	void toBinary() const {
		std::cout << "\nBinary Representation of Address:\n";
		std::cout << "+--------------+--------------+------------+\n";
		std::cout << "| Dir Index    | Table Index  | Offset     |\n";
		std::cout << "+--------------+--------------+------------+\n";
		std::cout << "| " << std::bitset<dirIndexBits>(dirIndex)
		          << " | " << std::bitset<tableIndexBits>(tableIndex)
		          << " | " << std::bitset<offsetBits>(offset) << " |\n";
		std::cout << "+--------------+--------------+------------+\n";
	}

	// Print Address in Hex Format
	void toHex() const {
		uint32_t address = (dirIndex << (tableIndexBits + offsetBits)) |
		                   (tableIndex << offsetBits) |
		                   offset;
		std::cout << "\nHexadecimal Representation of Address:\n0x";
		std::cout << std::hex << std::setw(8) << std::setfill('0') << address << "\n";
	}

	// Getters for the values
	uint32_t getDirIndex() const { return dirIndex; }
	uint32_t getTableIndex() const { return tableIndex; }
	uint32_t getOffset() const { return offset; }
};

// ============================
// Physical Address Class
// ============================
class PhysicalAddress {
  private:
	char address[archSize]; // Physical address stored as a char array

  public:
	// Constructor: Convert uint32_t physical address to char[archSize]
	PhysicalAddress(uint32_t physAddr) {
		std::memcpy(address, &physAddr, archSize);
	}

	// Print in Binary
	void toBinary() const {
		std::cout << "\nBinary Representation of Physical Address:\n";
		std::cout << "| ";
		for (int i = 0; i < archSize; i++) {
			std::cout << std::bitset<8>(address[i]) << " ";
		}
		std::cout << "|\n";
	}

	// Print in Hex
	void toHex() const {
		std::cout << "\nHexadecimal Representation of Physical Address:\n0x";
		for (int i = 0; i < archSize; i++) {
			std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)address[i] << " ";
		}
		std::cout << std::dec << "\n"; // Reset to decimal mode
	}
};

// ============================
// FUNCTION: Virtual-to-Physical Memory Translation (Pointer Arithmetic)
// ============================
PhysicalAddress virtualToPhysical(Address vAddr) {

	// ============================
	// Step 1: Fetch Page Table Base Address from Page Directory (PDE)
	// ============================

	char* pageTablePointer = (char*)pageDirectory + vAddr.getDirIndex() * sizeAddress;

	printf("Page table pointer %p\n", pageTablePointer);
	requestAddress(pageTablePointer, sizeof(uint32_t*));
	uint32_t pageTableEntry = *(uint32_t*)pageTablePointer;

	// Extract frame number and metadata from PDE
	uint32_t pageTableFrameNumber = pageTableEntry & FRAME_NUMBER_MASK; // Extracts frame number
	uint32_t pageTableMetadata = pageTableEntry & METADATA_MASK;        // Extracts metadata bits

	// Compute the base address of the page table
	char* pageTableBase = (char*)(uintptr_t)pageTableFrameNumber + vAddr.getTableIndex() * sizeAddress;

	// ============================
	// Step 2: Fetch Physical Page Base from Page Table (PTE)
	// ============================

	// Fetch PTE entry and cast to integer
	printf("Page table pointer %p\n", pageTableBase);
	requestAddress(pageTableBase, sizeof(uint32_t*));
	uint32_t physicalFrameEntry = *(uint32_t*)pageTableBase;

	// Extract frame number and metadata from PTE (integers)
	uint32_t physicalFrameNumber = physicalFrameEntry & FRAME_NUMBER_MASK;
	uint32_t physicalMetadata = physicalFrameEntry & METADATA_MASK;

	// Compute base address of the physical frame
	char* physicalFrameBase = (char*)(uintptr_t)(physicalFrameNumber) + vAddr.getOffset() * sizeAddress;

	// ============================
	// Step 3: Compute Final Physical Address
	// ============================

	char* finalPhysicalAddress = physicalFrameBase; // Now it's a full physical address

	// Safe conversion without loss
	uintptr_t pa = reinterpret_cast<uintptr_t>(finalPhysicalAddress);
	PhysicalAddress physicalAddr(pa);

	return physicalAddr;
}

int main() {
	// Example Virtual Address
	printf("\n\n\n");
	uint32_t virtualAddress = 0x123456;
	Address addr(virtualAddress);

	pageDirectory = (char*)malloc(dirEntryN * sizeAddress);

	// Print Virtual Address Information
	addr.printSizes();
	addr.toBinary();
	addr.toHex();

	// ============================
	// SETTING UP MEMORY MAPPING
	// ============================

	// ============================
	// TRANSLATING VIRTUAL TO PHYSICAL
	// ============================
	std::cout << "\nTranslating Virtual Address to Physical:\n";
	// PhysicalAddress physicalAddr = virtualToPhysical(virtualAddress);

	PhysicalAddress physicalAddr = virtualToPhysical(virtualAddress);
	// Print Physical Address
	physicalAddr.toBinary();
	physicalAddr.toHex();

	return 0;
}
