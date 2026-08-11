void kernel_main(){
    volatile char* vid = (volatile char*) 0xB8000;
    vid[0]='s'; vid[1]=0x0F;
    vid[2]='h'; vid[3]=0x0F;
    vid[4]='a'; vid[5]=0x0F;
    vid[6]='l'; vid[7]=0x0F;
    vid[8]='o'; vid[9]=0x0F;
    vid[10]='m'; vid[11]=0x0F;
    vid[12]=' '; vid[13]=0x0F;
    vid[14]='f'; vid[15]=0x0F;
    vid[16]='r'; vid[17]=0x0F;
    vid[18]='o'; vid[19]=0x0F;
    vid[20]='m'; vid[21]=0x0F;
    vid[22]=' '; vid[23]=0x0F;
    vid[24]='c'; vid[25]=0x0F;
}
