#include "objdumplite.h"

/**
 * @brief Affiche l'aide du programme.
 */
void print_help() {
    printf("Usage: objdumplite [options] <fichier>\n");
    printf("Affiche des informations sur les fichiers objets au format ELF.\n\n");
    printf("Options:\n");
    printf("  -h, --header     Afficher l'en-tête du fichier ELF.\n");
    printf("  --help           Afficher ce message d'aide.\n");
}

/**
 * @brief Récupère le nom correspondant à un type de section ELF.
 * 
 * @param type Le type de la section (ex: SHT_PROGBITS).
 * @return const char* Le nom du type de section sous forme de chaîne de caractères.
 */
const char *get_section_type_name(uint32_t type) {
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
        default: return "UNKNOWN";
    }
}

/**
 * @brief Affiche les informations de l'en-tête du fichier ELF.
 * 
 * @param elf_file Un pointeur vers la structure contenant les données du fichier ELF.
 */
void print_elf_header(elf_file_t *elf_file) {
    Elf64_Ehdr *hdr = &elf_file->header;

    printf(YELLOW "ELF Header:\n" RESET);
    printf(CYAN "  Magic:   " RESET);
    for (int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", hdr->e_ident[i]);
    }
    printf("\n");

    // Classe de l'architecture (32 ou 64 bits)
    printf(CYAN "  Class:                             " RESET);
    switch(hdr->e_ident[EI_CLASS]) {
        case ELFCLASS32: printf("ELF32\n"); break;
        case ELFCLASS64: printf("ELF64\n"); break;
        default: printf("Invalid class\n"); break;
    }

    // Endianness (ordre des octets)
    printf(CYAN "  Data:                              " RESET);
    switch(hdr->e_ident[EI_DATA]) {
        case ELFDATA2LSB: printf("2's complement, little endian\n"); break;
        case ELFDATA2MSB: printf("2's complement, big endian\n"); break;
        default: printf("Invalid data encoding\n"); break;
    }

    printf(CYAN "  Version:                           " RESET "%d (current)\n", hdr->e_ident[EI_VERSION]);

    // OS/ABI
    printf(CYAN "  OS/ABI:                            " RESET);
    switch(hdr->e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV:       printf("UNIX - System V\n"); break;
        case ELFOSABI_HPUX:       printf("HP-UX\n"); break;
        case ELFOSABI_NETBSD:     printf("NetBSD\n"); break;
        case ELFOSABI_LINUX:      printf("Linux\n"); break;
        case ELFOSABI_SOLARIS:    printf("Solaris\n"); break;
        case ELFOSABI_IRIX:       printf("IRIX\n"); break;
        case ELFOSABI_FREEBSD:    printf("FreeBSD\n"); break;
        case ELFOSABI_TRU64:      printf("TRU64 UNIX\n"); break;
        case ELFOSABI_ARM:        printf("ARM\n"); break;
        case ELFOSABI_STANDALONE: printf("Standalone App\n"); break;
        default:                  printf("Unknown OS/ABI\n"); break;
    }
    
    printf(CYAN "  ABI Version:                       " RESET "%d\n", hdr->e_ident[EI_ABIVERSION]);
    
    // Type du fichier objet
    printf(CYAN "  Type:                              " RESET);
    switch(hdr->e_type) {
        case ET_NONE:   printf("NONE (Unknown type)\n"); break;
        case ET_REL:    printf("REL (Relocatable file)\n"); break;
        case ET_EXEC:   printf("EXEC (Executable file)\n"); break;
        case ET_DYN:    printf("DYN (Shared object file)\n"); break;
        case ET_CORE:   printf("CORE (Core file)\n"); break;
        default:        printf("Unknown type\n"); break;
    }

    printf(CYAN "  Machine:                           " RESET "%d\n", hdr->e_machine); // On pourrait mapper vers des noms
    printf(CYAN "  Version:                           " RESET "0x%x\n", hdr->e_version);
    printf(CYAN "  Entry point address:               " RESET MAGENTA "0x%lx\n" RESET, hdr->e_entry);
    printf(CYAN "  Start of program headers:          " RESET "%ld (bytes into file)\n", hdr->e_phoff);
    printf(CYAN "  Start of section headers:          " RESET "%ld (bytes into file)\n", hdr->e_shoff);
    printf(CYAN "  Flags:                             " RESET "0x%x\n", hdr->e_flags);
    printf(CYAN "  Size of this header:               " RESET "%d (bytes)\n", hdr->e_ehsize);
    printf(CYAN "  Size of program headers:           " RESET "%d (bytes)\n", hdr->e_phentsize);
    printf(CYAN "  Number of program headers:         " RESET "%d\n", hdr->e_phnum);
    printf(CYAN "  Size of section headers:           " RESET "%d (bytes)\n", hdr->e_shentsize);
    printf(CYAN "  Number of section headers:         " RESET "%d\n", hdr->e_shnum);
    printf(CYAN "  Section header string table index: " RESET "%d\n", hdr->e_shstrndx);
}

/**
 * @brief Affiche les informations sur les sections du fichier ELF.
 * 
 * @param elf_file Un pointeur vers la structure contenant les données du fichier ELF.
 */
