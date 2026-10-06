#include "kernel.h"
#include <string.h>
#include <sys/mman.h>

int generate_pagefault() {
    char filename[] = "pagefault-XXXXXX";
    int fd = mkstemp(filename);
    if (fd == -1) return -1;
    unlink(filename);

    size_t size = sysconf(_SC_PAGESIZE);
    if (ftruncate(fd, size) == -1) {
        close(fd);
        return -1;
    }

    volatile char* data = mmap(NULL, size, PROT_READ,
                               MAP_SHARED, fd, 0);
    close(fd);
    if (data == MAP_FAILED) return -1;

    (void) data[0];
    return munmap((void*) data, size);
}

int main(int argc, char** argv){
    // TODO: parse the arguments in argv. 
    // You can expect argv[1] to be the mode
    // You can expect argv[2] to be the filepath
    // You can expect argv[3] to be the integer width
    // You can expect argv[4] to be the integer height
    // You can expect argv[5] to be the output filepath.

    if(argc != 6) {
        printf("Incorrect number of arguments. Expected: ./build/image_calc <MODE=kernel|mmap|convert|uconvert|fault> <input_image> <width> <height> <output_image_path>\n");
        return -1;
    }

    // TODO: call correct function based on mode
    char* mode = argv[1];
    if (strcmp(mode, "fault") == 0) return generate_pagefault();

    int mapped = strcmp(mode, "mmap") == 0 || strcmp(mode, "uconvert") == 0;
    if (!mapped && strcmp(mode, "kernel") != 0 && strcmp(mode, "convert") != 0) {
        return -1;
    }

    // TODO: allocate the space needed for one image and load the image
    struct image image = {NULL, atoi(argv[3]), atoi(argv[4])};
    if (image.width <= 0 || image.height <= 0) return -1;

    size_t size = sizeof(struct image) +
                  (size_t) image.width * image.height * sizeof(struct pixel);
    int result = mapped ? loadimage_mmap(argv[2], &image) : loadimage(argv[2], &image);
    if (result != 0) return -1;

    if (strcmp(mode, "convert") == 0) {
        result = saveimage_mmap(argv[5], &image);
        free(image.pixels);
        return result;
    }
    if (strcmp(mode, "uconvert") == 0) {
        result = saveimage(argv[5], &image);
        munmap((char*) image.pixels - sizeof(struct image), size);
        return result;
    }

    int kernel[3][3] = {{1,1,1},{1,1,1},{1,1,1}};

    // TODO: call apply kernel with 1/9 (as a float) as the normalization value
    struct image* out = apply_kernel(&image, (int*) kernel, 3, 1.0f / 9);
    if (mapped) {
        munmap((char*) image.pixels - sizeof(struct image), size);
    } else {
        free(image.pixels);
    }
    if (out == NULL) return -1;

    result = saveimage(argv[5], out);
    free(out->pixels);
    free(out);
    return result;
}