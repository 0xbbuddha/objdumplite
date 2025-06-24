#ifndef OBJDUMPLITE_H
#define OBJDUMPLITE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <elf.h>

// Définitions des couleurs pour la sortie console
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

// Structure pour stocker les informations du fichier ELF
typedef struct {
    int fd;                 // Descripteur du fichier
    Elf64_Ehdr header;      // En-tête ELF
    Elf64_Shdr *sections;   // Tableau des en-têtes de section
    char *section_names;    // Noms des sections
} elf_file_t;

// Prototypes des fonctions
void print_help();
void print_elf_header(elf_file_t *elf_file);
void print_elf_sections(elf_file_t *elf_file);
const char *get_section_type_name(uint32_t type);

#endif // OBJDUMPLITE_H
