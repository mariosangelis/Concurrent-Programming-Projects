volatile int flag_read=1;                /*Ginetai 1  otan termatizei h pipe_read*/
volatile int flag_write=1;               /*Ginetai 1 otan termatizei h pipe_write*/
volatile int index_write=0;              
volatile int index_read=0;
volatile int close_flag=0;               /*Ginetai 1 otan ftasoume se EOF*/
volatile int size;                       /*Size tou buffer,dinetai os orisma*/
char *buffer;
void pipe_init(int size){                                                                    /*Initialize buffer*/
    buffer=(char *)malloc(sizeof(char)*size);
}
void pipe_close(){
    free(buffer);
}
void pipe_read(char *c){
    c[0]=buffer[index_read];
}
void pipe_write(char c){
    buffer[index_write]=c;
}
