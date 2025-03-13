#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/**
 * @brief Validates if an ELF file is compiled for AArch64 architecture
 * 
 * This program checks if a given ELF file is compiled for the AArch64 
 * (ARM 64-bit) architecture by examining its ELF header.
 */
class ElfAarch64Validator {
private:
    std::string filename;
    int fd;
    void* mapped_data;
    size_t file_size;
    
    bool is_valid_elf() {
        if (file_size < sizeof(Elf64_Ehdr)) {
            std::cerr << "File too small to be a valid ELF file" << std::endl;
            return false;
        }
        
        Elf64_Ehdr* ehdr = static_cast<Elf64_Ehdr*>(mapped_data);
        
        // Check ELF magic number
        if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
            std::cerr << "Not an ELF file (invalid magic number)" << std::endl;
            return false;
        }
        
        return true;
    }
    
public:
    ElfAarch64Validator(const std::string& file) : filename(file), fd(-1), mapped_data(nullptr), file_size(0) {}
    
    ~ElfAarch64Validator() {
        cleanup();
    }
    
    bool open_file() {
        fd = ::open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }
        
        struct stat st;
        if (fstat(fd, &st) == -1) {
            std::cerr << "Failed to get file stats" << std::endl;
            close(fd);
            fd = -1;
            return false;
        }
        
        file_size = st.st_size;
        
        mapped_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped_data == MAP_FAILED) {
            std::cerr << "Failed to map file into memory" << std::endl;
            close(fd);
            fd = -1;
            return false;
        }
        
        return true;
    }
    
    void cleanup() {
        if (mapped_data != nullptr && mapped_data != MAP_FAILED) {
            munmap(mapped_data, file_size);
            mapped_data = nullptr;
        }
        
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }
    
    bool is_aarch64() {
        if (!is_valid_elf()) {
            return false;
        }
        
        Elf64_Ehdr* ehdr = static_cast<Elf64_Ehdr*>(mapped_data);
        
        // Check if 64-bit
        if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
            std::cout << "Not a 64-bit ELF file" << std::endl;
            return false;
        }
        
        // Check machine type for AArch64
        // EM_AARCH64 is defined as 183 in elf.h
        if (ehdr->e_machine != EM_AARCH64) {
            std::cout << "Not an AArch64 ELF file (machine type: " << ehdr->e_machine << ")" << std::endl;
            return false;
        }
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <elf_file>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    ElfAarch64Validator validator(filename);
    
    if (!validator.open_file()) {
        return 1;
    }
    
    if (validator.is_aarch64()) {
        std::cout << "The file '" << filename << "' is an AArch64 ELF file." << std::endl;
        return 0;
    } else {
        std::cout << "The file '" << filename << "' is NOT an AArch64 ELF file." << std::endl;
        return 1;
    }
}