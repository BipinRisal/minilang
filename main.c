#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_VARS 64
#define MAX_NAME 32

typedef struct{
    char name[MAX_NAME];
    int value;
} Variable;

typedef struct{
    int multiLinePrint;
    Variable variables[MAX_VARS];
    int variableCount;
} Interpreter;

void removeNewLine(char *line){
    line[strcspn(line, "\r\n")] = '\0';             // removes \r\n from the line. strcspn returns the position.
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';                         // removes trailing spaces and tabs
    }
}

Variable *findVariable(Interpreter *interpreter, const char *name){
    for (int i = 0; i < interpreter->variableCount; i++){
        if (strcmp(interpreter->variables[i].name, name) == 0){
            return &interpreter->variables[i];
        }
    }
    return NULL;
}

int printEscaped(const char *s, Interpreter *interpreter){
    for (; *s != '\0'; s++){
        if (*s == '{'){
            const char *end = strchr(s, '}');
            if (end != NULL && end - s - 1 > 0 && end - s - 1 < MAX_NAME){
                char name[MAX_NAME];
                strncpy(name, s + 1, end - s - 1);
                name[end - s - 1] = '\0';
                Variable *var = findVariable(interpreter, name);
                if (var == NULL){
                    printf("\nError: Undefined variable: %s\n", name);
                    return -1;
                }
                printf("%d", var->value);
                s = end;
                continue;
            }
        }
        if (*s == '\\' && s[1] != '\0'){
            s++;
            switch (*s){
                case 'n':  putchar('\n'); break;
                case 't':  putchar('\t'); break;
                case '\\': putchar('\\'); break;
                case '"':  putchar('"');  break;
                case '{':  putchar('{');  break;
                default:   putchar('\\'); putchar(*s); break;
            }
        } else {
            putchar(*s);
        }
    }
    return 1;
}


int setVariable(Interpreter *interpreter, const char *name, int value){
    Variable *var = findVariable(interpreter, name);
    if (var != NULL){
        var->value = value;
        return 1;
    }
    if (interpreter->variableCount >= MAX_VARS){
        return 0;
    }
    var = &interpreter->variables[interpreter->variableCount++];
    strcpy(var->name, name);
    var->value = value;
    return 1;
}

int handleAssignment(char *line, Interpreter *interpreter){
    char name[MAX_NAME];
    int value;
    int consumed = 0;

    if (sscanf(line, " %31[A-Za-z_] = %d %n", name, &value, &consumed) != 2){  //scanset : tells the sscanf to read characters matching the charachter set i.e. uppercase, lowercase, underscore and 31 max characters to read
        return 0;
    }
    if (line[consumed] != '\0'){
        return 0;
    }
    if (!setVariable(interpreter, name, value)){
        printf("\nError: Too many variables\n");
        return -1;
    }
    return 1;
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

    if (printEscaped(line, interpreter) == -1) return -1;
    return 1;
}

int handlePrint(char *line, Interpreter *interpreter){
    if (strncmp(line, "print ", 6) != 0) {
        return 0;
    }
    char *message = line + 6;

    size_t length = strlen(message);

    if (length >= 2 && message[0] == '"' && message[length - 1] == '"') {
        message[length - 1] = '\0';
        message++;
        if (printEscaped(message, interpreter) == -1) return -1;
        printf("\n");
        return 1;
    }
    Variable *var = findVariable(interpreter, message);
    if (var == NULL){
        printf("\nError: Undefined variable: %s\n", message);
        return -1;
    }
    printf("%d\n", var->value);
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

         status = handlePrint(line, &interpreter);
        if(status == -1){
            fclose(file);
            return -1;
        }
        if(status == 1) continue;

        status = handleAssignment(line, &interpreter);
        if(status == -1){
            fclose(file);
            return -1;
        }
        if(status == 1) continue; 

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