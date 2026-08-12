char* video = (char*) 0xB8000;
int cursor = 0;

unsigned char color = 0x0F;

char input[256];
int input_len = 0;


void clear(){
    int i = 0;
    while(i < 80 * 25 * 2){
        video[i] = ' ';
        video[i + 1] = color;
        i = i + 2;
    }
    cursor = 0;
}
void print(char* str){
    int i = 0;
    while(str[i] != 0){
        if(str[i] == '\n'){
            cursor = cursor + (160 - cursor % 160);
        } else if (str[i] == '\b'){
            if (cursor >= 2){
                cursor = cursor - 2;
                video[cursor] = ' ';
                video[cursor + 1] = color;
            }
        } else{
            video[cursor] = str[i];
            video[cursor + 1] = color;
            cursor = cursor + 2;
        }
        i = i + 1;
    }
}

unsigned char inb(unsigned short port){
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}
char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};
char getkey(){
    unsigned char scancode;
    do {
        while((inb(0x64) & 1) == 0);
        scancode = inb(0x60);
    }while (scancode & 0x80);
    if(scancode < sizeof(scancode_to_ascii)){
        return scancode_to_ascii[scancode];
    }
    return 0;
}

int streq(char* a, char* b){
    int  i = 0;
    while(a[i] != 0 && b[i] != 0){
        if(a[i] != b[i]) return 0;
        i = i + 1;
    }
    return a[i] == b[i];
}
void run_command(){
    if(streq(input, "hello")){
        print("\nwaddup\n");
    } else if(streq(input, "clear")){
        clear();
    } else if (streq(input, "bgcol red")) {
        color = (0x4 << 4) | (color &0x0f);
        clear();
    } else if (streq(input, "bgcol black")) {
        color = (0x0 << 4) | (color &0x0f);
        clear();
    } else if (streq(input, "txcol blue")) {
        color = (color & 0xF0) | 0x1;
    } else if (streq(input, "txcol white")) {
        color = (color & 0xF0) | 0xF;
    } else{
        print("\nunknown command: ");
        print(input);
        print("\n");
    }
    print("> ");
}

void kernel_main(){
    clear();
    print("noelOS\n> ");
    while(1){
        char c = getkey();
        if(c == '\n'){
            input[input_len] = 0;
            print("\n");
            if(input_len > 0){
                run_command();
            } else{
                print("> ");
            }
            input_len = 0;
        } else if (c == '\b') {
            if(input_len > 0){
                input_len = input_len - 1;
                print("\b");
            }
        } else if (c != 0) {
            input[input_len] = c;
            input_len = input_len +1;
            char str[2];
            str[0] = c;
            str[1] = 0;
            print(str);
        
        }
    }
}
