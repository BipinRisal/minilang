#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct{
    int multiLinePrint;
} Interpreter;

void removeNewLine(char *line){
    line[strcspn(line, "\r\n")] = '\0';             // removes \r\n from the line. strcspn returns the position.
}

int checkMultiLinePrint(char *line, Interpreter *interpreter){
    if (strncmp(line,"print \"\"\"",9) == 0) {
            interpreter->multiLinePrint = 1;
            if (strlen((line+9)) != 0){
                printf("");
            }
            return 1;
        }
    return 0;
}

int handleMultiLinePrint(char *line, Interpreter *interpreter){
    if (interpreter->multiLinePrint == 0){
        return 0;
    }

    if (strncmp(line, "\"\"\"",3) == 0){
        interpreter->multiLinePrint = 0;
        return 1;
    }

    printf("%s",line);
    return 1;
}

int handlePrint(char *line){
    if (strncmp(line, "print ", 6) != 0) {     // compares two strings upto 6th position
        return 0;
    }       
    char *message = line + 6;                   

    size_t length = strlen(message);

    if (length >= 2 && message[0] == '"' && message[length - 1] == '"') {
        message[length - 1] = '\0';
        message++;
    }

    printf("%s\n", message);
    
    return 1;
}

int handleErrors(char *line){
    if (line[0] == '\0') return 0;
    printf("\nError: Unkown commad:%s\n", line);
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

    printf("Running program: %s\n", filename);

    char line[1024];
    Interpreter interpreter = {0};


    while (fgets(line, sizeof(line), file) != NULL) {

        removeNewLine(line);
        
        if(handleMultiLinePrint(line, &interpreter)) continue;
        
        if(checkMultiLinePrint(line, &interpreter)) continue;
        
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
