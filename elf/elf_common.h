#ifndef STUDENTELFREADER_LIBRARY_H
#define STUDENTELFREADER_LIBRARY_H

#include "elf.h"

#define DEBUG_GET_ELF_DATA 0
#define DEBUG_DESTROY 0
#define NAME_BUFFER 100

/**
 * Represents a section of the ELF file.
 * @param sectionHeader Header for this section
 * @param sectionName The name of the section copied from the string table.
 */
typedef struct ElfSection {
    Elf64_Shdr sectionHeader;
    char* sectionName;
} ElfSection;

/**
 * Respresents a symbol from the ELF file
 * @param symbol The symbol located in the ELF file
 * @param name The name of the symbol copied from the string table.
 */
typedef struct ElfSymbol {
    Elf64_Sym symbol;
    char* name;
} ElfSymbol;

/**
 * List of ELF symbols
 * @param list A pointer of sequential ElfSymbol objects.  One symbol immediately after one another
 * @param size Number of ELFSymbols in this list
 */
typedef struct ElfSymbolList {
    ElfSymbol* list;
    int size;
} ElfSymbolList;

/**
 * All of the ELF data in a representation that we can use for testing by the ELFTester project
 * @param elfHeader The ELF Header copied out of the file.
 * @param sections A pointer containing all of the elf sections, the number of sections is specifed in the ELF header
 * @param programHeader The program header copied out of the file
 * @param dynSymbols All of the dynamic symbols from the .dynsym section.
 * @param otherSymbols All of the symbols from the .symtab section.
 */
typedef struct ElfData {
    Elf64_Ehdr elfHeader;
    ElfSection* sections;
    Elf64_Phdr* programHeader;
    ElfSymbolList dynSymbols;
    ElfSymbolList otherSymbols;
} ElfData;

/**
 * Init ElfData takes an ElfData struct and sets all values to 0/NULL.
 *
 * @param ElfData ret elf data struct to be initialized.
 */
void init_ElfData(ElfData* ret);

/**
 * Description: This function takes pointers to the start of a file map, an elf header,
 * and a pointer to the program header of an ElfData structure you are building and copies
 * the data from the file map to the program header of your EldData struct using the header
 * that you have already copied.
 *
 * Signature: buildProgramHeader :: char*, Elf64_Phdr*, Elf64_Ehdr* -> void
 *
 * @param char* data: location of your mmap
 * @param ElfData* ret: pointer to ElfData structure you are building.
 */
void buildELFProgramHeader(char* data, ElfData* ret);

/**
 * Description: This function takes pointers to the start of a file map, an elf header,
 * and a pointer to the sections of an ElfData structure you are building and copies
 * the data from the file map to the section of your EldData struct using the header
 * that you have already copied.
 *
 * Signature: buildSections :: char*, ElfData* -> void
 *
 * @param char* data: location of your mmap
 * @param ElfData* ret: pointer to ElfData structure you are building.
 */
void buildELFSections(char* data, ElfData* ret);

/**
 * Description: This function builds a Symbol list by taking the pointer to the mmap
 * and a pointer to the ElfData struct we are building and using the section heaeers
 * to find the appropriate data.
 *
 * @param data pointer to the beginning of the mmap.
 * @param int section_header the section index of the Symbol Table
 * @param ret ElfData struct we are building to return to the stack
 */
void build_ElfSymbolLists(char* data, ElfData* ret);

/**
 * This function will grab elf data from the executable given by mapping the file into memory. It should handle the
 * error if the file deos not exist.  A file that is not of ELF format should also fail. The function should fill in all
 * data and put in NULL for pointers that don't have any data.
 * @param executable The name of the executable to read.
 * @return All data from file that fills in the ELF data structure. A zeroed out data structure.
 */
ElfData getELFData(const char* executable);

/**
 * Destroys all memory created during the creation of the ElfData structure.
 * @param elfData The object to eliminate created memory.
 */
void destroyELFData(ElfData elfData);
#endif
