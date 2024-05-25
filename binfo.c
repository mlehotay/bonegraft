#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define E_FAI(x) { fprintf(stderr, x); fflush(stderr); exit(EXIT_FAILURE); }

typedef struct {
    uint32_t incarnation;
    uint32_t feature_set;
    uint32_t entity_count;
    uint32_t struct_sizes;
} bones_header_t;

typedef struct {
    bones_header_t header;
    uint8_t vmajor, vminor, vpatch, vedit;
    uint16_t nmonsters, nobjects, nartifacts;
    uint8_t struct_sizes[4];
    uint8_t has_goldobj, has_zerocomp, has_rlecomp;
} bones_t;

uint32_t swap_endian_32(uint32_t value) {
    return ((value >> 24) & 0xff) | ((value << 8) & 0xff0000) | ((value >> 8) & 0xff00) | ((value << 24) & 0xff000000);
}

uint64_t swap_endian_64(uint64_t value) {
    return ((value >> 56) & 0xff) |
           ((value << 40) & 0xff000000000000ULL) |
           ((value >> 40) & 0xff0000000000ULL) |
           ((value << 24) & 0xff00000000ULL) |
           ((value >> 24) & 0xff000000ULL) |
           ((value << 8) & 0xff0000ULL) |
           ((value >> 8) & 0xff00ULL) |
           ((value << 56) & 0xff00000000000000ULL);
}

void read_bytes(FILE *file, void *buffer, size_t size, int big_endian) {
    fread(buffer, size, 1, file);
    if (big_endian) {
        if (size == 4) {
            uint32_t *value = (uint32_t *)buffer;
            *value = swap_endian_32(*value);
        } else if (size == 8) {
            uint64_t *value = (uint64_t *)buffer;
            *value = swap_endian_64(*value);
        }
    }
}

int detect_endianness_and_read_incarnation(FILE *file, uint32_t *incarnation, int big_endian) {
    uint32_t value;
    uint8_t bytes[4];

    fread(bytes, sizeof(uint8_t), 4, file);
    if (big_endian) {
        value = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    } else {
        value = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    }

    fseek(file, 0, SEEK_SET);

    uint8_t vmajor = (value >> 24) & 0xFF;
    uint8_t vminor = (value >> 16) & 0xFF;
    uint8_t vpatch = (value >> 8) & 0xFF;

    if (vmajor == 3 && vminor < 10) {
        *incarnation = value;
        return 1;
    } else {
        return 0;
    }
}

void read_bones_header(FILE *file, bones_t *bones, int big_endian, int use_64bit, int use_16bit_ints) {
    if (use_64bit) {
        uint64_t temp;
        read_bytes(file, &temp, sizeof(uint64_t), big_endian);
        bones->header.incarnation = (uint32_t)(temp & 0xFFFFFFFF);
        read_bytes(file, &temp, sizeof(uint64_t), big_endian);
        bones->header.feature_set = (uint32_t)(temp & 0xFFFFFFFF);
        read_bytes(file, &temp, sizeof(uint64_t), big_endian);
        bones->header.entity_count = (uint32_t)(temp & 0xFFFFFFFF);
        read_bytes(file, &temp, sizeof(uint64_t), big_endian);
        bones->header.struct_sizes = (uint32_t)(temp & 0xFFFFFFFF);
    } else {
        read_bytes(file, &bones->header.incarnation, sizeof(uint32_t), big_endian);
        read_bytes(file, &bones->header.feature_set, sizeof(uint32_t), big_endian);
        read_bytes(file, &bones->header.entity_count, sizeof(uint32_t), big_endian);
        read_bytes(file, &bones->header.struct_sizes, sizeof(uint32_t), big_endian);
    }

    uint32_t incarnation = bones->header.incarnation;
    bones->vmajor = (incarnation >> 24) & 0xFF;
    bones->vminor = (incarnation >> 16) & 0xFF;
    bones->vpatch = (incarnation >> 8) & 0xFF;
    bones->vedit = incarnation & 0xFF;

    if (use_16bit_ints) {
        bones->nartifacts = (bones->header.entity_count >> 16) & 0xFFFF;
        bones->nobjects = (bones->header.entity_count >> 8) & 0xFF;
        bones->nmonsters = bones->header.entity_count & 0xFF;
    } else {
        bones->nartifacts = (bones->header.entity_count >> 24) & 0xFF;
        bones->nobjects = (bones->header.entity_count >> 12) & 0xFFF;
        bones->nmonsters = bones->header.entity_count & 0xFFF;
    }

    bones->struct_sizes[0] = (bones->header.struct_sizes >> 24) & 0xFF;
    bones->struct_sizes[1] = (bones->header.struct_sizes >> 16) & 0xFF;
    bones->struct_sizes[2] = (bones->header.struct_sizes >> 8) & 0xFF;
    bones->struct_sizes[3] = bones->header.struct_sizes & 0xFF;

    bones->has_goldobj = (bones->header.feature_set & (1 << 12)) != 0;
    bones->has_zerocomp = (bones->header.feature_set & (1 << 27)) != 0;
    bones->has_rlecomp = (bones->header.feature_set & (1 << 28)) != 0;
}

