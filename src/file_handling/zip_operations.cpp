#include "../bzip2/bzlib.h"
#include <stdio.h>

// number of characters in index file is around 1M using 1000 records
constexpr int OUTPUT_BUF_MX_SIZE = 200000;

struct WriteBuffer {
    BZFILE *bzFile;
    FILE *file;
    int bzError;
    char *buffer;
    char *bufferOrg;
    int written;

    WriteBuffer(const std::string &filepath) {
        bufferOrg = buffer = (char *) malloc((OUTPUT_BUF_MX_SIZE + 1) * sizeof(char));
        written = 0;

        file = fopen(filepath.c_str(), "w");
        if (file == NULL) {
            perror("Could not open index file");
            std::cout << filepath << std::endl;
            exit(1);
        }

        bzFile = BZ2_bzWriteOpen(&bzError, file, 9, 0, 0);

        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzWriteOpen: %d\n", bzError);
            exit(1);
        }
    }

    inline int charsLeft() {
        return OUTPUT_BUF_MX_SIZE - written;
    }

    void flush() {
        if (written == 0) return;

        BZ2_bzWrite(&bzError, bzFile, bufferOrg, written);

        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzWrite: %d\n", bzError);
            exit(1);
        }

        written = 0;
        buffer = bufferOrg;
    }

    void close() {
        flush();

        unsigned int nbytes_in, nbytes_out;
        BZ2_bzWriteClose(&bzError, bzFile, 0, &nbytes_in, &nbytes_out);
        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzWriteClose: %d\n", bzError);
            exit(1);
        }
        fclose(file);
        free(buffer);
    }

    void write(int num, char end) {
        int rem = charsLeft();

        // cnt = number of characters that would have been written (excluding the null character)
        int cnt = snprintf(buffer, rem, "%d%c", num, end);

        if (rem <= cnt) {
            flush();
            write(num, end);
            return;
        }

        postwrite(cnt);
    }

    void write(char ch) {
        int rem = charsLeft();
        if (rem <= 1) {
            flush();
            write(ch);
            return;
        }
        int cnt = snprintf(buffer, rem, "%c", ch);
        postwrite(cnt);
    }

//    void write(const std::string &s) {
//        int rem = charsLeft();
//        if (rem <= s.size()) { flush(); write(s); return; }
//
//        int cnt = snprintf(buffer, rem, "%s", s.c_str());
//        postwrite(cnt);
//    }

    void postwrite(int outputChars) {
        written += outputChars;
        buffer += outputChars;
    }
};

struct ReadBuffer {
    BZFILE *bzFile;
    FILE *file;
    char *buf;
    int bzError;

    ReadBuffer(const std::string &filePath) {
        file = fopen(filePath.c_str(), "r");
        if (file == NULL) {
            perror("Could not open index file");
            std::cout << filePath << std::endl;
            exit(1);
        }

        bzFile = BZ2_bzReadOpen(&bzError, file, 0, 0, NULL, 0);
        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzReadOpen: %d\n", bzError);
            exit(1);
        }

        buf = (char *) malloc(sizeof(char));
    }

    char readChar() {
        BZ2_bzRead(&bzError, bzFile, buf, 1);

        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzRead: %d\n", bzError);
            exit(1);
        }

        return *buf;
    }

    int readInt(char delim = ' ') {
        int num = 0;

        do {
            const char c = readChar();
            if (c == delim) break;
            assert ('0' <= c and c <= '9');
            num = num * 10 + (c - '0');
        } while (true);

        return num;
    }

    std::pair<int, char> readInt(char d1, char d2) {
        int num = 0;
        char c;

        do {
            c = readChar();
            if (c == d1 or c == d2) break;
            assert('0' <= c and c <= '9');
            num = num * 10 + (c - '0');
        } while (true);

        return {num, c};
    }

    void ignoreTillDelim(char delim = '\n') {
        while (readChar() != delim);
    }

    void close() {
        BZ2_bzReadClose(&bzError, bzFile);
        if (bzError != BZ_OK) {
            fprintf(stderr, "E: BZ2_bzReadClose: %d\n", bzError);
            exit(1);
        }
        int res = fclose(file);
        if (res != 0) {
            fprintf(stderr, "E: fclose readbuffer: %d\n", bzError);
            exit(1);
        }
    }
};
