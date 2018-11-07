#include "elf_common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <elf.h>

void init_ElfData(ElfData* ret){
    memset(&ret->elfHeader, 0, sizeof(Elf64_Ehdr));
    ret->sections = NULL;
    ret->programHeader = NULL;
    ret->dynSymbols.list = NULL;
    ret->dynSymbols.size = 0;
    ret->otherSymbols.list = NULL;
    ret->otherSymbols.size = 0;

    if(DEBUG_GET_ELF_DATA){ printf("ElfData zeroed out.\n"); }
}

void buildELFProgramHeader(char* data, ElfData* ret){
    if(DEBUG_GET_ELF_DATA){ printf("Prog Offset: %ld Prog Num: %d Prog Size: %d \n", ret->elfHeader.e_phoff,
                ret->elfHeader.e_phnum, ret->elfHeader.e_phentsize); }

    ret->programHeader = calloc(ret->elfHeader.e_phnum, sizeof(Elf64_Phdr));

    char* pointer_offset = data + ret->elfHeader.e_phoff;

    for (int i = 0; i < ret->elfHeader.e_phnum; ++i) {
        memcpy(&ret->programHeader[i], pointer_offset, sizeof(Elf64_Phdr));
        pointer_offset += sizeof(Elf64_Phdr);
    }
}

void buildELFSections(char* data, ElfData* ret){

    if(DEBUG_GET_ELF_DATA){ printf("Section Offset: %ld Section Num: %d Section Size: %d \n",
                ret->elfHeader.e_shoff, ret->elfHeader.e_shnum, ret->elfHeader.e_shentsize); }

    char* pointer_offset = data + ret->elfHeader.e_shoff;

    ret->sections = calloc(ret->elfHeader.e_shnum, sizeof(Elf64_Shdr) + NAME_BUFFER);

    memcpy(&ret->sections[0].sectionHeader, pointer_offset, sizeof(Elf64_Shdr));
    pointer_offset += ret->elfHeader.e_shentsize;

    if(DEBUG_GET_ELF_DATA){ printf("The section header table index: %d, loreserve: %d\n",
            ret->elfHeader.e_shstrndx, SHN_LORESERVE); }

    int sh_name_string_index;
    if(ret->elfHeader.e_shstrndx >= SHN_LORESERVE){
        sh_name_string_index = ret->sections[0].sectionHeader.sh_link;
    } else {
        sh_name_string_index = ret->elfHeader.e_shstrndx;
    }

    for (int i = 1; i < ret->elfHeader.e_shnum; ++i) {
        memcpy(&ret->sections[i].sectionHeader, pointer_offset, sizeof(Elf64_Shdr));
        pointer_offset += ret->elfHeader.e_shentsize;
    }

    for (int j = 0; j < ret->elfHeader.e_shnum; ++j) {
        ret->sections[j].sectionName = strdup(data
                + ret->sections[sh_name_string_index].sectionHeader.sh_offset
                + ret->sections[j].sectionHeader.sh_name);

        if(DEBUG_GET_ELF_DATA){ printf("Section name: %s\n", ret->sections[j].sectionName); }
    }
}

void build_ElfSymbolLists(char* data, ElfData* ret){

    if(DEBUG_GET_ELF_DATA){ printf("Start build symbol lists \n"); }

    int sym_table_index = 0;
    int sym_table_name_index = 0;
    int dyn_sym_table_index = 0;
    int dyn_sym_table_name_index = 0;
    char* ptr;

    for (int i = 0; i < ret->elfHeader.e_shnum; i++) {
        if(strcmp(ret->sections[i].sectionName, ".dynsym") == 0){ dyn_sym_table_index = i; }
        if(strcmp(ret->sections[i].sectionName, ".dynstr") == 0){ dyn_sym_table_name_index = i; }
        if(strcmp(ret->sections[i].sectionName, ".symtab") == 0){ sym_table_index = i; }
        if(strcmp(ret->sections[i].sectionName, ".strtab") == 0){ sym_table_name_index = i; }

        if(DEBUG_GET_ELF_DATA){ printf("Section Header name:[%d]: %s\n", i, ret->sections[i].sectionName); }
    }

    if(sym_table_index > 0){
        if(DEBUG_GET_ELF_DATA) { printf("Sym Table Index: %d\n", sym_table_index); }

        ret->otherSymbols.size = ret->sections[sym_table_index].sectionHeader.sh_size
                / sizeof(Elf64_Sym);
        ret->otherSymbols.list = calloc(ret->otherSymbols.size, sizeof(Elf64_Sym) + NAME_BUFFER);

        if(DEBUG_GET_ELF_DATA){ printf("Number of Symbols: %d Section Size: %ld\n",
                ret->otherSymbols.size,
                ret->sections[sym_table_index].sectionHeader.sh_size); }

        ptr = data + ret->sections[sym_table_index].sectionHeader.sh_offset;

        for (int i = 0; i < ret->otherSymbols.size; ++i) {
            memcpy(&ret->otherSymbols.list[i].symbol, ptr, sizeof(Elf64_Sym));
            ptr += sizeof(Elf64_Sym);
            ret->otherSymbols.list[i].name = strdup(data
                    + ret->sections[sym_table_name_index].sectionHeader.sh_offset
                    + ret->otherSymbols.list[i].symbol.st_name);

            if(DEBUG_GET_ELF_DATA){ printf("Symbol Name: %s\n", ret->otherSymbols.list[i].name); }
        }

    } else {
        if(DEBUG_GET_ELF_DATA){ printf("No Symbol Table in this file\n"); }
    }

    if(dyn_sym_table_index > 0){

        ret->dynSymbols.size = ret->sections[dyn_sym_table_index].sectionHeader.sh_size
                / sizeof(Elf64_Sym);
        ret->dynSymbols.list = calloc(ret->dynSymbols.size, sizeof(Elf64_Sym)+NAME_BUFFER);

        if(DEBUG_GET_ELF_DATA){ printf("Number of Dynamic Symbols: %d Section Size: %ld\n",
                ret->dynSymbols.size,
                ret->sections[dyn_sym_table_index].sectionHeader.sh_size); }

        ptr = data + ret->sections[dyn_sym_table_index].sectionHeader.sh_offset;

        for (int i = 0; i < ret->dynSymbols.size; ++i) {
            memcpy(&ret->dynSymbols.list[i].symbol, ptr, sizeof(Elf64_Sym));
            ptr += sizeof(Elf64_Sym);
            ret->dynSymbols.list[i].name = strdup(data
                    + ret->sections[dyn_sym_table_name_index].sectionHeader.sh_offset
                    + ret->dynSymbols.list[i].symbol.st_name);

            if(DEBUG_GET_ELF_DATA){ printf("Dyn Symbol Name: %s\n", ret->dynSymbols.list[i].name); }
        }
    }else {
        if(DEBUG_GET_ELF_DATA){ printf("No Dynamic Symbol Table in this file\n"); }
    }
}

