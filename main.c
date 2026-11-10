#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_VARS 64
#define MAX_NAME 32
#define MAX_LINES 1000
#define MAX_DEPTH 32

static char program[MAX_LINES][1024];

typedef struct{
    char name[MAX_NAME];
    int value;
} Variable;

typedef struct{
    int multiLinePrint;
    int lineNumber;
    int multiLineStart;
    int pc;
    int lineCount;
    int stack[MAX_DEPTH];
    int depth;
    Variable variables[MAX_VARS];
    int variableCount;
} Interpreter;

void removeNewLine(char *line){
    line[strcspn(line, "\r\n")] = '\0';             
    size_t len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';                       
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

int getValue(const char *token, Interpreter *interpreter, int *out){
    char *endptr;
    long number = strtol(token, &endptr, 10);
    if (endptr != token && *endptr == '\0'){
        *out = (int)number;
        return 1;
    }
    Variable *var = findVariable(interpreter, token);
    if (var == NULL){
        printf("\nError: Undefined variable: %s\n", token);
        return -1;
    }
    *out = var->value;
    return 1;
}

int evalCondition(const char *text, Interpreter *interpreter, int *result){
    char a[MAX_NAME], op[3], b[MAX_NAME];
    if (sscanf(text, "%31s %2s %31s", a, op, b) != 3){
        printf("\nError: Invalid condition:%s\n", text);
        return -1;
    }
    int left, right;
    if (getValue(a, interpreter, &left) == -1) return -1;
    if (getValue(b, interpreter, &right) == -1) return -1;

    if      (strcmp(op, "==") == 0) *result = (left == right);
    else if (strcmp(op, "!=") == 0) *result = (left != right);
    else if (strcmp(op, "<")  == 0) *result = (left <  right);
    else if (strcmp(op, ">")  == 0) *result = (left >  right);
    else if (strcmp(op, "<=") == 0) *result = (left <= right);
    else if (strcmp(op, ">=") == 0) *result = (left >= right);
    else {
        printf("\nError: Unknown operator: %s\n", op);
        return -1;
    }
    return 1;
}

const char *skipSpaces(const char *s){
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

int isKeyword(const char *line, const char *word){
    size_t n = strlen(word);
    return strncmp(line, word, n) == 0 && (line[n] == '\0' || line[n] == ' ');
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
    int consumed = 0;

    sscanf(line, " %31[A-Za-z_] = %n", name, &consumed);
    if (consumed == 0){
        return 0;
    }

    char a[MAX_NAME], op[2], b[MAX_NAME];
    int parts = sscanf(line + consumed, "%31s %1s %31s", a, op, b);
    int value, left, right;

    if (parts == 1){
        if (getValue(a, interpreter, &value) == -1) return -1;
    } else if (parts == 3){
        if (getValue(a, interpreter, &left) == -1) return -1;
        if (getValue(b, interpreter, &right) == -1) return -1;
        switch (op[0]){
            case '+': value = left + right; break;
            case '-': value = left - right; break;
            case '*': value = left * right; break;
            case '/':
            case '%':
                if (right == 0){
                    printf("\nError: Division by zero\n");
                    return -1;
                }
                value = (op[0] == '/') ? left / right : left % right;
                break;
            default:
                printf("\nError: Unknown operator: %s\n", op);
                return -1;
        }
    } else {
        return 0;
    }

    if (!setVariable(interpreter, name, value)){
        printf("\nError: Too many variables\n");
        return -1;
    }
    return 1;
}

int findMatching(Interpreter *interpreter, int from, int stopAtElse){
    int depth = 0;
    int inBlock = 0;
    for (int i = from + 1; i < interpreter->lineCount; i++){
        const char *text = program[i];
        if (inBlock){
            if (strncmp(text, "\"\"\"", 3) == 0) inBlock = 0;
            continue;
        }
        text = skipSpaces(text);
        if (strncmp(text, "print \"\"\"", 9) == 0){
            inBlock = 1;
            continue;
        }
        if (isKeyword(text, "if") || isKeyword(text, "while")){
            depth++;
        } else if (isKeyword(text, "end")){
            if (depth == 0) return i;
            depth--;
        } else if (stopAtElse && depth == 0 && isKeyword(text, "else")){
            return i;
        }
    }
    return -1;
}

int pushBlock(Interpreter *interpreter, int line){
    if (interpreter->depth >= MAX_DEPTH){
        printf("\nError: Blocks are nested too deeply\n");
        return -1;
    }
    interpreter->stack[interpreter->depth++] = line;
    return 1;
}

int handleBlocks(char *line, Interpreter *interpreter){
    int current = interpreter->lineNumber - 1;

    if (isKeyword(line, "if") || isKeyword(line, "while")){
        int isWhile = isKeyword(line, "while");
        int result;
        if (evalCondition(line + (isWhile ? 5 : 2), interpreter, &result) == -1) return -1;

        if (result){
            if (pushBlock(interpreter, current) == -1) return -1;
            return 1;
        }

        int target = findMatching(interpreter, current, !isWhile);
        if (target == -1){
            printf("\nError: Missing end for block\n");
            return -1;
        }
        if (isKeyword(skipSpaces(program[target]), "else")){
            if (pushBlock(interpreter, current) == -1) return -1;
        }
        interpreter->pc = target + 1;
        return 1;
    }

    if (isKeyword(line, "else")){
        if (interpreter->depth == 0){
            printf("\nError: else without if\n");
            return -1;
        }
        int target = findMatching(interpreter, current, 0);
        if (target == -1){
            printf("\nError: Missing end for block\n");
            return -1;
        }
        interpreter->depth--;
        interpreter->pc = target + 1;
        return 1;
    }

    if (isKeyword(line, "end")){
        if (interpreter->depth == 0){
            printf("\nError: end without if or while\n");
            return -1;
        }
        int opener = interpreter->stack[--interpreter->depth];
        if (isKeyword(skipSpaces(program[opener]), "while")){
            interpreter->pc = opener;
        }
        return 1;
    }

    return 0;
}

int checkMultiLinePrint(char *line, Interpreter *interpreter){
    if (strncmp(line,"print \"\"\"",9) == 0) {
        if (line[9] != '\0'){
            printf("Error: Multiline print must start with print \"\"\" on its own line\n");
            return -1;
        }
        interpreter->multiLinePrint = 1;
        interpreter->multiLineStart = interpreter->lineNumber;
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

int fail(Interpreter *interpreter){
    printf("  (line %d)\n", interpreter->lineNumber);
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 2){
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
        if (interpreter.lineCount >= MAX_LINES){
            printf("Error: Program is too long (max %d lines)\n", MAX_LINES);
            fclose(file);
            return 1;
        }
        removeNewLine(line);
        strcpy(program[interpreter.lineCount++], line);
    }
    fclose(file);

    while (interpreter.pc < interpreter.lineCount) {
        int index = interpreter.pc++;
        interpreter.lineNumber = index + 1;
        strcpy(line, program[index]);

        char *text = line;
        if (!interpreter.multiLinePrint){
            while (*text == ' ' || *text == '\t') text++;
        }

        int status = handleMultiLinePrint(text, &interpreter);
        if(status == -1) return fail(&interpreter);
        if(status == 1) continue;

        status = checkMultiLinePrint(text, &interpreter);
        if(status == -1) return fail(&interpreter);
        if(status == 1) continue;

        status = handleBlocks(text, &interpreter);
        if(status == -1) return fail(&interpreter);
        if(status == 1) continue;

        status = handlePrint(text, &interpreter);
        if(status == -1) return fail(&interpreter);
        if(status == 1) continue;

        status = handleAssignment(text, &interpreter);
        if(status == -1) return fail(&interpreter);
        if(status == 1) continue;

        if(handleErrors(text) == -1) return fail(&interpreter);
    }

    if(interpreter.multiLinePrint == 1){
        printf("Error: Unclosed multiline print statement (started on line %d)\n", interpreter.multiLineStart);
        return -1;
    }
    if(interpreter.depth > 0){
        printf("Error: Block opened on line %d is missing end\n", interpreter.stack[interpreter.depth - 1] + 1);
        return -1;
    }

    return 0;
}