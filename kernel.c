void kernel_main(){
  char* video = (char*) 0xB8000;
  video[0] = 'A';
  video[1] = 0x0F; 
  video[2] = 'B'; 
  video[3] = 0x0F; 
  video[4] = 'C'; 
  video[5] = 0x0F;   
}