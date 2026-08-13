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

char digit_to_char(int d){
    return '0' + d;
}
void print_int(int n){
    if(n == 0){
        print("0");
        return;
    }
    if(n < 0){
        print("-");
        n = -n;
    }
    char buf[12];
    int len = 0;
    while(n > 0){
        int digit = n % 10;
        buf[len] = digit_to_char(digit);
        len = len + 1;
        n = n / 10;
    }
    int start = 0;
    int end = len - 1;
    while(start < end){
        char temp = buf[start];
        buf[start] = buf[end];
        buf[end] = temp;
        start = start + 1;
        end = end - 1;
    }
    buf[len] = 0;
    print(buf);
}

int streq(char* a, char* b){
    int  i = 0;
    while(a[i] != 0 && b[i] != 0){
        if(a[i] != b[i]) return 0;
        i = i + 1;
    }
    return a[i] == b[i];
}
int color_from_name(char* name){
    if(streq(name, "black")) return 0x0;
    if(streq(name, "blue")) return 0x1;
    if(streq(name, "green")) return 0x2;
    if(streq(name, "red")) return 0x4;
    if(streq(name, "white")) return 0xF;
    return -1;
}
void run_command(){
    char cmd[32];
    char arg[32];
    int i = 0;
    while(input[i] != 0 && input[i] != ' ') {
        cmd[i] = input[i];
        i = i + 1;
    }
    cmd[i] = 0;
    if(input[i] == ' '){
        i = i + 1;
    }
    int j = 0;
    while(input[i] != 0){
        arg[j] = input[i];
        i = i + 1;
        j = j + 1;
    }
    arg[j] = 0;
    if(streq(cmd, "hello")){
        print("\nwaddup\n");
    } else if(streq(cmd, "clear")){
        clear();
    } else if (streq(cmd, "bgcol")) {
        int c  =  color_from_name(arg);
        if(c >= 0){
            color = (c << 4) | (color & 0x0F);
            clear();
        } else {
            print("\nunknown color: ");
            print(arg);
            print("\n");
        }
    } else if(streq(cmd, "txcol")){
        int c = color_from_name(arg);
        if(c >= 0){
            color = (color & 0xF0) | c;
        } else {
            print("\nunknown color: ");
            print(arg);
            print("\n");
        }
    }  else{
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
