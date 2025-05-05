#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

void afficher_aide(const char *nom_prog) {
    printf("Usage: %s [options] <fichier>\n", nom_prog);
    printf("Options:\n");
    printf("  -h           Afficher l'en-tête du fichier ELF\n");
    printf("  --help       Afficher ce message d'aide\n");
}

void afficher_entete_elf(Elf64_Ehdr *ehdr) {
    printf("ELF Header:\n");
    printf("  Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++) printf("%02x ", ehdr->e_ident[i]);
    printf("\n");

    printf("  Classe:                            %s\n", 
           ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "Invalide");
    printf("  Données:                           %s\n",
           ehdr->e_ident[EI_DATA] == ELFDATA2LSB ? "Little Endian" : "Big Endian");
    printf("  Version:                           %d\n", ehdr->e_ident[EI_VERSION]);
    printf("  OS/ABI:                            %d\n", ehdr->e_ident[EI_OSABI]);
    printf("  Type:                              %d\n", ehdr->e_type);
    printf("  Machine:                           %d\n", ehdr->e_machine);
    printf("  Entrée point:                      0x%lx\n", (unsigned long)ehdr->e_entry);
}

const char *type_section(uint32_t type) {
    switch (type) {
        case SHT_NULL:      return "NULL";
        case SHT_PROGBITS:  return "PROGBITS";
        case SHT_SYMTAB:    return "SYMTAB";
        case SHT_STRTAB:    return "STRTAB";
        case SHT_RELA:      return "RELA";
        case SHT_HASH:      return "HASH";
        case SHT_DYNAMIC:   return "DYNAMIC";
        case SHT_NOTE:      return "NOTE";
        case SHT_NOBITS:    return "NOBITS";
        case SHT_REL:       return "REL";
        case SHT_SHLIB:     return "SHLIB";
        case SHT_DYNSYM:    return "DYNSYM";
        default:            return "AUTRE";
    }
}

void afficher_sections(Elf64_Ehdr *ehdr, void *data) {
    Elf64_Shdr *sections = (Elf64_Shdr *)((char *)data + ehdr->e_shoff);
    int nb_sections = ehdr->e_shnum;
    int index_strtab = ehdr->e_shstrndx;

    const char *shstrtab = (const char *)data + sections[index_strtab].sh_offset;

    printf("\nNombre de sections : %d\n", nb_sections);
    printf("Sections:\n");
    printf("  [Index] Nom                 Type       Offset     Taille\n");

    for (int i = 0; i < nb_sections; i++) {
        const char *nom_section = shstrtab + sections[i].sh_name;
        const char *type = type_section(sections[i].sh_type);

        printf("  [%2d]    %-18s %-10s 0x%08lx 0x%08lx\n", 
               i, nom_section, type,
               (unsigned long)sections[i].sh_offset,
               (unsigned long)sections[i].sh_size);
    }
}

int main(int argc, char *argv[]) {
    int afficher_header = 0;

    if (argc < 2) {
        fprintf(stderr, "Erreur : fichier manquant.\n");
        afficher_aide(argv[0]);
        return 1;
    }

    const char *fichier = NULL;

    // Analyse des arguments
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            afficher_aide(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-h")) {
            afficher_header = 1;
        } else if (argv[i][0] != '-') {
            fichier = argv[i];
        }
    }

    if (!fichier) {
        fprintf(stderr, "Erreur : fichier manquant.\n");
        afficher_aide(argv[0]);
        return 1;
    }

    int fd = open(fichier, O_RDONLY);
    if (fd < 0) {
        perror("Erreur à l'ouverture du fichier");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Erreur stat");
        close(fd);
        return 1;
    }

    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Erreur mmap");
        close(fd);
        return 1;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)data;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Ce fichier n'est pas un ELF valide.\n");
        munmap(data, st.st_size);
        close(fd);
        return 1;
    }

    printf("\n%s : format elf64-x86-64\n\n", fichier);
    if (afficher_header || argc == 2) {
        afficher_entete_elf(ehdr);
        afficher_sections(ehdr, data);
    }

    munmap(data, st.st_size);
    close(fd);
    return 0;
}