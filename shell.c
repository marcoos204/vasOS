#include "user.h"

void main(void) {
    while (1) {
prompt: 
        printf("> ");
        char cmdline[128];
        for (int i = 0;; i++){
            char ch = getchar(); //reads serial port input (in qemus case, connected to the keyboard)
                                 //since this syscall basically halts the program i believe that each i iteration controls the number of characters of the strings
            putchar(ch); //puts the character you just input in the cmdline
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') { //on debug console newline character is \r
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0)
            printf("Hello world from shell!\n");
        else if (strcmp(cmdline, "exit") == 0)
            exit();
        else if (strcmp(cmdline, "readfile") == 0) {
            char buf[128];
            int len = readfile("hello.txt", buf, sizeof(buf));
            buf[len] = '\0';
            printf("%s\n", buf);
        }
        else if (strcmp(cmdline, "writefile") == 0)
            writefile("hello.txt", "Hello from shell!\n", 19);
        else
            printf("what? lol (unknown command: %s)\n", cmdline);
    }
}