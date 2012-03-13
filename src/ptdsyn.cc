/*
 *	epos/src/ptdsyn.cc
 *	(c) 1998 Martin Petriska, petriska@decef.elf.stuba.sk
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *
 *
 */

#include "common.h"
#include "ptdsyn.h"

//#ifdef HAVE_MATH_H
#include <math.h>
//#endif

/* Kruhovy buffer */
kbuffer::kbuffer(int len)
{
 N=len;
 i1=0;
 i2=0;
 i3=0;
 oST=400;
 data=new sample_type[len+1];
 for(int k=0;k<N;k++)
  data[k]=0; 
};

sample_type kbuffer::kbread()
{sample_type y;
y=data[i1];
data[i1]=0;
if(i1<(N-1))i1++;
else i1=0;
return(y);
};

void kbuffer::kbadd(sample_type x)
{long y;
y=data[i3]+x;
if(y>32767)y=32767;
if(y<-32766)y=-32766;
data[i3]=(sample_type)y;
if(i3<(N-1))i3++;
else i3=0;
};
void kbuffer::kbaddST(sample_type *x, int ST, int ind)
{
oST=ST;
i2+=ind;
if(i2>=N)i2=i2-N;
if(i2<0)i2=N-i2;
i3=i2;
for(int k=0;k<ST;k++)
 kbadd(x[k]);
 
};
kbuffer::~kbuffer()
{
 delete(data);
};

