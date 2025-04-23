#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <getopt.h>

// Flags pour les différentes options
typedef struct {
    int show_file_header;      // -h option
    int show_help;             // --help option
} options_t;

void print_usage(const char *prog_name) {
    printf("Usage: %s [options] <file>\n", prog_name);
    printf("Options:\n");
    printf("  -h           Display the ELF file header\n");
    printf("  --help       Display this help message\n");
}

void print_elf_header(Elf64_Ehdr *ehdr) {
    printf("ELF Header:\n");
    printf("  Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", ehdr->e_ident[i]);
    }
    printf("\n");
    
    printf("  Class:                             %s\n", 
           ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : 
           ehdr->e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : "Invalid");
    
    printf("  Data:                              %s\n",
           ehdr->e_ident[EI_DATA] == ELFDATA2LSB ? "2's complement, little endian" :
           ehdr->e_ident[EI_DATA] == ELFDATA2MSB ? "2's complement, big endian" : "Invalid");
    
    printf("  Version:                           %d%s\n", 
           ehdr->e_ident[EI_VERSION],
           ehdr->e_ident[EI_VERSION] == EV_CURRENT ? " (current)" : "");
    
    printf("  OS/ABI:                            %d\n", ehdr->e_ident[EI_OSABI]);
    printf("  ABI Version:                       %d\n", ehdr->e_ident[EI_ABIVERSION]);
    
    printf("  Type:                              ");
    switch (ehdr->e_type) {
        case ET_NONE: printf("NONE (No file type)\n"); break;
        case ET_REL: printf("REL (Relocatable file)\n"); break;
        case ET_EXEC: printf("EXEC (Executable file)\n"); break;
        case ET_DYN: printf("DYN (Shared object file)\n"); break;
        case ET_CORE: printf("CORE (Core file)\n"); break;
        default: printf("<unknown>: %d\n", ehdr->e_type);
    }
    
    printf("  Machine:                           ");
    switch (ehdr->e_machine) {
        case EM_X86_64: printf("AMD x86-64\n"); break;
        case EM_386: printf("Intel 80386\n"); break;
        case EM_ARM: printf("ARM\n"); break;
        case EM_AARCH64: printf("AArch64\n"); break;
        default: printf("<unknown>: %d\n", ehdr->e_machine);
    }
    
    printf("  Version:                           0x%x\n", ehdr->e_version);
    printf("  Entry point address:               0x%lx\n", (unsigned long)ehdr->e_entry);
    printf("  Start of program headers:          %lu (bytes into file)\n", (unsigned long)ehdr->e_phoff);
    printf("  Start of section headers:          %lu (bytes into file)\n", (unsigned long)ehdr->e_shoff);
    printf("  Flags:                             0x%x\n", ehdr->e_flags);
    printf("  Size of this header:               %d (bytes)\n", ehdr->e_ehsize);
    printf("  Size of program headers:           %d (bytes)\n", ehdr->e_phentsize);
    printf("  Number of program headers:         %d\n", ehdr->e_phnum);
    printf("  Size of section headers:           %d (bytes)\n", ehdr->e_shentsize);
    printf("  Number of section headers:         %d\n", ehdr->e_shnum);
    printf("  Section header string table index: %d\n", ehdr->e_shstrndx);
}

int main(int argc, char *argv[]) {
    options_t opts = {0};  // Initialize all options to 0
    int c;
    
    // Structure for long options
    static struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    
    // Parse command line options
    while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                // Long options
                if (strcmp(long_options[option_index].name, "help") == 0) {
                    opts.show_help = 1;
                }
                break;
            case 'h':
                opts.show_file_header = 1;
                break;
            case '?':
                // Invalid option - getopt already prints an error message
                return 1;
            default:
                abort();
        }
    }
    
    // Show help and exit if requested
    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Check if a filename was provided
    if (optind >= argc) {
        fprintf(stderr, "Error: Expected a filename\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // If no specific options were provided, default to showing file header
    if (!opts.show_file_header) {
        opts.show_file_header = 1; // Default behavior like objdump
    }
    
    // Open the file
    const char *filename = argv[optind];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return 1;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error getting file stats: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        fprintf(stderr, "Error mapping file: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    // Check if the file is an ELF file
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file_data;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Error: Not an ELF file: %s\n", filename);
        munmap(file_data, st.st_size);
        close(fd);
        return 1;
    }
    
    // Print file information
    printf("\n%s:     file format elf64-x86-64\n\n", filename);
    
    // Display the ELF header if requested
    if (opts.show_file_header) {
        print_elf_header(ehdr);
    }
    
    munmap(file_data, st.st_size);
    close(fd);
    return 0;
}
