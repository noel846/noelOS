char* video = (char*) 0xB8000;
int cursor = 0;

void print(char* str){
    int i = 0;
    while(str[i] != 0){
        video[cursor] = str[i];
        video[cursor + 1] = 0x0F;
        cursor = cursor + 2;
        i = i + 1;
    }
}

void kernel_main(){
    print("shalom from c");
    print(" hej fra c");
}