double ptdsyn::window(int I, int N, WIN w)
{double v;
 switch(w){case HANN:
                     v=0.5-0.5*cos((double)(2*3.1415926536*I/N));
		     break;
              case HAMMING:
	             v=0.54-0.46*cos((double)(2*3.1415926536*I/N));
		     break;
              case PARZEN:
	             v=1-fabs((double)(2*I-N)/(double)(N+1));
		     break;
	      case BLACKMAN:
	             v=0.42+0.5*cos((double)(2*3.1415926536*(I-N/2)/ST))+0.08*cos((double)(4*3.1415926536*(I-N/2)/N));
	             break;
	      default: v=1; 
             }
 return(v);	     
};
/*
     ANALYZA roznasobenie oknom     
  
*/
int ptdsyn::analyza(DIFON *dfn, sample_type *waveout, int *tas, voice *v)
{
  
 sample_type *wavein;
 wavein=new sample_type[WAVEMAX]; 
 strcpy(scratch, "segments/");
 strcat(scratch, dfn->fname);
 strcat(scratch, ".wav");
 char *filename = compose_pathname(scratch, v->loc, cfg->inv_base_dir);


/* filename=new char[MAXFILENAME];
 strcpy(filename, cfg->base_dir);
 strcat(filename,"/");
 strcat(filename, cfg->inv_dir);
 strcat(filename,"/petriska/segments/");
 strcat(filename,dfn->fname);
 strcat(filename,".wav");
*/
 
 FILE *fil;

 fil=fopen(filename,"rb");
 if (!fil) shriek(445, fmt("ptdsyn cannot find segments in %s", filename));
 else {

 unsigned char *buf;
 buf=(unsigned char*)xcalloc(50,1);
 fread(buf,1,44,fil); // WAVE head
 if(buf)free(buf);
 int count3=0;
 while(!feof(fil)){
   fread(&wavein[count3],sizeof(sample_type),1,fil);
   count3++;
   }
   count3--;
/* Analyza signalu; waveout=W*x */
 tas[0]=0;  
 ST=v->st;
 for(i=0;i<dfn->pimp;i++)
 {
    
   tas[i+1]=tas[i]+ST;      
  for(int j=0;j<ST;j++){
        int indx=dfn->labels[i]+j-(ST>>1); 
	if((indx>=0)&&(indx<count3)){
	  waveout[i*ST+j]=(sample_type)((window(j,ST,HAMMING)*wavein[indx]));	 
	 }
        else waveout[i*ST+j]=0; 
  }	
 }

 }
 fclose(fil);
 free(filename);
 delete(wavein);  
 return(0);
};
/*

     uprava na novu frekvenciu a cas 

*/
int ptdsyn::modifik(sample_type *wave, int pimp, int npimp, int nperiN, int *ta)
{
 for(int k=0;k<npimp;k++)
  {
  int aimp=(int)(k*pimp/npimp);
  int sST=ta[aimp+1]-ta[aimp];
  K->kbaddST(&wave[ta[aimp]],sST,(int)((K->oST+2*nperiN-sST)/2));
  }
  return(0);
};
/*

     Konstruktor ptdsyn

*/
ptdsyn::ptdsyn(voice *v)
{ dif=new DIFON[2000];
  K=new kbuffer(5000);
  if(!dif)fprintf(stderr,"Not enough memory for DIFON");
  FILE *f;
  char * pathname = compose_pathname("difon.dat", v->loc, cfg->inv_base_dir);
 if((f=fopen(pathname,"rt"))==NULL){
  fprintf(stderr,"Cannot open file :%s",pathname);
  exit(1); 
 }
 free(pathname);
 int count=0;
 while(!feof(f))
  {int x,y;
   fscanf(f,"%s%s%d%d",dif[count].name,dif[count].fname,&x,&y);
   count++;
  }
 fclose(f);
 char *filename;
// filename=new char[MAXFILENAME];
 for(int i=0;i<(count-1);i++)

  {	

  strcpy(scratch, "labels/");
  strcat(scratch, dif[i].fname);
  strcat(scratch, ".label");
  filename = compose_pathname(scratch, v->loc, cfg->inv_base_dir);
/*        strcpy (filename, cfg->base_dir);
        strcat (filename, "/");
	strcat (filename,v->inv_dir); 
        strcat(filename,"/petriska/labels/");
	strcat(filename,dif[i].fname);
	strcat(filename,".label");
*/
  f=fopen(filename,"rb");
  if (!f) ;//shriek(499, fmt("ptdsyn cannot find labels at %s", filename));
  else {
	char *buf=(char *)xcalloc(50,1);
	fscanf(f,"%s",buf);
	if(!strcmp(buf,"Labels"))
	{	 
	 int num;
	 int named;
	 char name[120];
	 int r,g,b;
	 fscanf(f,"%s%d%s%d%d%d%d",buf,&num,name,&named,&r,&g,&b);
	 fscanf(f,"%s",buf);
	 int count2=0;
	 while(!feof(f))
         {
	  fscanf(f,"%d%d",&num,&dif[i].labels[count2]);
          count2++;
         }
	 count2--; 
	 count2--;
	 dif[i].pimp=count2;
	 dif[i].time=10*count2;
	 free(buf);
	 fclose(f);
	}
  }
  free(filename);
  }
};

void ptdsyn::synseg(voice *v, segment d,wavefm *w)
{
 ta=new int[100];
 PSL=new sample_type[WAVEMAX];
 Fvz1=16000;// inventory sample rate
 fn=Fvz1/(double)d.f; 		// FIXME is it good ? fn  - frequency Fo [Hz]
 ntime=(double)(dif[d.code].time*d.t)/100;// new time  [ms] (segment length)
 nperi=1000/fn;          //period of new signal [ms]
 nperiN=(int)(Fvz1/fn);
 npimp=(int)(ntime/nperi+0.5);
 if(dif[d.code].pimp!=0)analyza(&dif[d.code],PSL,ta,v); //PSL ST sample data (windowed with lenght ST) 
 modifik(PSL,dif[d.code].pimp,npimp,nperiN,ta);
 do
   {    
    w->sample((int)((double)(K->kbread())*d.e/100));
   }
 while(K->i1!=K->i2);
         
 delete (PSL);
 delete (ta);
  
};

ptdsyn::~ptdsyn()
{
 delete K;
};