ElfData getELFData(const char* executable) {

    ElfData ret;

    struct stat file_stats;
    char* data;
    int file_num = 0;

//  I have questions about this. memset 0 works, setting pointers and values to null and zero does not.
//    init_ElfData(&ret);
    memset(&ret, 0, sizeof(ElfData));

    if(DEBUG_GET_ELF_DATA){ printf("Begin getELFData. executable: %s \n", executable); }

    if(stat(executable, &file_stats) == -1) {
        perror("File stat creation failed. line 65 elf_common.c");
        return ret;
    }

    if(DEBUG_GET_ELF_DATA){ printf("Created file_stats. stats.st_size: %ld\n", file_stats.st_size); }

    if((file_num = open(executable, 0)) == -1) {
        perror("File open failed. line 71 elf_common.c");
        return ret;
    }

    if(DEBUG_GET_ELF_DATA){ printf("Opened executable, file_num: %d \n", file_num); }

    data = mmap(0, file_stats.st_size, PROT_READ, MAP_SHARED, file_num, 0);

    if(data[0] == ELFMAG0 && data[1] == ELFMAG1 && data[2] == ELFMAG2 && data[3] == ELFMAG3){
        if(DEBUG_GET_ELF_DATA){ printf("This is at least pretending to be an ELF file. data: %s\n", data); }
    } else {
        printf("This is not an ELF file, data: %s\n", data);
        return ret;
    }

    memcpy(&ret.elfHeader, data, sizeof(Elf64_Ehdr));

    if(DEBUG_GET_ELF_DATA){ printf("ELF E_Header copied.\n Num Program Headers: %d\n Num Sections: %d \n",
                      ret.elfHeader.e_phnum, ret.elfHeader.e_shnum); }

    buildELFProgramHeader(data, &ret);

    if(DEBUG_GET_ELF_DATA) {
        for (int i = 0; i < ret.elfHeader.e_phnum; i++) {
            printf("Program Header name[%d]: %d\n", i, ret.programHeader[i].p_type);
        }
    }

    buildELFSections(data, &ret);

    build_ElfSymbolLists(data, &ret);

    close(file_num);
    munmap(data, file_stats.st_size);

    if(DEBUG_GET_ELF_DATA) { printf("ELF header close.\n"); }

    return ret;
}

void destroyELFData(ElfData elfData) {

    if(DEBUG_DESTROY){printf("1\n"); }
    free(elfData.programHeader);

    if(DEBUG_DESTROY){ printf("2\n"); }

    for (int k = 0; k < elfData.elfHeader.e_shnum; ++k) {
        free(elfData.sections[k].sectionName);
    }
    free(elfData.sections);

    if(DEBUG_DESTROY){ printf("3 other sym size: %d\n", elfData.otherSymbols.size); }

    if(elfData.otherSymbols.size){
        for (int i = 0; i < elfData.otherSymbols.size; ++i) {
            free(elfData.otherSymbols.list[i].name);
        }
        free(elfData.otherSymbols.list);
    }

    if(DEBUG_DESTROY){ printf("4 other dyn size: %d\n", elfData.dynSymbols.size); }

    if(elfData.dynSymbols.size){
        for (int j = 0; j < elfData.dynSymbols.size; ++j) {
        free(elfData.dynSymbols.list[j].name);
        }
        free(elfData.dynSymbols.list);
    }

    if(DEBUG_DESTROY){ printf("5\n"); }
}