#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct{
    int multiLinePrint;
} Interpreter;

void removeNewLine(char *line){
    line[strcspn(line, "\r\n")] = '\0';             // removes \r\n from the line. strcspn returns the position.
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';                         // removes trailing spaces and tabs
    }
}

void printEscaped(const char *s){
    for (; *s != '\0'; s++){
        if (*s == '\\' && s[1] != '\0'){
            s++;
            switch (*s){
                case 'n':  putchar('\n'); break;
                case 't':  putchar('\t'); break;
                case '\\': putchar('\\'); break;
                case '"':  putchar('"');  break;
                default:   putchar('\\'); putchar(*s); break;
            }
        } else {
            putchar(*s);
        }
    }
}

int checkMultiLinePrint(char *line, Interpreter *interpreter){
    if (strncmp(line,"print \"\"\"",9) == 0) {
        if (line[9] != '\0'){
            printf("Error: Multiline print must start with print \"\"\" on its own line\n");
            return -1;
        }
        interpreter->multiLinePrint = 1;
        return 1;
    }
    return 0;
}

int handleMultiLinePrint(char *line, Interpreter *interpreter){
    if (interpreter->multiLinePrint == 0){
        return 0;
    }

    if (strncmp(line, "\"\"\"",3) == 0){
        if (line[3] != '\0'){
            printf("\nError: Multiline print must end with \"\"\" on its own line\n");
            return -1;
        }
        interpreter->multiLinePrint = 0;
        printf("\n");
        return 1;
    }

    printEscaped(line);  
    return 1;
}

int handlePrint(char *line){
    if (strncmp(line, "print ", 6) != 0) {
        return 0;
    }
    char *message = line + 6;

    size_t length = strlen(message);

    if (length >= 2 && message[0] == '"' && message[length - 1] == '"') {
        message[length - 1] = '\0';
        message++;
    }

    printEscaped(message); 
    printf("\n");

    return 1;
}

int handleErrors(char *line){
    if (line[0] == '\0') return 0;
    printf("\nError: Unknown command: %s\n", line);
    return -1;
}


int main(int argc, char *argv[]) {                            // Accepts arguments from the terminal. argc: argument count. For example: .\main.exe program.mini 
    if (argc < 2){                                            // argc = 2, argv[0] = .\main.exe, argv[1] = program.mini
        printf("Usage: ./main.exe <filename>.mini\n");
        return 1;
    }

    char *filename = argv[1];


    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open file\n");
        return 1;
    }

    printf("Running program: %s\n\n", filename);

    char line[1024];
    Interpreter interpreter = {0};


    while (fgets(line, sizeof(line), file) != NULL) {

        removeNewLine(line);

        int status = handleMultiLinePrint(line, &interpreter);
        if(status == -1){
            fclose(file);
            return -1;
        }
        if(status == 1) continue;

        status = checkMultiLinePrint(line, &interpreter);
        if(status == -1){
            fclose(file);
            return -1;
        }
        if(status == 1) continue;

        if(handlePrint(line)) continue; 

        if(handleErrors(line) == -1){
            fclose(file);
            return -1;
        }

        
    }
    if(interpreter.multiLinePrint == 1){
        printf("Error: Unclosed multiline print statement\n");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}