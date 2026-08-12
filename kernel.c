char* video = (char*) 0xB8000;
int cursor = 0;


void clear(){
    int i = 0;
    while(i < 80 * 25 * 2){
        video[i] = ' ';
        video[i + 1] = 0x0F;
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
                video[cursor + 1] = 0x0F;
            }
        } else{
            video[cursor] = str[i];
            video[cursor + 1] = 0x0F;
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

void kernel_main(){
    clear();
    print("noelOS\n> ");
    while(1){
        char c = getkey();
        if(c != 0){
            char str[2];
            str[0] = c;
            str[1] = 0;
            print(str);
        }
    }
}
