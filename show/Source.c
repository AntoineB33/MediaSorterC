#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to decode percent-encoded characters in a URL
void url_decode(const char* src, char* dest, size_t dest_size) {
    char a, b;
    size_t i = 0;
    while (*src && i < dest_size - 1) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            dest[i++] = 16 * a + b;
            src += 3;
        }
        else if (*src == '+') {
            dest[i++] = ' ';
            src++;
        }
        else {
            dest[i++] = *src++;
        }
    }
    dest[i] = '\0';
}

// Function to sanitize file name by replacing invalid characters
void sanitize_filename(char* filename) {
    const char invalid_chars[] = "\\/:*?\"<>|";
    for (char* p = filename; *p; p++) {
        if (strchr(invalid_chars, *p)) {
            *p = '_'; // Replace invalid character with '_'
        }
    }
}

// Function to safely include query parameters in file name
void include_query_params(char* filename, const char* query) {
    if (query && *query) {
        strcat_s(filename, 256, "_");
        char sanitized_query[256];
        strncpy_s(sanitized_query, sizeof(sanitized_query), query, _TRUNCATE);
        sanitize_filename(sanitized_query);
        strcat_s(filename, 256, sanitized_query);
    }
}

// Function to extract file name from URL using strncpy_s
void url_to_filename(const char* url, char* filename, size_t filename_size) {
    // Find the last '/' character
    const char* p = strrchr(url, '/');
    if (p) {
        p++; // Move past '/'
    }
    else {
        p = url; // No '/' found, use the whole URL
    }

    // Copy the path component
    char temp[512];
    strncpy_s(temp, sizeof(temp), p, _TRUNCATE);

    // Find query parameters
    char* q = strchr(temp, '?');
    char* query = NULL;
    if (q) {
        *q = '\0'; // Split the string
        query = q + 1;
    }

    // Decode and sanitize
    url_decode(temp, filename, filename_size);
    sanitize_filename(filename);

    // Include sanitized query parameters
    include_query_params(filename, query);

    // Ensure filename is not empty
    if (strlen(filename) == 0) {
        strncpy_s(filename, filename_size, "index.html", _TRUNCATE);
    }
}

int main() {
    const char* url = "https://www.youtube.com/watch?v=LdMX4-g6BM8";
    char filename[256];

    url_to_filename(url, filename, sizeof(filename));
    printf("File Name: %s\n", filename);

    return 0;
}
