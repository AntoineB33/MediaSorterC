#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <direct.h>
#include <cjson/cJSON.h>

#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavutil/samplefmt.h>

//#include <libswscale/swscale.h>
//#include <libavutil/imgutils.h>


#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_VOLUME 128  // Max SDL volume

#define MAX_LISTS 100
#define MAX_URLS_PER_LIST 100
#define MAX_URL_LEN 1024
#define MAX_FOUND_FILES 1000


// Global variable to store volume level
int volume_level = MAX_VOLUME / 2;  // Start at 50% volume
int audio_stream_index = -1; // Declare it globally


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
    size_t url_len = strlen(url);

    // Calculate the required size: two hex digits per character plus a null terminator
    size_t required_size = url_len * 2 + 1;

    // Check if the provided buffer is large enough
    if (filename_size < required_size) {
        if (filename_size > 0) {
            filename[0] = '\0'; // Ensure the filename is a valid empty string
        }
        return;
    }

    const unsigned char* ptr = (const unsigned char*)url;
    char* out = filename;
    const char hex_digits[] = "0123456789ABCDEF";

    // Convert each character in the URL to its hexadecimal representation
    while (*ptr) {
        unsigned char c = *ptr++;
        *out++ = hex_digits[(c >> 4) & 0xF]; // High nibble
        *out++ = hex_digits[c & 0xF];        // Low nibble
    }
    *out = '\0'; // Null-terminate the string
}

// Function to match a filename with a pattern (supporting basic '*' wildcard)
int wildcard_match(const char* filename, const char* pattern) {
    // Split the pattern into base name and extension
    const char* dot = strchr(pattern, '.');
    if (!dot) {
        // No extension in pattern, match entire filename
        return strncmp(filename, pattern, strlen(pattern)) == 0;
    }

    // Separate the base name and extension
    size_t base_len = dot - pattern;
    const char* pattern_base = pattern;
    const char* pattern_ext = dot + 1; // Skip the '.'

    // Match the base of the filename with the base of the pattern
    if (strncmp(filename, pattern_base, base_len) != 0) {
        return 0; // Base part doesn't match
    }

    // Check if the pattern extension is '*' (wildcard for any extension)
    if (strcmp(pattern_ext, "*") == 0) {
        return 1; // Matches any extension
    }

    // Now match the exact extension
    const char* file_ext = strrchr(filename, '.');
    if (!file_ext) {
        return 0; // No extension in filename
    }

    // Compare extensions
    return strcmp(file_ext + 1, pattern_ext) == 0;
}

// Recursive function to search for files matching the pattern in a folder and its subfolders
void recursive_search(const char* folder_path, const char* search_pattern, const char* url, int* found, char found_files[MAX_FOUND_FILES][300], int* file_count) {
    char path[300];
    struct _finddata_t c_file;
    intptr_t hFile;

    // Construct the search pattern for files
    snprintf(path, sizeof(path), "%s\\*.*", folder_path);

    if ((hFile = _findfirst(path, &c_file)) == -1L) {
        return; // No files found in this directory
    }

    do {
        if (strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0) {
            continue; // Skip the current and parent directories
        }

        snprintf(path, sizeof(path), "%s\\%s", folder_path, c_file.name);

        if (c_file.attrib & _A_SUBDIR) {
            // Recursively search subdirectories
            recursive_search(path, search_pattern, url, found, found_files, file_count);
        }
        else {
            // Use wildcard_match to check if the file matches the search pattern
            if (wildcard_match(c_file.name, search_pattern)) {
                *found = 1;
                // Add the file path to the found_files array
                snprintf(found_files[*file_count], 300, "%s\\%s", folder_path, c_file.name);
                (*file_count)++;
                return;
            }
        }
    } while (_findnext(hFile, &c_file) == 0);

    _findclose(hFile);
}

int main(int argc, char* argv[]) {
    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd)) != NULL) {  // Use _getcwd instead of getcwd
        printf("Current working directory: %s\n", cwd);
    }
    else {
        perror("_getcwd() error");
    }

    const char* input = argv[1];

    // Parse the JSON input
    cJSON* root = cJSON_Parse(input);
    if (!root || !cJSON_IsArray(root)) {
        fprintf(stderr, "Invalid JSON input %s\n", input);
        return 1;
    }

    int num_lists = cJSON_GetArraySize(root);
    char found_files[MAX_FOUND_FILES][300]; // Array to store paths of found files
    int file_count = 0; // Counter for the number of found files

    for (int i = 0; i < num_lists; i++) {
        cJSON* url_array = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsArray(url_array)) {
            fprintf(stderr, "Expected an array of URLs at position %d\n", i);
            continue;
        }

        int num_urls = cJSON_GetArraySize(url_array);
        int found = 0;

        for (int j = 0; j < num_urls; j++) {
            cJSON* url_item = cJSON_GetArrayItem(url_array, j);
            if (!cJSON_IsString(url_item)) {
                fprintf(stderr, "Expected a URL string at position [%d][%d]\n", i, j);
                continue;
            }

            const char* url = url_item->valuestring;

            // Process URL to generate filename
            char filename[256];
            url_to_filename(url, filename, sizeof(filename));

            // Now, construct the search pattern
            char search_pattern[300];
            snprintf(search_pattern, sizeof(search_pattern), "%s.*", filename);

            // Start recursive search in the base directory
            const char* folder_path = "..\\..\\..\\..\\media";
            recursive_search(folder_path, search_pattern, url, &found, found_files, &file_count);

            if (found) {
                break; // Found a matching file for this list
            }
        }

        if (!found) {
            printf("No file found for list %d\n", i);
        }
    }

    // Print the found files
    printf("Found files:\n");
    for (int i = 0; i < file_count; i++) {
        printf("%s\n", found_files[i]);
    }

    // Free cJSON root
    cJSON_Delete(root);

    play_video("qj0sopzku6kc1.mp4");

    return 0;
}
