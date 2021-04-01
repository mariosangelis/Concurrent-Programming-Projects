int compare_res; /*variable for the return value of the compareFiles()*/
int compareFiles(FILE *file1,FILE *file2);
void diff(FILE *file1,FILE *file2);

void diff(FILE *file1,FILE *file2) {
    file1=fopen("file1.jpg","r");
	file2=fopen("file2.jpg","r");
	if(file1==NULL || file2==NULL) {
		fprintf(stderr,"Error with fopen().\n");
		exit(EXIT_FAILURE);
	}
	compare_res=compareFiles(file1,file2);
	if(compare_res==0) {
		printf("\n=====diff file1 file2=====\n");
		printf("No difference between file1 and file2.\n\n");
	}
	else {
		printf("\n=====diff file1 file2=====\n");
		printf("There are differencies between the files.\n\n");
	}
}

int compareFiles(FILE *fp1,FILE *fp2) {
	char ch1=getc(fp1);
	char ch2=getc(fp2);
	int error=0;
	int pos=0;
	int line=1;
	while (ch1 != EOF && ch2 != EOF) {
		pos++;
		// if both variable encounters new
        // line then line variable is incremented
        // and pos variable is set to 0
        if (ch1 == '\n' && ch2 == '\n') {
            line++;
            pos = 0;
        }
        // if fetched data is not equal then
        // error is incremented
        if (ch1 != ch2) {
            error++;
        }
		// fetching character until end of file
        ch1 = getc(fp1);
        ch2 = getc(fp2);
    }
	if(error==0) {
		return(0);
	}
	else {
		printf("Total Errors : %d\t", error);
		return(1);
    }
}