void print_bones_info(bones_t *bones) {
    const char *bits[] = {
        "REINCARNATION", "SINKS", "<bit 3>", "<bit 4>", "ARMY", "KOPS", "MAIL", "<bit 8>",
        "<bit 9>", "TOURIST", "STEED", "GOLDOBJ", "<bit 13>", "<bit 14>", "MUSE", "POLYSELF",
        "TEXTCOLOR", "INSURANCE", "ELBERETH", "EXP_ON_BOTL", "SCORE_ON_BOTL", "WEAPON_SKILLS",
        "TIMED_DELAY", "<bit 24>", "<bit 25>", "<bit 26>", "ZEROCOMP", "RLECOMP", "<bit 29>",
        "<bit 30>", "<bit 31>"
    };

    printf("Bone Graft v0.01\n");
    printf("NetHack version %d.%d.%d editlevel %d\n", bones->vmajor, bones->vminor, bones->vpatch, bones->vedit);

    printf("Features\n");
    printf(" Dungeon:");
    for (int i = 0; i < 8; i++) {
        if (bones->header.feature_set & (1 << i)) {
            printf(" %s", bits[i]);
        }
    }
    printf("\n");

    printf(" Monsters:");
    for (int i = 8; i < 16; i++) {
        if (bones->header.feature_set & (1 << i)) {
            printf(" %s", bits[i]);
        }
    }
    printf("\n");

    printf(" Objects:");
    for (int i = 16; i < 24; i++) {
        if (bones->header.feature_set & (1 << i)) {
            printf(" %s", bits[i]);
        }
    }
    printf("\n");

    printf(" Flags:");
    for (int i = 24; i < 28; i++) {
        if (bones->header.feature_set & (1 << i)) {
            printf(" %s", bits[i]);
        }
    }
    printf("\n");

    printf(" Format:");
    for (int i = 28; i < 32; i++) {
        if (bones->header.feature_set & (1 << i)) {
            printf(" %s", bits[i]);
        }
    }
    printf("\n");

    printf("Entity counts\n");
    printf(" Monsters: %d\n", bones->nmonsters);
    printf(" Objects: %d\n", bones->nobjects);
    printf(" Artifacts: %d\n", bones->nartifacts);

    printf("Structure sizes\n");
    printf(" you: %d\n", bones->struct_sizes[3]);
    printf(" monst: %d\n", bones->struct_sizes[2]);
    printf(" obj: %d\n", bones->struct_sizes[1]);
    printf(" flag: %d\n", bones->struct_sizes[0]);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        E_FAI("Usage: binfo <bonesfile>\n");
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Failed to open file %s\n", filename);
        return EXIT_FAILURE;
    }

    printf("Testing configuration: little endian, 32-bit long, 32-bit int\n");
    bones_t bones;
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    printf("%s: %ld bytes\n", filename, file_size);

    int big_endian = 0;
    int use_64bit = 0;
    int use_16bit_ints = 0;

    if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
        read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
        print_bones_info(&bones);
    } else {
        printf("Testing configuration: big endian, 32-bit long, 32-bit int\n");
        big_endian = 1;
        rewind(file);
        if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
            read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
            print_bones_info(&bones);
        } else {
            printf("Testing configuration: little endian, 64-bit long, 64-bit int\n");
            use_64bit = 1;
            rewind(file);
            if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
                read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
                print_bones_info(&bones);
            } else {
                printf("Testing configuration: big endian, 64-bit long, 64-bit int\n");
                big_endian = 1;
                rewind(file);
                if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
                    read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
                    print_bones_info(&bones);
                } else {
                    printf("Testing configuration: little endian, 64-bit long, 32-bit int\n");
                    use_64bit = 1;
                    use_16bit_ints = 1;
                    rewind(file);
                    if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
                        read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
                        print_bones_info(&bones);
                    } else {
                        printf("Testing configuration: big endian, 64-bit long, 32-bit int\n");
                        big_endian = 1;
                        rewind(file);
                        if (detect_endianness_and_read_incarnation(file, &bones.header.incarnation, big_endian)) {
                            read_bones_header(file, &bones, big_endian, use_64bit, use_16bit_ints);
                            print_bones_info(&bones);
                        } else {
                            printf("Failed to detect valid incarnation version in bones file\n");
                        }
                    }
                }
            }
        }
    }

    fclose(file);

    return EXIT_SUCCESS;
}
