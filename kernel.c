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
        } else{
            video[cursor] = str[i];
            video[cursor + 1] = 0x0F;
            cursor = cursor + 2;
        }
        i = i + 1;
    }
}

void kernel_main(){
    clear();
    print("shalom from c\n");
    print("line two\n");
    print("line 3");
}
