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

    if (ehdr->e_ident[EI_CLASS] == ELFCLASS32) {
        printf("  Classe:                            ELF32\n");
    } else if (ehdr->e_ident[EI_CLASS] == ELFCLASS64) {
        printf("  Classe:                            ELF64\n");
    } else {
        printf("  Classe:                            Invalide\n");
    }
    if (ehdr->e_ident[EI_DATA] == ELFDATA2LSB) {
        printf("  Données:                           Little Endian (LSB)\n");
    } else if (ehdr->e_ident[EI_DATA] == ELFDATA2MSB) {
        printf("  Données:                           Big Endian (MSB)\n");
    } else {
        printf("  Données:                           Invalide\n");
    }

    printf("  Version:                           0x%d\n", ehdr->e_ident[EI_VERSION]);
    printf("  OS/ABI:                            %d\n", ehdr->e_ident[EI_OSABI]);
    printf("  Type:                              %d\n", ehdr->e_type);
    printf("  Machine:                           %d\n", ehdr->e_machine);
    printf("  Entrée point:                      0x%lx\n", (unsigned long)ehdr->e_entry);
    printf("  Offset des Program Headers:        0x%lx\n", (unsigned long)ehdr->e_phoff);
    printf("  Offset des Section Headers:        0x%lx\n", (unsigned long)ehdr->e_shoff);
    printf("  Flags processeur:                  0x%x\n", ehdr->e_flags);
    printf("  Taille de l'en-tête ELF:           %d (octets)\n", ehdr->e_ehsize);
    printf("  Taille d'une entrée programme:     %d (octets)\n", ehdr->e_phentsize);
    printf("  Nombre d'entrées programme:        %d\n", ehdr->e_phnum);
    printf("  Taille d'une entrée section:       %d (octets)\n", ehdr->e_shentsize);
    printf("  Index de la table des noms de section: %d\n", ehdr->e_shstrndx);
}

const char *type_section(uint32_t type) {
    switch (type) {
        case SHT_NULL: return "NULL";
        case SHT_PROGBITS: return "PROGBITS";
        case SHT_SYMTAB: return "SYMTAB";
        case SHT_STRTAB: return "STRTAB";
        case SHT_RELA: return "RELA";
        case SHT_HASH: return "HASH";
        case SHT_DYNAMIC: return "DYNAMIC";
        case SHT_NOTE: return "NOTE";
        case SHT_NOBITS: return "NOBITS";
        case SHT_REL: return "REL";
        case SHT_SHLIB: return "SHLIB";
        case SHT_DYNSYM: return "DYNSYM";
        default: return "AUTRE";
    }
}

void afficher_sections(int fd, Elf64_Ehdr *ehdr) {
    Elf64_Shdr *sections = malloc(ehdr->e_shentsize * ehdr->e_shnum);
    if (!sections) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    lseek(fd, ehdr->e_shoff, SEEK_SET);
    read(fd, sections, ehdr->e_shentsize * ehdr->e_shnum);

    Elf64_Shdr strtab_hdr = sections[ehdr->e_shstrndx];
    char *shstrtab = malloc(strtab_hdr.sh_size);
    lseek(fd, strtab_hdr.sh_offset, SEEK_SET);
    read(fd, shstrtab, strtab_hdr.sh_size);

    printf("\nNombre de sections : %d\n", ehdr->e_shnum);
    printf("Sections:\n");
    printf("  [Index] Nom                 Type       Offset     Taille\n");

    for (int i = 0; i < ehdr->e_shnum; i++) {
        printf("  [%2d]    %-18s %-10s 0x%08lx 0x%08lx\n",
               i,
               shstrtab + sections[i].sh_name, type_section(sections[i].sh_type),
               (unsigned long)sections[i].sh_offset,
               (unsigned long)sections[i].sh_size);
    }

    free(sections);
    free(shstrtab);
}

int main(int argc, char *argv[]) {
    int afficher_header = 0;
    const char *fichier = NULL;

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
        perror("open");
        exit(EXIT_FAILURE);
    }

    Elf64_Ehdr ehdr;
    read(fd, &ehdr, sizeof(ehdr));

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Ce fichier n'est pas un ELF valide.\n");
        close(fd);
        return 1;
    }

    printf("\n%s : format elf64-x86-64\n\n", fichier);
    if (afficher_header || argc == 2) {
        afficher_entete_elf(&ehdr);
        afficher_sections(fd, &ehdr);
    }

    close(fd);
    return 0;
}