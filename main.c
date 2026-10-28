
#include <stdio.h>
#include <string.h>

int main(void) {
    FILE *file = fopen("program.mini", "r");

    if (file == NULL) {
        printf("Error: Could not open program.mini\n");
        return 1;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file) != NULL) {
        
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "print ", 6) == 0) {
            char *message = line + 6;

            size_t length = strlen(message);

            if (length >= 2 &&
                message[0] == '"' &&
                message[length - 1] == '"') {
                message[length - 1] = '\0';
                message++;
            }

            printf("%s\n", message);
        } else if (line[0] != '\0') {
            printf("Error: Unknown command: %s\n", line);
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}