void print_elf_sections(elf_file_t *elf_file) {
    printf(GREEN "\nIl y a %d en-têtes de section, commençant à l'adresse 0x%lx:\n" RESET,
           elf_file->header.e_shnum, elf_file->header.e_shoff);

    printf(YELLOW "En-têtes de section:\n" RESET);
    printf(CYAN "  [Nr] Nom                Type               Adresse          Offset\n" RESET);
    printf(CYAN "       Taille             EntSize            Flags  Link  Info  Align\n" RESET);

    for (int i = 0; i < elf_file->header.e_shnum; i++) {
        Elf64_Shdr *shdr = &elf_file->sections[i];
        printf("  [%2d] %-17s " BLUE "%-18s" RESET " " MAGENTA "%016lx" RESET " %08lx\n",
               i,
               elf_file->section_names + shdr->sh_name,
               get_section_type_name(shdr->sh_type),
               shdr->sh_addr,
               shdr->sh_offset);
        printf("       " MAGENTA "%016lx %016lx " RESET "%5ld %6d %5d %5ld\n",
               shdr->sh_size,
               shdr->sh_entsize,
               shdr->sh_flags,
               shdr->sh_link,
               shdr->sh_info,
               shdr->sh_addralign);
    }
}

/**
 * @brief Point d'entrée du programme.
 * 
 * @param argc Nombre d'arguments de la ligne de commande.
 * @param argv Tableau des arguments de la ligne de commande.
 * @return int Code de sortie du programme.
 */
int main(int argc, char *argv[]) {
    const char *filename = NULL;
    int show_header = 0;
    ssize_t bytes_read = 0;

    // Analyse simple des arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--header") == 0) {
            show_header = 1;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (!filename) {
        fprintf(stderr, RED "Erreur: nom de fichier manquant.\n" RESET);
        print_help();
        return EXIT_FAILURE;
    }

    elf_file_t elf_file = {0};
    elf_file.fd = open(filename, O_RDONLY);
    if (elf_file.fd < 0) {
        perror(RED "Erreur lors de l'ouverture du fichier" RESET);
        return EXIT_FAILURE;
    }

    // Lire l'en-tête ELF
    bytes_read = read(elf_file.fd, &elf_file.header, sizeof(Elf64_Ehdr));
    if (bytes_read != sizeof(Elf64_Ehdr)) {
        fprintf(stderr, RED "Erreur: Impossible de lire l'en-tête ELF.\n" RESET);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }

    // Vérifier le "nombre magique" pour s'assurer que c'est un fichier ELF
    if (memcmp(elf_file.header.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, RED "Erreur: Ce n'est pas un fichier au format ELF.\n" RESET);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }
    
    // Pour la simplicité, on ne gère que le 64-bit pour le moment
    if (elf_file.header.e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, RED "Erreur: Seuls les fichiers ELF 64-bit sont supportés par cette version.\n" RESET);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }

    // Allouer de la mémoire pour les en-têtes de section
    elf_file.sections = malloc(elf_file.header.e_shentsize * elf_file.header.e_shnum);
    if (!elf_file.sections) {
        perror(RED "Erreur d'allocation mémoire pour les sections" RESET);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }
    
    // Se déplacer au début des en-têtes de section et les lire
    lseek(elf_file.fd, elf_file.header.e_shoff, SEEK_SET);
    size_t sections_size = (size_t)elf_file.header.e_shentsize * elf_file.header.e_shnum;
    bytes_read = read(elf_file.fd, elf_file.sections, sections_size);
    if (bytes_read < 0 || (size_t)bytes_read != sections_size) {
        fprintf(stderr, RED "Erreur: Impossible de lire les en-têtes de section.\n" RESET);
        free(elf_file.sections);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }

    // Lire la table des noms de section
    Elf64_Shdr *shstrtab = &elf_file.sections[elf_file.header.e_shstrndx];
    elf_file.section_names = malloc(shstrtab->sh_size);
    if (!elf_file.section_names) {
        perror(RED "Erreur d'allocation mémoire pour les noms de section" RESET);
        free(elf_file.sections);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }
    lseek(elf_file.fd, shstrtab->sh_offset, SEEK_SET);
    bytes_read = read(elf_file.fd, elf_file.section_names, shstrtab->sh_size);
    if (bytes_read < 0 || (size_t)bytes_read != shstrtab->sh_size) {
        fprintf(stderr, RED "Erreur: Impossible de lire les noms des sections.\n" RESET);
        free(elf_file.section_names);
        free(elf_file.sections);
        close(elf_file.fd);
        return EXIT_FAILURE;
    }

    // Afficher les informations demandées
    printf("\nFichier : %s\n", filename);
    if (show_header) {
        print_elf_header(&elf_file);
    } else {
        // Comportement par défaut: tout afficher
        print_elf_header(&elf_file);
        print_elf_sections(&elf_file);
    }
    
    // Nettoyage
    free(elf_file.sections);
    free(elf_file.section_names);
    close(elf_file.fd);

    return EXIT_SUCCESS;
}
