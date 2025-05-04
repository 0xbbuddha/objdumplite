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

int main(int argc, char *argv[]) {
    int afficher_header = 0;

    // Analyse des arguments
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            afficher_aide(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-h")) {
            afficher_header = 1;
        } else if (argv[i][0] != '-') {
            // Fichier ELF spécifié
            const char *fichier = argv[i];
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
            }

            munmap(data, st.st_size);
            close(fd);
            return 0;
        }
    }

    fprintf(stderr, "Erreur : fichier manquant.\n");
    afficher_aide(argv[0]);
    return 1;
}
