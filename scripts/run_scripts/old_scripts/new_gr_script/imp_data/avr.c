//
//
//  This program is designed to caculate the mean value of a given sheet.
//  And the sheet is a format file which contains data having format data like
//  'benchname : double value' for each row,which is the test value of Certain
//  Case and group..,for the likes of a format file,see ./tmp
//
//  Usage : ./$PROGRAM $FORMATFILE
//
//  Author @cyz  mail:lonrun@126.com
//  time@2013-7-20 10:43:12
//
//
# include <stdio.h>
# include <string.h>
# include <stdlib.h>

int main(int argc,char *argv[])
{
    FILE *in,*out;
    if(argc>=2){
        in = fopen(argv[1],"r");
        out = fopen("out.dat","ab+");

        if(in==NULL || out==NULL){
            printf("file open failed,please check.\n");
            exit(-1);
        }

        double t1,t2,s1,s2;
        int idx,i,j,k;
        char tmp[10],bm[3]={'\0'};
        char bn1[5],bn2[5];

        fprintf(out,"%s\n","The Mean Value of each group Under Certain Cases.");
        for(i=0; i<=7; i++){
            strcpy(tmp,"Case ");
            sprintf(bm,"%d",i);
            strcat(tmp,bm);
            fprintf(out,"%s\n",tmp);
            fprintf(out,"%s\n","-------------");

            for(k=0; k<3; k++){
                s1 = s2 = 0.0;
                for(j=0;j<3;j++){
                    fscanf(in,"%s %lf",bn1,&t1);
                    fscanf(in,"%s %lf",bn2,&t2);

                    s1 += t1;
                    s2 += t2;
                }
                fprintf(out,"%s : %7.2lf\n",bn1,s1/3);
                fprintf(out,"%s : %7.2lf\n\n",bn2,s2/3);
            }
        }
        fclose(in);
        fclose(out);
    }else printf("please indicate a format data file.eg:tmp.dat.\n");
    return 0;
}
